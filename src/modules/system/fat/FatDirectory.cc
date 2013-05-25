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
#include <LockGuard.h>
#include "FatFile.h"

#include "fat.h"

FatDirectory::FatDirectory(String name, uintptr_t inode_num,
                            FatFilesystem *pFs, File *pParent, FatFileInfo &info,
                            uint32_t dirClus, uint32_t dirOffset) :
  Directory(name,
            LITTLE_TO_HOST32(info.accessedTime),
            LITTLE_TO_HOST32(info.modifiedTime),
            LITTLE_TO_HOST32(info.creationTime),
            inode_num,
            static_cast<Filesystem*>(pFs),
            LITTLE_TO_HOST32(0),
            pParent),
  m_DirClus(dirClus), m_DirOffset(dirOffset), m_Type(FAT16), m_BlockSize(0),
  m_bRootDir(false), m_Lock(false), m_DirBlockSize(0)
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

  setInode(inode_num);
}

FatDirectory::~FatDirectory()
{
}

void FatDirectory::setInode(uintptr_t inode)
{
  FatFilesystem *pFs = reinterpret_cast<FatFilesystem *>(m_pFilesystem);
  m_Inode = inode;
  uintptr_t clus = m_Inode;

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

bool FatDirectory::addEntry(String filename, File *pFile, size_t type)
{
  FatFilesystem *pFs = reinterpret_cast<FatFilesystem *>(m_pFilesystem);

  NOTICE("addEntry(" << filename << ")");
  LockGuard<Mutex> guard(m_Lock);

  // grab the first cluster of the parent directory
  uint32_t clus = m_Inode;
  uint8_t* buffer = reinterpret_cast<uint8_t*>(pFs->readDirectoryPortion(clus));
  PointerGuard<uint8_t> bufferGuard(buffer);
  if(!buffer)
    return false;

  // how many long filename entries does the filename require?
  size_t numRequired = 1; // need *at least* one for the short filename entry
  size_t fnLength = filename.length();

  // Dot and DotDot entries?
  if(!strncmp(static_cast<const char*>(filename), ".", filename.length()) ||
     !strncmp(static_cast<const char*>(filename), "..", filename.length()))
  {
    // Only one entry required for the dot/dotdot entries, all else get
    // a free long filename entry.
  }
  else
  {
    // each long filename entry is 13 bytes of filename
    size_t numSplit = fnLength / 13;
    numRequired += (numSplit) + 1;
  }

  size_t longFilenameOffset = fnLength;

  // find the first free element
  bool spaceFound = false;
  size_t offset;
  size_t consecutiveFree = 0;
  while(true)
  {
    // Look for space in the directory
    /// \todo Some (sets of) entries will need to cross a cluster boundary
    while(!spaceFound)
    {
      consecutiveFree = 0;
      for(offset = 0; offset < m_BlockSize; offset += sizeof(Dir))
      {
        if(buffer[offset] == 0 || buffer[offset] == 0xE5)
        {
          consecutiveFree++;
        }
        else
          consecutiveFree = 0;

        if(consecutiveFree == numRequired)
        {
          spaceFound = true;
          break;
        }
      }

      // If space was found, quit
      if(spaceFound)
        break;

      // Root Directory check:
      // If no space found for our file, and if not FAT32, the root directory is not resizeable so we have to fail
      if(m_Type != FAT32 && clus == 0)
        return false;

      // check the next cluster, add a new cluster if needed
      uint32_t prev = clus;
      clus = pFs->getClusterEntry(clus);

      if(pFs->isEof(clus))
      {
        uint32_t newClus = pFs->findFreeCluster();
        if(!newClus)
          return false;

        pFs->setClusterEntry(prev, newClus);
        pFs->setClusterEntry(newClus, pFs->eofValue());

        clus = newClus;
      }

      pFs->readCluster(clus, reinterpret_cast<uintptr_t>(buffer));
    }

    {
      // long filename entries first
      if(numRequired)
      {
        size_t currOffset = offset - ((numRequired - 1) * sizeof(Dir));
        size_t i;
        for(i = 0; i < (numRequired - 1); i++)
        {
          // grab a pointer to the data
          DirLongFilename* lfn = reinterpret_cast<DirLongFilename*>(&buffer[currOffset]);
          memset(lfn, 0xab, sizeof(DirLongFilename));

          if(i == 0)
            lfn->LDIR_Ord = 0x40 | (numRequired - 1);
          else
            lfn->LDIR_Ord = (numRequired - 1 - i);
          lfn->LDIR_Attr = ATTR_LONG_NAME;

          // get the next 13 bytes
          size_t nChars = 13;
          if(longFilenameOffset >= 13)
            longFilenameOffset -= nChars;
          else
          {
            // longFilenameOffset is not bigger than 13, so it's the number of characters to copy
            nChars = longFilenameOffset - 1;
            longFilenameOffset = 0;
          }

          size_t nOffset = longFilenameOffset;
          size_t nWritten = 0;
          size_t n;

          for(n = 0; n < 10; n += 2)
          {
            if(nWritten > nChars)
              break;
            lfn->LDIR_Name1[n] = filename[nOffset++];
            nWritten++;
          }
          for(n = 0; n < 12; n += 2)
          {
            if(nWritten > nChars)
              break;
            lfn->LDIR_Name2[n] = filename[nOffset++];
            nWritten++;
          }
          for(n = 0; n < 4; n += 2)
          {
            if(nWritten > nChars)
              break;
            lfn->LDIR_Name3[n] = filename[nOffset++];
            nWritten++;
          }

          currOffset += sizeof(Dir);
        }
      }

      // get a Dir struct for it so we can manipulate the data
      Dir* ent = reinterpret_cast<Dir*>(&buffer[offset]);
      memset(ent, 0xb0, sizeof(Dir));
      ent->DIR_Attr = type ? ATTR_DIRECTORY : 0;

      String shortFilename = pFs->convertFilenameTo(filename);
      memcpy(ent->DIR_Name, static_cast<const char*>(shortFilename), 11);
      ent->DIR_FstClusLO = pFile->getInode() & 0xFFFF;
      ent->DIR_FstClusHI = (pFile->getInode() >> 16) & 0xFFFF;

      pFs->writeDirectoryPortion(clus, buffer);

      if(type)
      {
        FatDirectory *fatDir = static_cast<FatDirectory *>(pFile);

        fatDir->setDirCluster(clus);
        fatDir->setDirOffset(offset);
      }
      else
      {
        FatFile *fatFile = static_cast<FatFile *>(pFile);

        fatFile->setDirCluster(clus);
        fatFile->setDirOffset(offset);
      }

      // If the cache is *not yet* populated, don't add the entry to the cache. This allows
      // cacheDirectoryContents to build the cache properly.
      // NOTICE("Adding " << filename << "...");
      if(m_bCachePopulated)
        m_Cache.insert(filename, pFile);

      return true;
    }
  }
  return false;
}

bool FatDirectory::removeEntry(File *pFile)
{
  FatFilesystem *pFs = static_cast<FatFilesystem *>(m_pFilesystem);
  FatDirectory *fatDir = static_cast<FatDirectory *>(pFile);
  FatFile *fatFile = static_cast<FatFile *>(pFile);
  String filename = pFile->getName();

  uint32_t dirClus, dirOffset;
  if(pFile->isDirectory())
  {
    dirClus = fatDir->getDirCluster();
    dirOffset = fatDir->getDirOffset();
  }
  else
  {
    dirClus = fatFile->getDirCluster();
    dirOffset = fatFile->getDirOffset();
  }

  LockGuard<Mutex> guard(m_Lock);

  // First byte = 0xE5 means the file's been deleted.
  Dir *dir = reinterpret_cast<Dir *>(pFs->getDirectoryEntry(dirClus, dirOffset));
  PointerGuard<Dir> dirGuard(dir);
  if(!dir)
    return false;
  dir->DIR_Name[0] = 0xE5;

  // Check that we can actually use the previous directory entry!
  if(dirOffset >= sizeof(Dir))
  {
    // The main entry is fixed, but there may be one or more long filename entries for this file...
    size_t numLfnEntries = (filename.length() / 13) + 1;

    // Grab the first entry behind this one - check that it is in fact a LFN entry
    Dir *dir_prev = reinterpret_cast<Dir *>(pFs->getDirectoryEntry(dirClus, dirOffset - sizeof(Dir)));
    PointerGuard<Dir> prevGuard(dir_prev);
    if(!dir_prev)
      return false;

    if((dir_prev->DIR_Attr & ATTR_LONG_NAME_MASK) == ATTR_LONG_NAME)
    {
      // Previous entry is a long filename entry, so delete each entry
      // This goes backwards up the list of LFN entries - if it finds an
      // entry that does *not* match a standard LFN entry it'll dump a
      // warning and break out.
      for(size_t ent = 0; ent < numLfnEntries; ent++)
      {
        uint32_t bytesBack = sizeof(Dir) * (ent + 1);
        if(bytesBack > dirOffset)
        {
          /// \todo Save the previous cluster in FatFile too
          ERROR("LFN set crosses a cluster boundary!");
          break;
        }

        uint32_t newOffset = dirOffset - bytesBack;
        Dir *lfn = reinterpret_cast<Dir *>(pFs->getDirectoryEntry(dirClus, newOffset));
        PointerGuard<Dir> lfnGuard(lfn);
        if((!lfn) || ((lfn->DIR_Attr & ATTR_LONG_NAME_MASK) != ATTR_LONG_NAME))
          break;

        lfn->DIR_Name[0] = 0xE5;

        pFs->writeDirectoryEntry(lfn, dirClus, newOffset);
      }
    }
  }

  pFs->writeDirectoryEntry(dir, dirClus, dirOffset);
  if(m_bCachePopulated)
    m_Cache.remove(filename);
  return true;
}

void FatDirectory::cacheDirectoryContents()
{
  FatFilesystem *pFs = reinterpret_cast<FatFilesystem *>(m_pFilesystem);

  LockGuard<Mutex> guard(m_Lock);

  // first check that we're not working in the root directory - if so, handle . and .. for it
  uint32_t clus = m_Inode;
  uint32_t sz = m_DirBlockSize;
  if (m_bRootDir)
  {
      NOTICE("Adding root directory");
    FatFileInfo info;
    info.creationTime = info.modifiedTime = info.accessedTime = 0;
    m_Cache.insert(String("."), new FatDirectory(String("."), m_Inode, pFs, 0, info));
    if(!m_bCachePopulated)
      m_bCachePopulated = true;
  }

  // read in the first cluster for this directory
  uint8_t* buffer = reinterpret_cast<uint8_t*>(pFs->readDirectoryPortion(clus));
  PointerGuard<uint8_t> bufferGuard(buffer);
  if(!buffer)
  {
    WARNING("FatDirectory::cacheDirectoryContents() - got a null buffer from readDirectoryPortion!");
    return;
  }

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
          // WARNING("FAT: Using short filename rather than long filename");
          filename = pFs->convertFilenameFrom(String(reinterpret_cast<const char*>(ent->DIR_Name)));
        }

        Time writeTime = pFs->getUnixTimestamp(ent->DIR_WrtTime, ent->DIR_WrtDate);
        Time accTime = pFs->getUnixTimestamp(0, ent->DIR_LstAccDate);
        Time createTime = pFs->getUnixTimestamp(ent->DIR_CrtTime, ent->DIR_CrtDate);

        FatFileInfo info;
        info.accessedTime = accTime;
        info.modifiedTime = writeTime;
        info.creationTime = createTime;

        File *pF;
        if((attr & ATTR_DIRECTORY) == ATTR_DIRECTORY)
          pF = new FatDirectory(filename, fileCluster, pFs, this, info);
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

        // NOTICE("Inserting '" << filename << "'.");
        m_Cache.insert(filename, pF);
        if(!m_bCachePopulated)
          m_bCachePopulated = true;
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
}

void FatDirectory::fileAttributeChanged()
{
}

