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

#include "FatDirectory.h"
#include "FatFilesystem.h"
#include <syscallError.h>
#include "FatFile.h"

#include "fat.h"

FatDirectory::FatDirectory(String name, uintptr_t inode_num,
                             FatFilesystem *pFs, File *pParent) :
  Directory(name, LITTLE_TO_HOST32(0), /// \todo Write some FAT-style thing here
            LITTLE_TO_HOST32(0),
            LITTLE_TO_HOST32(0),
            inode_num,
            static_cast<Filesystem*>(pFs),
            LITTLE_TO_HOST32(0), /// \todo Hmm...
            pParent),
  m_Type(FAT16), m_BlockSize(0), m_bRootDir(false), m_DirBlockSize(0)
{
  /*uint32_t mode = LITTLE_TO_HOST32(inode.i_mode);
  uint32_t permissions;
  if (mode & EXT2_S_IRUSR) permissions |= FILE_UR;
  if (mode & EXT2_S_IWUSR) permissions |= FILE_UW;
  if (mode & EXT2_S_IXUSR) permissions |= FILE_UX;
  if (mode & EXT2_S_IRGRP) permissions |= FILE_GR;
  if (mode & EXT2_S_IWGRP) permissions |= FILE_GW;
  if (mode & EXT2_S_IXGRP) permissions |= FILE_GX;
  if (mode & EXT2_S_IROTH) permissions |= FILE_OR;
  if (mode & EXT2_S_IWOTH) permissions |= FILE_OW;
  if (mode & EXT2_S_IXOTH) permissions |= FILE_OX;*/
  
  uint32_t permissions = 0777; /// \todo Permissions

  setPermissions(permissions);
  setUid(LITTLE_TO_HOST16(0)); /// \todo Ownership of files
  setGid(LITTLE_TO_HOST16(0));
  
  m_BlockSize = pFs->m_BlockSize;
  m_Type = pFs->m_Type;
  uintptr_t clus = inode_num;
  
  m_DirBlockSize = m_BlockSize;
  
  m_bRootDir = false;
  if(clus == 0 && m_Type != FAT32)
  {
    m_DirBlockSize = pFs->m_RootDirCount * pFs->m_Superblock.BPB_BytsPerSec;
    m_bRootDir = true;
  }
  else
    if(pFs->m_Type == FAT32)
      if(clus == pFs->m_Superblock32.BPB_RootClus)
        m_bRootDir = true;
}

FatDirectory::~FatDirectory()
{
}

bool FatDirectory::addEntry(String filename, File *pFile, size_t type)
{

  // We're all good - add the directory to our cache.
  m_Cache.insert(filename, pFile);

  m_Size = 0;
  return true;
}

bool FatDirectory::removeEntry(File *pFile)
{
  m_Size = 0;
  return false;
}

void FatDirectory::cacheDirectoryContents()
{
  NOTICE("FatDirectory::cacheDirectoryContents() - inode = " << m_Inode << " [size=" << m_DirBlockSize << ", root dir = " << m_bRootDir << "].");
  
  FatFilesystem *pFs = reinterpret_cast<FatFilesystem *>(m_pFilesystem);
  
  // first check that we're not working in the root directory - if so, handle . and .. for it
  uint32_t clus = m_Inode;
  uint32_t sz = m_DirBlockSize;
  if (m_bRootDir)
  {
    m_Cache.insert(String("."), new FatDirectory(String("."), m_Inode, pFs, 0));
    m_Cache.insert(String(".."), new FatDirectory(String(".."), m_Inode, pFs, 0));
  }

  // read in the first cluster for this directory
  uint8_t* buffer = reinterpret_cast<uint8_t*>(pFs->readDirectoryPortion(clus));
  
  // moved this out of the main loop in case of a long filename set crossing a cluster boundary
  NormalStaticString longFileName;
  longFileName.clear();
  int32_t longFileNameIndex = 0;
  bool nextIsEnd = false; // next entry is the short filename entry for this long filename

  size_t i, j = 0;
  bool endOfDir = false;
  while(true)
  {
    for(i = 0; i < sz; i += sizeof(Dir))
    {
      Dir* ent = reinterpret_cast<Dir*>(&buffer[i]);

      uint8_t firstChar = ent->DIR_Name[0];
      if(firstChar == 0)
      {
        endOfDir = true;
        break;
      }

      if(firstChar == 0xE5)
      {
        // deleted file
        longFileNameIndex = 0;
        longFileName.clear();
        nextIsEnd = false;
        continue;
      }

      if((ent->DIR_Attr & ATTR_LONG_NAME_MASK) == ATTR_LONG_NAME)
      {
        if(firstChar & 0x40) // first LFN (ie, masked with 0x40 to state LAST_LONG_ENTRY
          longFileNameIndex = firstChar ^ 0x40;

        char* entBuffer = reinterpret_cast<char*>(&buffer[i]);
        char tmp[64];

        // probably a more efficient way of doing this somehow...
        int a,b=0;
        for(a = 1; a < 11; a += 2)
          tmp[b++] = entBuffer[a];
        for(a = 14; a < 26; a += 2)
          tmp[b++] = entBuffer[a];
        for(a = 28; a < 32; a += 2)
          tmp[b++]= entBuffer[a];

        tmp[b] = '\0';

        // hack to make long filenames work
        NormalStaticString latterString = longFileName;
        longFileName = tmp;
        longFileName += latterString;

        // will be zero if the last entry
        if((longFileNameIndex == 0) || (--longFileNameIndex <= 1))
        {
          nextIsEnd = true;
        }

        continue;
      }

      uint8_t attr = ent->DIR_Attr;
      if(attr != ATTR_VOLUME_ID)
      {
        uint32_t fileCluster = ent->DIR_FstClusLO | (ent->DIR_FstClusHI << 16);
        String filename;
        if(nextIsEnd)
          filename = static_cast<const char*>(longFileName); // use the long filename rather than the short one
        else
        {
          //WARNING("FAT: Using short filename rather than long filename");
          filename = pFs->convertFilenameFrom(String(reinterpret_cast<const char*>(ent->DIR_Name)));
        }
          
        Time writeTime = pFs->getUnixTimestamp(ent->DIR_WrtTime, ent->DIR_WrtDate);
        Time accTime = pFs->getUnixTimestamp(0, ent->DIR_LstAccDate);
        Time createTime = pFs->getUnixTimestamp(ent->DIR_CrtTime, ent->DIR_CrtDate);
        
        File *pF;
        if((attr & ATTR_DIRECTORY) == ATTR_DIRECTORY)
          pF = new FatDirectory(filename, fileCluster, pFs, this);
        else
        {
          pF = new FatFile(
                            filename,
                            accTime,
                            writeTime,
                            createTime,
                            fileCluster,
                            pFs,
                            ent->DIR_FileSize,
                            clus,
                            i,
                            this
          );
        }
          
        NOTICE("Cache: inserting " << filename);
        m_Cache.insert(filename, pF);
      }

      longFileNameIndex = 0;
      longFileName.clear();
      nextIsEnd = false;
      j++;
    }

    if(endOfDir)
      break;

    if(clus == 0 && m_Type != FAT32)
      break; // not found

    // find the next cluster in the chain, if this is the end, break, if not, continue
    clus = pFs->getClusterEntry(clus);
    if(clus == 0)
      break; // something broke!

    if(pFs->isEof(clus))
      break;

    // continue by reading in this cluster
    pFs->readCluster(clus, reinterpret_cast<uintptr_t>(buffer));
  }
  
  delete [] buffer;
  
  m_bCachePopulated = true;
}

void FatDirectory::fileAttributeChanged()
{
//  static_cast<*>(this)->fileAttributeChanged(m_Size, m_AccessedTime, m_ModifiedTime, m_CreationTime);
}

