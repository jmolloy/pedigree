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

#include "FatFilesystem.h"
#include <vfs/VFS.h>
#include <Module.h>
#include <utilities/utility.h>
#include <Log.h>
#include <utilities/List.h>
#include <processor/Processor.h>
#include <utilities/StaticString.h>
#include <syscallError.h>

// helper functions

bool isPowerOf2(uint32_t n)
{
   uint8_t log;

   for(log = 0; log < 16; log++)
   {
      if(n & 1)
      {
         n >>= 1;
         return (n != 0) ? false : true;
      }
      n >>= 1;
   }
   return false;
}

FatFilesystem::FatFilesystem() :
  m_pDisk(0), m_Superblock(), m_Superblock16(), m_Superblock32(), m_Type(FAT12), m_DataAreaStart(0), m_RootDirCount(0), m_RootDir(), m_BlockSize(0), m_pFatCache(0)
{
}

FatFilesystem::~FatFilesystem()
{
}

bool FatFilesystem::initialise(Disk *pDisk)
{
  m_pDisk = pDisk;

  // Attempt to read the superblock.
  uint8_t buffer[512];
  m_pDisk->read(0, 512,
                reinterpret_cast<uintptr_t> (buffer));

  memcpy(reinterpret_cast<void*> (&m_Superblock), reinterpret_cast<void*> (buffer), sizeof(Superblock));

  /** Validate the BPB and check for FAT FS */

  /* check for EITHER a near jmp, or a jmp and a nop */
  if (m_Superblock.BS_jmpBoot[0] != 0xE9)
  {
    if (!(m_Superblock.BS_jmpBoot[0] == 0xEB && m_Superblock.BS_jmpBoot[2] == 0x90))
    {
      String devName;
      pDisk->getName(devName);
      ERROR("FAT: Superblock not found on device " << devName);
      return false;
    }
  }

  /** Check the FAT FS itself, ensuring it's valid */

  // SecPerClus must be a power of 2
  if (!isPowerOf2(m_Superblock.BPB_SecPerClus))
  {
    ERROR("FAT: SecPerClus not a power of 2 (" << m_Superblock.BPB_SecPerClus << ")");
    return false;
  }

  // and there must be at least 1 FAT, and at most 2
  if (m_Superblock.BPB_NumFATs < 1 || m_Superblock.BPB_NumFATs > 2)
  {
    ERROR("FAT: Too many (or too few) FATs (" << m_Superblock.BPB_NumFATs << ")");
    return false;
  }

  /** Start loading actual FS info */

  // load the 12/16/32 additional info structures (only one is actually VALID, but both are loaded nonetheless)
  memcpy(reinterpret_cast<void*> (&m_Superblock16), reinterpret_cast<void*> (buffer + 36), sizeof(Superblock16));
  memcpy(reinterpret_cast<void*> (&m_Superblock32), reinterpret_cast<void*> (buffer + 36), sizeof(Superblock32));

  // number of root directory sectors
  if (!m_Superblock.BPB_BytsPerSec) // we sanity check the value, because we divide by this later
  {
    ERROR("FAT: BytsPerSec is zero!");
    return false;
  }
  uint32_t rootDirSectors = ((m_Superblock.BPB_RootEntCnt * 32) + (m_Superblock.BPB_BytsPerSec - 1)) / m_Superblock.BPB_BytsPerSec;

  // determine the size of the FAT
  uint32_t fatSz = (m_Superblock.BPB_FATSz16) ? m_Superblock.BPB_FATSz16 : m_Superblock32.BPB_FATSz32;

  // find the first data sector
  uint32_t firstDataSector = m_Superblock.BPB_RsvdSecCnt + (m_Superblock.BPB_NumFATs * fatSz) + rootDirSectors;

  // determine the number of data sectors, so we can determine FAT type
  uint32_t totDataSec = 0, totSec = (m_Superblock.BPB_TotSec16) ? m_Superblock.BPB_TotSec16 : m_Superblock.BPB_TotSec32;
  totDataSec = totSec - firstDataSector;

  if (!m_Superblock.BPB_SecPerClus) // again, sanity checking due to division by this
  {
    ERROR("FAT: SecPerClus is zero!");
    return false;
  }
  uint32_t clusterCount = totDataSec / m_Superblock.BPB_SecPerClus;

  // TODO: magic numbers here, perhaps #define MAXCLUS_{12|16|32} would work better for readability
  if (clusterCount < 4085)
  {
    m_Type = FAT12;
  }
  else if (clusterCount < 65525)
  {
    m_Type = FAT16;
  }
  else
  {
    m_Type = FAT32;
  }

  switch(m_Type)
  {
    case FAT12:
    case FAT16:

      m_RootDir.sector = m_Superblock.BPB_RsvdSecCnt + (m_Superblock.BPB_NumFATs * fatSz);

      break;

    case FAT32:

      m_RootDir.cluster = m_Superblock32.BPB_RootClus;

      break;
  }

  // fill the filesystem data
  m_DataAreaStart = firstDataSector;
  m_RootDirCount = rootDirSectors;
  m_BlockSize = m_Superblock.BPB_SecPerClus * m_Superblock.BPB_BytsPerSec;

  /// \todo Read in the FAT32 FSInfo structure

  // determine the size of the FAT
  fatSz = (m_Superblock.BPB_FATSz16) ? m_Superblock.BPB_FATSz16 : m_Superblock32.BPB_FATSz32;
  fatSz *= m_Superblock.BPB_BytsPerSec;

  // read the FAT into cache
  uint32_t fatSector = m_Superblock.BPB_RsvdSecCnt;
  m_pFatCache = new uint8_t[fatSz];

  uint8_t* tmpBuffer = new uint8_t[fatSz];
  readSectorBlock(fatSector, fatSz, reinterpret_cast<uintptr_t>(tmpBuffer));

  memcpy(reinterpret_cast<void*>(m_pFatCache+0), reinterpret_cast<void*>(tmpBuffer), fatSz);

  delete tmpBuffer;

  return true;
}

Filesystem *FatFilesystem::probe(Disk *pDisk)
{
  FatFilesystem *pFs = new FatFilesystem();
  if (!pFs->initialise(pDisk))
  {
    delete pFs;
    return 0;
  }
  else
  {
    return pFs;
  }
}

File FatFilesystem::getRoot()
{
  // needs to return a file referring to the root directory

  uint32_t cluster = 0;
  if(m_Type == FAT32)
    cluster = m_RootDir.cluster;

  return File(String(""), 0, 0, 0, cluster, false, true, this, 0);
}

String FatFilesystem::getVolumeLabel()
{
  // The root directory (typically) contains the volume label, with a specific flag
  // In my experience, it's always the first entry, and it's always there. Even so,
  // we want to cater to unusual formats.
  //
  // In order to do so we check the entire root directory.

  uint8_t* buffer = 0;

  uint32_t sz = m_BlockSize;

  uint32_t clus = 0;
  if(m_Type == FAT32)
    clus = m_RootDir.cluster;

  String volid;

  // allocate buffers
  if(clus == 0)
  {
    // FAT12/16: read in the entire root directory

    uint32_t sec = m_RootDir.sector;

    sz = m_RootDirCount * m_Superblock.BPB_BytsPerSec;

    buffer = new uint8_t[sz];
    readSectorBlock(sec, sz, reinterpret_cast<uintptr_t>(buffer));
  }
  else
  {
    // Read in the first cluster of the directory
    buffer = new uint8_t[m_BlockSize];
    readCluster(clus, reinterpret_cast<uintptr_t> (buffer));
  }

  size_t i;
  bool endOfDir = false;
  while(true)
  {

    for(i = 0; i < sz; i += sizeof(Dir))
    {
      Dir* ent = reinterpret_cast<Dir*>(&buffer[i]);

      if(ent->DIR_Name[0] == 0)
      {
        endOfDir = true;
        break;
      }

      if(ent->DIR_Attr & ATTR_VOLUME_ID)
      {
        volid =  convertFilenameFrom(String(reinterpret_cast<const char*>(ent->DIR_Name)));
        delete buffer;
        return volid;
      }
    }

    if(endOfDir)
      break;

    if(clus == 0 && m_Type != FAT32)
      break; // not found

    // find the next cluster in the chain, if this is the end, break, if not, continue
    clus = getClusterEntry(clus);
    if(clus == 0)
      break; // something broke!

    if(isEof(clus))
      break;

    // continue by reading in this cluster
    readCluster(clus, reinterpret_cast<uintptr_t> (buffer));

  }

  delete buffer;

  // none found, do a default
  NormalStaticString str;
  str += "no-volume-label@";
  str.append(reinterpret_cast<uintptr_t>(this), 16);
  return String(static_cast<const char*>(str));
}

/////////////////////////////////////////////////////////////////////////////

uint64_t FatFilesystem::read(File *pFile, uint64_t location, uint64_t size, uintptr_t buffer)
{

  // Sanity check.
  if (pFile->isDirectory())
    return 0;

  // the inode of the file is the first cluster
  uint32_t clus = pFile->getInode();
  if(clus == 0)
  {
    return 0; // can't do it
  }

  // validity checking
  if(static_cast<size_t>(location) > pFile->getSize())
  {
    ERROR("FAT: Attempt to read on file " << pFile->getName() << " with offset past EOF.");
    return 0;
  }

  uint64_t endOffset = location + size;
  uint64_t finalSize = size;
  if(static_cast<size_t>(endOffset) > pFile->getSize())
  {
    WARNING("FAT: offset + size is larger than the file! Offset = " << location << ", size = " << size << ".");
    finalSize = pFile->getSize() - location;
  }

  // finalSize holds the total amount of data to read, now find the cluster and sector offsets
  uint32_t clusOffset = location / (m_Superblock.BPB_SecPerClus * m_Superblock.BPB_BytsPerSec);
  uint32_t firstOffset = location % (m_Superblock.BPB_SecPerClus * m_Superblock.BPB_BytsPerSec); // the offset within the cluster specified above to start reading from

  // tracking info

  uint64_t bytesRead = 0;
  uint64_t currOffset = firstOffset;
  while(clusOffset)
  {
    clus = getClusterEntry(clus);
    if(clus == 0 || isEof(clus))
      return 0; // can't do it
    clusOffset--;
  }

  // buffers
  uint8_t* tmpBuffer = new uint8_t[m_BlockSize];
  uint8_t* destBuffer = reinterpret_cast<uint8_t*>(buffer);

  // main read loop
  while(true)
  {
    // read in the entire cluster
    readCluster(clus, reinterpret_cast<uintptr_t> (tmpBuffer));

    // read...
    while(currOffset < m_BlockSize)
    {
      destBuffer[bytesRead] = tmpBuffer[currOffset];
      currOffset++; bytesRead++;

      // if at any time we're done, end reading
      // TODO: do we need a null byte?
      if(bytesRead == finalSize)
      {
        delete tmpBuffer;
        return bytesRead;
      }
    }

    // end of cluster, set the offset back to zero
    currOffset = 0;

    // grab the next cluster, check for EOF
    clus = getClusterEntry(clus);
    if(clus == 0)
      break; // something broke!

    if(isEof(clus))
      break;
  }

  delete tmpBuffer;

  // if we reach here, something's gone wrong
  return 0;
}

/////////////////////////////////////////////////////////////////////////////

uint32_t FatFilesystem::findFreeCluster()
{
  int j;
  uint32_t clus;
  for(j = (m_Type == FAT32 ? 2 /** \todo FSInfo structure */ : 2); j < (m_Superblock.BPB_TotSec32 / m_Superblock.BPB_SecPerClus); j++)
  {
    clus = getClusterEntry(j);
    if((clus & 0x0FFFFFFF) == 0)
    {
      /// \todo For FAT32, update the FSInfo structure
      return clus;
    }
  }
  return 0;
}

uint64_t FatFilesystem::write(File *pFile, uint64_t location, uint64_t size, uintptr_t buffer)
{
  // test whether the entire Filesystem is read-only.
  if(m_bReadOnly)
  {
    SYSCALL_ERROR(ReadOnlyFilesystem);
    return 0;
  }

  uint32_t firstClus = pFile->getInode();
  
  if(firstClus == 0)
  {
    // find a free cluser
    uint32_t freeClus = findFreeCluster();
    if(freeClus == 0)
    {
      //SYSCALL_ERROR(FilesystemFull);
      return 0;
    }
    
    // set EOF
    if(m_Type == FAT12)
      setClusterEntry(freeClus, 0x0FF8);
    else if(m_Type == FAT16)
      setClusterEntry(freeClus, 0xFFF8);
    else if(m_Type == FAT32)
      setClusterEntry(freeClus, 0x0FFFFFF8); 
    firstClus = freeClus;
    
    /// \todo Write the updated cluster into the directory entry
    // pFile->setInode(freeClus);
  }
  
  uint32_t clusSize = m_Superblock.BPB_SecPerClus * m_Superblock.BPB_BytsPerSec;
  uint32_t finalOffset = location + size;
  uint32_t offsetSector = location / m_Superblock.BPB_BytsPerSec;
  uint32_t finalSector = finalOffset / m_Superblock.BPB_BytsPerSec;
  uint32_t clus = 0;
  
  uint32_t realSector = ((firstClus - 2) * m_Superblock.BPB_SecPerClus) + m_DataAreaStart;
  
  // does the file currently have enough clusters to allow us to write without stopping?
  int i = clusSize;
  int j = pFile->getSize() / i;
  if(pFile->getSize() % i)
    j++; // extra cluster (integer division)
  if(j == 0)
    j = 1; // always one cluster
  
  uint32_t finalCluster = j * i;
  uint32_t numExtraBytes = 0;
  
  // if the final offset is past what we already have in the cluster chain, fill in the blanks
  if(finalOffset > finalCluster)
  {
    WARNING("Adding new clusters to chain");
    
    numExtraBytes = finalOffset - finalCluster;
    
    j = numExtraBytes / i;
    if(numExtraBytes % i)
      j++;
    
    uint32_t lastClus = 0;
    clus = getClusterEntry(firstClus);
    while(!isEof(clus))
    {
      lastClus = clus;
      clus = getClusterEntry(clus);
    }

    uint32_t prev;
    for(i = 0; i < j; i++)
    {
      prev = lastClus;
      lastClus = findFreeCluster();
                    /// \todo Fix this
      
      setClusterEntry(prev, lastClus);
    }
    
    if(m_Type == FAT12)
      setClusterEntry(lastClus, 0x0FF8);
    else if(m_Type == FAT16)
      setClusterEntry(lastClus, 0xFFF8);
    else if(m_Type == FAT32)
      setClusterEntry(lastClus, 0x0FFFFFF8); 
  }

  uint64_t endOffset = finalOffset;
  uint64_t finalSize = size;

  // finalSize holds the total amount of data to read, now find the cluster and sector offsets
  uint32_t clusOffset = location / (m_Superblock.BPB_SecPerClus * m_Superblock.BPB_BytsPerSec);
  uint32_t firstOffset = location % (m_Superblock.BPB_SecPerClus * m_Superblock.BPB_BytsPerSec); // the offset within the cluster specified above to start reading from

  // tracking info

  uint64_t bytesWritten = 0;
  uint64_t currOffset = firstOffset;
  while(clusOffset)
  {
    clus = getClusterEntry(clus);
    if(clus == 0 || isEof(clus))
      return 0; // can't do it
    clusOffset--;
  }

  // buffers
  uint8_t* tmpBuffer = new uint8_t[m_BlockSize];
  uint8_t* srcBuffer = reinterpret_cast<uint8_t*>(buffer);

  /// \todo We need to update the directory which contains this file, to ensure that it's got the new size/cluster information
  
  // main read loop
  while(true)
  {
    // read in the entire cluster
    readCluster(clus, reinterpret_cast<uintptr_t> (tmpBuffer));

    // read...
    while(currOffset < m_BlockSize)
    {
      tmpBuffer[bytesWritten] = srcBuffer[currOffset];
      currOffset++; bytesWritten++;

      if(bytesWritten == finalSize)
      {
        writeCluster(clus, reinterpret_cast<uintptr_t> (tmpBuffer));
        delete tmpBuffer;
        return bytesWritten;
      }
    }
    
    writeCluster(clus, reinterpret_cast<uintptr_t> (tmpBuffer));

    // end of cluster, set the offset back to zero
    currOffset = 0;

    // grab the next cluster, check for EOF
    clus = getClusterEntry(clus);
    if(clus == 0)
      break; // something broke!

    if(isEof(clus))
      break;
  }

  delete tmpBuffer;

  return bytesWritten;
}

void FatFilesystem::fileAttributeChanged(File *pFile)
{
}

File FatFilesystem::getDirectoryChild(File *pFile, size_t n)
{
  // Sanity check.
  if (!pFile->isDirectory())
    return File();

  // Grab the Inode of the file (for FAT this is the cluster for the directory)
  uint8_t *buffer = 0;
  uint32_t sz = m_BlockSize;
  uint32_t clus = pFile->getInode();
  if(clus == 0)
  {
    if(m_Type == FAT32)
      return File();

    // FAT12/16: read in the entire root directory, because clus == 0 (which would give an invalid sector)

    // hack to make lack of root directory containing "." and ".." entries invisible
    if(n == 1)
      return File(String("."), 0, 0, 0, 0, false, true, this, 0);
    if(n == 2)
      return File(String(".."), 0, 0, 0, 0, false, true, this, 0);
    n += 2;

    uint32_t sec = m_RootDir.sector;

    sz = m_RootDirCount * m_Superblock.BPB_BytsPerSec;

    buffer = new uint8_t[sz];
    readSectorBlock(sec, sz, reinterpret_cast<uintptr_t>(buffer));
  }
  else
  {
    // root directory?
    if(clus == m_Superblock32.BPB_RootClus)
    {
      // hack to make lack of root directory containing "." and ".." entries invisible
      if(n == 1)
        return File(String("."), 0, 0, 0, 0, false, true, this, 0);
      if(n == 2)
        return File(String(".."), 0, 0, 0, 0, false, true, this, 0);
      n += 2;
    }

    // Read in the first cluster of the directory
    buffer = new uint8_t[m_BlockSize];
    readCluster(clus, reinterpret_cast<uintptr_t> (buffer));
  }

  // moved this out of the main loop in case of a long filename set crossing a cluster
  // boundary
  NormalStaticString longFileName;
  longFileName.clear();
  int32_t longFileNameIndex = 0;
  bool nextIsEnd = false; // next entry is the short filename entry for this long filename

  // was initially separated for FAT12/16 and FAT32, but I hate redundancy (if I can figure out how to make
  // this code work for the Volume Label as well, I'll be a very happy man)
  size_t i, j;
  bool endOfDir = false;
  while(true)
  {

    for(i = 0, j = 0; i < sz; i += sizeof(Dir), j++)
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

        j--; // long filename entries don't count in the list

        continue;
      }

      if(j == n)
      {
        uint8_t attr = ent->DIR_Attr;
        uint32_t fileCluster = ent->DIR_FstClusLO | (ent->DIR_FstClusHI << 16);
        String filename;
        if(nextIsEnd)
          filename = static_cast<const char*>(longFileName); // use the long filename rather than the short one
        else
        {
          // WARNING("FAT: Using short filename rather than long filename");
          filename = convertFilenameFrom(String(reinterpret_cast<const char*>(ent->DIR_Name)));
        }
        File ret(filename, 0, 0, 0, fileCluster, false, (attr & ATTR_DIRECTORY) == ATTR_DIRECTORY, this, ent->DIR_FileSize);
        delete buffer;
        return ret;
      }
      else
      {
        longFileName.clear();
        nextIsEnd = false;
      }
    }

    if(endOfDir)
      break;

    if(clus == 0 && m_Type != FAT32)
      break; // not found

    // find the next cluster in the chain, if this is the end, break, if not, continue
    clus = getClusterEntry(clus);
    if(clus == 0)
      break; // something broke!

    if(isEof(clus))
      break;

    // continue by reading in this cluster
    readCluster(clus, reinterpret_cast<uintptr_t> (buffer));

  }

  // n too high?
  delete buffer;

  return File();
}

bool FatFilesystem::readCluster(uint32_t block, uintptr_t buffer)
{
  block = getSectorNumber(block);
  readSectorBlock(block, m_BlockSize, buffer);
  return true;
}

bool FatFilesystem::readSectorBlock(uint32_t sec, size_t size, uintptr_t buffer)
{
  m_pDisk->read(static_cast<uint64_t>(m_Superblock.BPB_BytsPerSec)*static_cast<uint64_t>(sec), size, buffer);
  return true;
}

bool FatFilesystem::writeCluster(uint32_t block, uintptr_t buffer)
{  
  block = getSectorNumber(block);
  writeSectorBlock(block, m_BlockSize, buffer);
  return true;
}

bool FatFilesystem::writeSectorBlock(uint32_t sec, size_t size, uintptr_t buffer)
{
  m_pDisk->write(static_cast<uint64_t>(m_Superblock.BPB_BytsPerSec)*static_cast<uint64_t>(sec), size, buffer);
  return true;
}

uint32_t FatFilesystem::getSectorNumber(uint32_t cluster)
{
  return ((cluster - 2) * m_Superblock.BPB_SecPerClus) + m_DataAreaStart;
}

uint32_t FatFilesystem::getClusterEntry(uint32_t cluster)
{
  uint32_t fatOffset = 0;
  switch(m_Type)
  {
    case FAT12:
      fatOffset = cluster + (cluster / 2);
      break;

    case FAT16:
      fatOffset = cluster * 2;
      break;

    case FAT32:
      fatOffset = cluster * 4;
      break;
  }

  // read from cache
  uint32_t fatEntry = * reinterpret_cast<uint32_t*> (&m_pFatCache[fatOffset]);

  // calculate
  uint32_t ret = 0;
  switch(m_Type)
  {
    case FAT12:
      ret = fatEntry;

      // FAT12 entries are 1.5 bytes
      if(cluster & 0x1)
        ret >>= 4;
      else
        ret &= 0x0FFF;
      ret &= 0xFFFF;

      break;

    case FAT16:

      ret = fatEntry;
      ret &= 0xFFFF;

      break;

    case FAT32:

      ret = fatEntry;

      break;
  }

  return ret;
}

uint32_t FatFilesystem::setClusterEntry(uint32_t cluster, uint32_t value)
{
  uint32_t fatOffset = 0;
  switch(m_Type)
  {
    case FAT12:
      fatOffset = cluster + (cluster / 2);
      break;

    case FAT16:
      fatOffset = cluster * 2;
      break;

    case FAT32:
      fatOffset = cluster * 4;
      break;
  }
  
  uint32_t ent = getClusterEntry(cluster);
  
  uint32_t origEnt = ent;
  uint32_t setEnt = value;
  
  // calculate and write back into the cache
  switch(m_Type)
  {
    case FAT12:
    
      if(cluster & 0x1)
        setEnt >>= 4;
      else
        setEnt &= 0x0FFF;
      setEnt &= 0xFFFF;
      
      if(cluster & 0x1)
      {
        value <<= 4;
        origEnt &= 0x000F;
      }
      else
      {
        value &= 0x0FFF;
        origEnt &= 0xF000;
      }
      
      setEnt = origEnt | value;
      
      * reinterpret_cast<uint16_t*> (&m_pFatCache[fatOffset]) = setEnt;
      
      break;
      
    case FAT16:
    
      setEnt = value;
      
      * reinterpret_cast<uint16_t*> (&m_pFatCache[fatOffset]) = setEnt;
    
      break;
    
    case FAT32:
    
      value &= 0x0FFFFFFF;
      setEnt = origEnt & 0xF0000000;
      setEnt |= value;
      
      * reinterpret_cast<uint32_t*> (&m_pFatCache[fatOffset]) = setEnt;
      
      break;
  }
  
	uint32_t fatSector = m_Superblock.BPB_RsvdSecCnt + (fatOffset / m_Superblock.BPB_BytsPerSec);
  fatOffset %= m_Superblock.BPB_BytsPerSec;
  
  // write back to the FAT
  uint8_t* tmpBuffer = new uint8_t[m_Superblock.BPB_BytsPerSec * 2]; /// \todo Safety checks
  memcpy(reinterpret_cast<void*>(&m_pFatCache[fatOffset]), reinterpret_cast<void*>(tmpBuffer), m_Superblock.BPB_BytsPerSec * 2);
  writeSectorBlock(fatSector, m_Superblock.BPB_BytsPerSec * 2, reinterpret_cast<uintptr_t>(tmpBuffer));

  return setEnt;
}

char toUpper(char c)
{
  if(c < 'a' || c > 'z')
    return c; // special chars
  c += ('A' - 'a');
  return c;
}

char toLower(char c)
{
  if(c < 'A' || c > 'Z')
    return c; // special chars
  c -= ('A' - 'a');
  return c;
}

String FatFilesystem::convertFilenameTo(String filename)
{
  NormalStaticString ret;
  size_t i;
  for(i = 0; i < 11; i++)
  {
    if(i >= filename.length())
      break;
    if(filename[i] != '.')
      ret += toUpper(filename[i]);
    else
    {
      int j;
      for(j = i; j < 8; j++)
        ret += ' ';
      i++; // skip the period
      for(j = 0; j < 3; j++)
          if((i + j) < filename.length())
            ret += toUpper(filename[i + j]);
      break;
    }
  }
  return String(static_cast<const char*>(ret));
}

String FatFilesystem::convertFilenameFrom(String filename)
{
  NormalStaticString ret;

  size_t i;
  for(i = 0; i < 8; i++)
  {
    if(i >= filename.length())
      break;
    if(filename[i] != ' ')
      ret += toLower(filename[i]);
    else
      break;
  }

  for(i = 0; i < 3; i++)
  {
    if((8 + i) >= filename.length())
      break;
    if(filename[8 + i] != ' ')
    {
      if(i == 0)
        ret += '.';
      ret += toLower(filename[8 + i]);
    }
    else
      break;
  }

  return String(static_cast<const char*>(ret));
}

bool FatFilesystem::isEof(uint32_t cluster)
{
  bool chainEnd = false;
  if(m_Type == FAT12)
    if(cluster >= 0x0FF8)
      chainEnd = true;
  if(m_Type == FAT16)
    if(cluster >= 0xFFF8)
      chainEnd = true;
  if(m_Type == FAT32)
    if(cluster >= 0x0FFFFFF8)
      chainEnd = true;
  return chainEnd;
}


void FatFilesystem::truncate(File *pFile)
{
}

bool FatFilesystem::createFile(File parent, String filename)
{
  return false;
}

bool FatFilesystem::createDirectory(File parent, String filename)
{
  return false;
}

bool FatFilesystem::createSymlink(File parent, String filename, String value)
{
  return false;
}

bool FatFilesystem::remove(File parent, File file)
{
  return false;
}

void initFat()
{
  VFS::instance().addProbeCallback(&FatFilesystem::probe);
}

void destroyFat()
{
}

MODULE_NAME("fat");
MODULE_ENTRY(&initFat);
MODULE_EXIT(&destroyFat);
MODULE_DEPENDS("VFS");

