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

#include "Ext2Filesystem.h"
#include "Ext2File.h"
#include "Ext2Directory.h"
#include "Ext2Symlink.h"
#include <vfs/VFS.h>
#include <Module.h>
#include <utilities/utility.h>
#include <Log.h>
#include <utilities/List.h>
#include <processor/Processor.h>
#include <utilities/StaticString.h>
#include <syscallError.h>
#include <machine/Timer.h>
#include <machine/Machine.h>
#include <process/Process.h>
#include <users/UserManager.h>

Ext2Filesystem::Ext2Filesystem() :
  m_pDisk(0), m_Superblock(), m_pGroupDescriptors(0), m_BlockSize(0), m_WriteLock(false),
  m_bSuperblockDirty(false), m_bGroupDescriptorsDirty(false), m_pRoot(0)
{
}

Ext2Filesystem::~Ext2Filesystem()
{
  delete [] m_pGroupDescriptors;
}

bool Ext2Filesystem::initialise(Disk *pDisk)
{
  m_pDisk = pDisk;

  // Attempt to read the superblock.
  uint8_t buffer[512];
  m_pDisk->read(1024, 512,
                reinterpret_cast<uintptr_t> (buffer));

  memcpy(reinterpret_cast<void*> (&m_Superblock), reinterpret_cast<void*> (buffer), sizeof(Superblock));

  // Read correctly?
  if (LITTLE_TO_HOST16(m_Superblock.s_magic) != 0xEF53)
  {
    String devName;
    pDisk->getName(devName);
    ERROR("Ext2: Superblock not found on device " << devName);
    return false;
  }

  // Calculate the block size.
  m_BlockSize = 1024 << LITTLE_TO_HOST32(m_Superblock.s_log_block_size);

  // Where is the group descriptor table?
  /// \todo Logic needs improvement - surely there's a specific way to calculate this?
  uint32_t gdBlock = 1;
  if (m_BlockSize == 1024)
    gdBlock = 2;

  // How large is the group descriptor table?
  uint32_t gdSize = m_BlockSize;
  while (gdSize <= (( LITTLE_TO_HOST32(m_Superblock.s_inodes_count) / LITTLE_TO_HOST32(m_Superblock.s_inodes_per_group) )
                    * sizeof(GroupDesc) ))
    gdSize += m_BlockSize;

  // Load the group descriptor table.
  uint8_t *tmp = new uint8_t[gdSize];
  m_pDisk->read(m_BlockSize*gdBlock, gdSize,
                reinterpret_cast<uintptr_t> (tmp));
  m_pGroupDescriptors = reinterpret_cast<GroupDesc*> (tmp);

  return true;
}

Filesystem *Ext2Filesystem::probe(Disk *pDisk)
{
  Ext2Filesystem *pFs = new Ext2Filesystem();
  if (!pFs->initialise(pDisk))
  {
    delete pFs;
    return 0;
  }
  else
    return pFs;
}

File* Ext2Filesystem::getRoot()
{
  if (!m_pRoot)
  {
    Inode inode = getInode(EXT2_ROOT_INO);
    m_pRoot = new Ext2Directory(String(""), EXT2_ROOT_INO, inode, this, 0);
  }
  return m_pRoot;
}

String Ext2Filesystem::getVolumeLabel()
{
  if (m_Superblock.s_volume_name[0] == '\0')
  {
    NormalStaticString str;
    str += "no-volume-label@";
    str.append(reinterpret_cast<uintptr_t>(this), 16);
    return String(static_cast<const char*>(str));
  }
  char buffer[17];
  strncpy(buffer, m_Superblock.s_volume_name, 16);
  buffer[16] = '\0';
  return String(buffer);
}

void Ext2Filesystem::fileAttributeChanged(File *pFile)
{
  Ext2File *pE2File = reinterpret_cast<Ext2File*>(pFile);
  pE2File->fileAttributeChanged();
}

uint64_t Ext2Filesystem::read(File *pFile, uint64_t location, uint64_t size, uintptr_t buffer)
{
  FATAL("EXT2: Read should not be called - Ext2File overrides it.");
  return false;
}

uint64_t Ext2Filesystem::write(File *pFile, uint64_t location, uint64_t size, uintptr_t buffer)
{
  FATAL("EXT2: Write should not be called - Ext2File overrides it.");
  return false;
}

void Ext2Filesystem::cacheDirectoryContents(File *pFile)
{
  FATAL("EXT2: cacheDirContents should not be called - Ext2File overrides it.");
}


void Ext2Filesystem::truncate(File *pFile)
{
  FATAL("EXT2: truncate should not be called - Ext2File overrides it.");
}

bool Ext2Filesystem::createNode(File* parent, String filename, uint32_t mask, String value, size_t type)
{
  // Quick sanity check;
  if (!parent->isDirectory())
  {
    SYSCALL_ERROR(NotADirectory);
    return false;
  }

  // The filename cannot be the special entries "." or "..".
  if (filename.length() == 0 || !strcmp(filename, ".") || !strcmp(filename, ".."))
  {
    SYSCALL_ERROR(InvalidArgument);
    return false;
  }

  // Find a free inode.
  uint32_t inode_num = findFreeInode();
  if (inode_num == 0)
  {
    SYSCALL_ERROR(NoSpaceLeftOnDevice);
    return false;
  }

  Timer *pTimer = Machine::instance().getTimer();

  // Populate the inode.
  /// \todo Endianness!
  Inode newInode;
  memset(reinterpret_cast<uint8_t*>(&newInode), 0, sizeof(Inode));
  newInode.i_mode = mask | type;
  newInode.i_uid = Processor::information().getCurrentThread()->getParent()->getUser()->getId();
  newInode.i_atime = newInode.i_ctime = newInode.i_mtime = pTimer->getUnixTimestamp();
  newInode.i_gid = Processor::information().getCurrentThread()->getParent()->getGroup()->getId();
  newInode.i_links_count = 1;

  if (value.length() && value.length() < 4*15)
  {
    memcpy(reinterpret_cast<void*>(newInode.i_block), value, value.length());
    newInode.i_size = value.length();
  }
  // Else case comes later, after pFile is created.

  // Write the inode data back to disk.
  if (!setInode(inode_num, newInode))
  {
    ERROR("EXT2: Internal error setting inode number.");
    SYSCALL_ERROR(IoError);
    // We could release the inode in the bitmap, but that would probably fail too,
    // so just leave it up to the defragmenter.
    return false;
  }

  // Create the new File object.
  File *pFile = 0;
  switch (type)
  {
    case EXT2_S_IFREG:
      pFile = new Ext2File(filename, inode_num, newInode, this, parent);
      break;
    case EXT2_S_IFDIR:
      pFile = new Ext2Directory(filename, inode_num, newInode, this, parent);
      break;
    case EXT2_S_IFLNK:
      pFile = new Ext2Symlink(filename, inode_num, newInode, this, parent);
      break;
    default:
      FATAL("EXT2: Unrecognised file type: " << Hex << type);
      break;
  }

  // Else case from earlier.
  if (value.length() && value.length() >= 4*15)
  {
    const char *pStr = value;
    pFile->write(0ULL, value.length(), reinterpret_cast<uintptr_t>(pStr));
  }

  // Add to the parent directory.
  Ext2Directory *pE2Parent = reinterpret_cast<Ext2Directory*>(parent);
  if (!pE2Parent->addEntry(filename, pFile, type))
  {
    ERROR("EXT2: Internal error adding directory entry.");
    SYSCALL_ERROR(IoError);
    return false;
  }

  // Edit the atime and mtime of the parent directory.
  parent->setAccessedTime(pTimer->getUnixTimestamp());
  parent->setModifiedTime(pTimer->getUnixTimestamp());

  return true;
}

bool Ext2Filesystem::createFile(File *parent, String filename, size_t mask)
{
  return createNode(parent, filename, mask, String(""), EXT2_S_IFREG);
}

bool Ext2Filesystem::createDirectory(File* parent, String filename)
{
  return createNode(parent, filename, 0, String(""), EXT2_S_IFDIR);
}

bool Ext2Filesystem::createSymlink(File* parent, String filename, String value)
{
  return createNode(parent, filename, 0, value, EXT2_S_IFLNK);
}

bool Ext2Filesystem::remove(File* parent, File* file)
{
}

bool Ext2Filesystem::readBlock(uint32_t block, uintptr_t buffer)
{
  if (block == 0)
  {
    // Sparse file.
    memset(reinterpret_cast<uint8_t*>(buffer), 0, m_BlockSize);
  }
  else
  {
    m_pDisk->read(static_cast<uint64_t>(m_BlockSize)*static_cast<uint64_t>(block),
                  m_BlockSize,
                  buffer);
  }
  return true;
}

bool Ext2Filesystem::writeBlock(uint32_t block, uintptr_t buffer)
{
  if (block == 0)
  {
    // Sparse file - Error!
    FATAL("EXT2: Attempting to write to sparse block!");
    return false;
  }

  m_pDisk->write(static_cast<uint64_t>(m_BlockSize)*static_cast<uint64_t>(block),
                 m_BlockSize,
                 buffer);
  return true;
}

uint32_t Ext2Filesystem::findFreeBlock(uint32_t inode)
{
  inode--; // Inode zero is undefined, so it's not used.

  uint32_t group = inode / LITTLE_TO_HOST32(m_Superblock.s_inodes_per_group);

  uint32_t nGroups = m_Superblock.s_inodes_count / LITTLE_TO_HOST32(m_Superblock.s_inodes_per_group);

  uint8_t *buffer = new uint8_t[m_BlockSize];
  while (group < nGroups)
  {
    uint64_t bitmapBlock = LITTLE_TO_HOST32(m_pGroupDescriptors[group].bg_block_bitmap);
    readBlock(bitmapBlock, reinterpret_cast<uintptr_t> (buffer));

    for (size_t i = 0; i < m_Superblock.s_blocks_per_group; i += sizeof(uint32_t))
    {
      // We can compare in 4-byte increments, to reduce the time spent looking for a block.
      if (* reinterpret_cast<uint32_t*>(&buffer[i]) == static_cast<uint32_t>(~0))
        continue;

      for (size_t j = 0; j < 4; j++)
      {
        uint8_t c = buffer[i+j];
        for (size_t k = 0; k < 8; k++)
        {
          if ( (c&0x1) == 0x0 )
          {
            // Unused, we can use this block!
            // Set it as used.
            buffer[i+j] = buffer[i+j] | (1<<k);
            m_pDisk->write(static_cast<uint64_t>(m_BlockSize)*bitmapBlock, m_BlockSize,
                           reinterpret_cast<uintptr_t> (buffer));
            /// \todo Update the group descriptor's free_blocks_count
            delete [] buffer;
            return (group*m_Superblock.s_blocks_per_group) + i*32 +
                                                             j*8 +
                                                             k;
          }
          c >>= 1;
        }
      }

      // Shouldn't get here - if there were no available blocks here it should
      // have hit the "continue" above!
      ERROR("EXT2: findFreeBlockInBitmap: Algorithm search error!");
    }
    group ++;
  }
  delete [] buffer;

  return 0;
}

uint32_t Ext2Filesystem::findFreeInode()
{
  uint32_t group = 0;
  uint32_t nGroups = LITTLE_TO_HOST32(m_Superblock.s_inodes_count) /
    LITTLE_TO_HOST32(m_Superblock.s_inodes_per_group);

  uint8_t *buffer = new uint8_t[m_BlockSize];
  while (group < nGroups)
  {
    uint64_t inodeBlock = LITTLE_TO_HOST32(m_pGroupDescriptors[group].bg_inode_bitmap);
    readBlock(inodeBlock, reinterpret_cast<uintptr_t> (buffer));

    for (size_t i = 0; i < m_Superblock.s_inodes_per_group; i += sizeof(uint32_t))
    {
      // We can compare in 4-byte increments, to reduce the time spent looking for a block.
      if (* reinterpret_cast<uint32_t*>(&buffer[i]) == static_cast<uint32_t>(~0))
        continue;

      for (size_t j = 0; j < 4; j++)
      {
        uint8_t c = buffer[i+j];
        for (size_t k = 0; k < 8; k++)
        {
          if ( (c&0x1) == 0x0 )
          {
            // Unused, we can use this block!
            // Set it as used.
            buffer[i+j] = buffer[i+j] | (1<<k);
            m_pDisk->write(static_cast<uint64_t>(m_BlockSize)*inodeBlock, m_BlockSize,
                           reinterpret_cast<uintptr_t> (buffer));
            /// \todo Update the group descriptor's free_inodes_count
            delete [] buffer;
            return (group*m_Superblock.s_inodes_per_group) + i*32 +
                                                             j*8 +
                                                             k;
          }
          c >>= 1;
        }
      }

      // Shouldn't get here - if there were no available blocks here it should
      // have hit the "continue" above!
      ERROR("EXT2: findFreeInodeInBitmap: Algorithm search error!");
    }
    group ++;
  }
  delete [] buffer;

  return 0;
}

void Ext2Filesystem::releaseBlock(uint32_t block)
{
}

Inode Ext2Filesystem::getInode(uint32_t inode)
{
  inode--; // Inode zero is undefined, so it's not used.

  uint32_t index = inode % LITTLE_TO_HOST32(m_Superblock.s_inodes_per_group);
  uint32_t group = inode / LITTLE_TO_HOST32(m_Superblock.s_inodes_per_group);

  uint64_t inodeTableBlock = LITTLE_TO_HOST32(m_pGroupDescriptors[group].bg_inode_table);

  // The inode table may extend onto multiple blocks - if the inode we want is in another block,
  // calculate that now.
  while ( (index*sizeof(Inode)) >= m_BlockSize )
  {
    index -= m_BlockSize/sizeof(Inode);
    inodeTableBlock++;
  }

  uint8_t *buffer = new uint8_t[m_BlockSize];
  m_pDisk->read(static_cast<uint64_t>(m_BlockSize)*inodeTableBlock, m_BlockSize,
                reinterpret_cast<uintptr_t> (buffer));

  Inode *inodeTable = reinterpret_cast<Inode*> (buffer);

  Inode node = inodeTable[index];
  delete [] buffer;

  return node;
}

bool Ext2Filesystem::setInode(uint32_t inode, Inode in)
{
  inode--; // Inode zero is undefined, so it's not used.

  uint32_t index = inode % LITTLE_TO_HOST32(m_Superblock.s_inodes_per_group);
  uint32_t group = inode / LITTLE_TO_HOST32(m_Superblock.s_inodes_per_group);

  uint64_t inodeTableBlock = LITTLE_TO_HOST32(m_pGroupDescriptors[group].bg_inode_table);

  // The inode table may extend onto multiple blocks - if the inode we want is in another block,
  // calculate that now.
  while ( (index*sizeof(Inode)) >= m_BlockSize )
  {
    index -= m_BlockSize/sizeof(Inode);
    inodeTableBlock++;
  }

  uint8_t *buffer = new uint8_t[m_BlockSize];
  m_pDisk->read(static_cast<uint64_t>(m_BlockSize)*inodeTableBlock, m_BlockSize,
                reinterpret_cast<uintptr_t> (buffer));

  Inode *inodeTable = reinterpret_cast<Inode*> (buffer);

  inodeTable[index] = in;

  m_pDisk->write(static_cast<uint64_t>(m_BlockSize)*inodeTableBlock, m_BlockSize,
                 reinterpret_cast<uintptr_t> (buffer));

  delete [] buffer;

  return true;
}

void initExt2()
{
  VFS::instance().addProbeCallback(&Ext2Filesystem::probe);
}

void destroyExt2()
{
}

MODULE_NAME("ext2");
MODULE_ENTRY(&initExt2);
MODULE_EXIT(&destroyExt2);
MODULE_DEPENDS("VFS");
