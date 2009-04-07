/*
 * Copyright (c) 2008 James Molloy, Jörg Pfähler, Matthew Iselin
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "Ext2Directory.h"
#include "Ext2Filesystem.h"
#include <syscallError.h>
#include "Ext2File.h"
#include "Ext2Symlink.h"

Ext2Directory::Ext2Directory(String name, uintptr_t inode_num, Inode inode,
                             Ext2Filesystem *pFs, File *pParent) :
  Directory(name, LITTLE_TO_HOST32(inode.i_atime),
            LITTLE_TO_HOST32(inode.i_mtime),
            LITTLE_TO_HOST32(inode.i_ctime),
            inode_num,
            static_cast<Filesystem*>(pFs),
            LITTLE_TO_HOST32(inode.i_size), /// \todo Deal with >4GB files here.
            pParent),
  Ext2Node(inode_num, inode, pFs)
{
  uint32_t mode = LITTLE_TO_HOST32(inode.i_mode);
  uint32_t permissions;
  if (mode & EXT2_S_IRUSR) permissions |= FILE_UR;
  if (mode & EXT2_S_IWUSR) permissions |= FILE_UW;
  if (mode & EXT2_S_IXUSR) permissions |= FILE_UX;
  if (mode & EXT2_S_IRGRP) permissions |= FILE_GR;
  if (mode & EXT2_S_IWGRP) permissions |= FILE_GW;
  if (mode & EXT2_S_IXGRP) permissions |= FILE_GX;
  if (mode & EXT2_S_IROTH) permissions |= FILE_OR;
  if (mode & EXT2_S_IWOTH) permissions |= FILE_OW;
  if (mode & EXT2_S_IXOTH) permissions |= FILE_OX;

  setPermissions(permissions);
  setUid(LITTLE_TO_HOST16(inode.i_uid));
  setGid(LITTLE_TO_HOST16(inode.i_gid));
}

Ext2Directory::~Ext2Directory()
{
}

bool Ext2Directory::addEntry(String filename, File *pFile, size_t type)
{
  // Make sure we know what blocks we're using.
  if (m_Blocks.count() != m_nBlocks)
  {
    if (!getBlockNumbers())
    {
      FATAL("EXT2: Unable to get block numbers!");
    }
  }

  // Calculate the size of our Dir* entry.
  size_t length = 4 + /* 32-bit inode number */
    2 + /* 16-bit record length */
    1 + /* 8-bit name length */
    1 + /* 8-bit file type */
    filename.length(); /* Don't leave space for NULL-terminator, not needed. */

  // Load in and scan the directory, looking for a space large enough to fit our directory descriptor.
  uint8_t *pBuffer = new uint8_t[m_pExt2Fs->m_BlockSize];

  bool bFound = false;

  uint32_t i;
  Dir *pDir;
  for (i = 0; i < m_nBlocks; i++)
  {
    m_pExt2Fs->readBlock(reinterpret_cast<uint32_t>(m_Blocks[i]), reinterpret_cast<uintptr_t>(pBuffer));
    pDir = reinterpret_cast<Dir*>(pBuffer);
    while (pDir->d_inode != 0 &&
           reinterpret_cast<uintptr_t>(pDir) < reinterpret_cast<uintptr_t>(pBuffer)+m_pExt2Fs->m_BlockSize)
    {
      // What's the minimum length of this directory entry?
      size_t thisReclen = 4 + 2 + 1 + 1 + pDir->d_namelen;
      // Is there enough space to add this dirent?
      /// \todo Ensure 4-byte alignment.
      if (pDir->d_reclen - thisReclen >= length)
      {
        bFound = true;
        // Save the current reclen.
        uint16_t oldReclen = pDir->d_reclen;
        // Adjust the current record's reclen field to the minimum.
        pDir->d_reclen = thisReclen;
        // Move to the new directory entry location.
        pDir = reinterpret_cast<Dir*> (reinterpret_cast<uintptr_t>(pDir)+thisReclen);
        // set the new record length.
        pDir->d_reclen = oldReclen-thisReclen;
        break;
      }

      // Next.
      pDir = reinterpret_cast<Dir*> (reinterpret_cast<uintptr_t>(pDir)+pDir->d_reclen);
    }
    if (bFound) break;
  }

  if (!bFound)
  {
    // Need to make a new block.
    uint32_t block = m_pExt2Fs->findFreeBlock(m_InodeNumber);
    if (block == 0)
    {
      // We had a problem.
      SYSCALL_ERROR(NoSpaceLeftOnDevice);
      return false;
    }
    if (!addBlock(block)) return false;
    i = m_nBlocks-1;

    m_Size = m_nBlocks * m_pExt2Fs->m_BlockSize;
    fileAttributeChanged();

    memset(pBuffer, 0, m_pExt2Fs->m_BlockSize);
    pDir = reinterpret_cast<Dir*> (pBuffer);
    pDir->d_reclen = m_pExt2Fs->m_BlockSize;
  }

  // Set the directory contents.
  pDir->d_inode = HOST_TO_LITTLE32(pFile->getInode());

  switch (type)
  {
    case EXT2_S_IFREG:
      pDir->d_file_type = EXT2_FILE;
      break;
    case EXT2_S_IFDIR:
      pDir->d_file_type = EXT2_DIRECTORY;
      break;
    case EXT2_S_IFLNK:
      pDir->d_file_type = EXT2_SYMLINK;
      break;
    default:
      ERROR("Unrecognised filetype.");
  }

  pDir->d_namelen = filename.length();
  memcpy(pDir->d_name, filename, filename.length());

  // Write the block back to disk.
  m_pExt2Fs->writeBlock(reinterpret_cast<uintptr_t>(m_Blocks[i]), reinterpret_cast<uintptr_t>(pBuffer));

  delete [] pBuffer;

  // We're all good - add the directory to our cache.
  m_Cache.insert(filename, pFile);

  m_Size = m_nSize;

  return true;
}

bool Ext2Directory::removeEntry(Ext2Node *pFile)
{
  m_Size = m_nSize;
  return false;
}

void Ext2Directory::cacheDirectoryContents()
{
  // Make sure we know what blocks we're using.
  if (m_Blocks.count() != m_nBlocks)
  {
    if (!getBlockNumbers())
    {
      FATAL("EXT2: Unable to get block numbers!");
    }
  }

  // Load in and scan the directory.
  uint8_t *pBuffer = new uint8_t[m_pExt2Fs->m_BlockSize];

  uint32_t i;
  Dir *pDir;
  for (i = 0; i < m_nBlocks; i++)
  {
    m_pExt2Fs->readBlock(reinterpret_cast<uint32_t>(m_Blocks[i]), reinterpret_cast<uintptr_t>(pBuffer));
    pDir = reinterpret_cast<Dir*>(pBuffer);

    while (pDir->d_inode != 0 &&
           reinterpret_cast<uintptr_t>(pDir) < reinterpret_cast<uintptr_t>(pBuffer)+m_pExt2Fs->m_BlockSize)
    {
      char *filename = new char[pDir->d_namelen+1];
      memcpy(filename, pDir->d_name, pDir->d_namelen);
      filename[pDir->d_namelen] = '\0';
      String sFilename(filename);
      delete [] filename;

      File *pFile = 0;
      switch (pDir->d_file_type)
      {
        case EXT2_FILE:
          pFile = new Ext2File(sFilename, LITTLE_TO_HOST32(pDir->d_inode), m_pExt2Fs->getInode(LITTLE_TO_HOST32(pDir->d_inode)), m_pExt2Fs, this);
          break;
        case EXT2_DIRECTORY:
          pFile = new Ext2Directory(sFilename, LITTLE_TO_HOST32(pDir->d_inode), m_pExt2Fs->getInode(LITTLE_TO_HOST32(pDir->d_inode)), m_pExt2Fs, this);
          break;
        case EXT2_SYMLINK:
          pFile = new Ext2Symlink(sFilename, LITTLE_TO_HOST32(pDir->d_inode), m_pExt2Fs->getInode(LITTLE_TO_HOST32(pDir->d_inode)), m_pExt2Fs, this);
          break;
        default:
          ERROR("EXT2: Unrecognised file type: " << pDir->d_file_type);
      }

      // Add to cache.
      m_Cache.insert(sFilename, pFile);

      // Next.
      pDir = reinterpret_cast<Dir*> (reinterpret_cast<uintptr_t>(pDir)+LITTLE_TO_HOST16(pDir->d_reclen));
    }
  }

  delete [] pBuffer;

  m_bCachePopulated = true;
}

void Ext2Directory::fileAttributeChanged()
{
  static_cast<Ext2Node*>(this)->fileAttributeChanged(m_Size, m_AccessedTime, m_ModifiedTime, m_CreationTime);
}
