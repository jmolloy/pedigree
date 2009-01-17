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

// helper functions

uint8_t IsPowerOf2(uint32_t n)
{
   uint8_t log;

   for(log = 0; log < 16; log++)
   {
      if(n & 1)
      {
         n >>= 1;
         return (n != 0) ? -1 : log;
      }
      n >>= 1;
   }
   return -1;
}

FatFilesystem::FatFilesystem() :
  m_pDisk(0), m_Superblock(), m_Superblock16(), m_Superblock32() //, m_pGroupDescriptors(0), m_BlockSize(0)
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
  
  // check first up the first couple of bytes
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
  if (!IsPowerOf2(m_Superblock.BPB_SecPerClus))
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
  
  // number of root directory sectors
  if (!m_Superblock.BPB_BytsPerSec) // we sanity check the value, because we divide by this later
  {
    ERROR("FAT: BytsPerSec is zero!");
    return false;
  }
  uint32_t rootDirSectors = ((m_Superblock.BPB_RootEntCnt * 32) + (m_Superblock.BPB_BytsPerSec - 1)) / m_Superblock.BPB_BytsPerSec;
  
  // determine the size of the FAT
  // TODO: see that m_Superblock32 right there... it's not actually defined as anything YET. Fix that.
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
    
      m_RootDir.sector = m_Superblock.BPB_RsvdSecCnt + (m_Superblock.BPB_NumFATs * m_Superblock.BPB_FATSz16);
    
      break;
    
    case FAT32:
    
      m_RootDir.cluster = m_Superblock32.BPB_RootClus;
    
      break;
  }
  
  // fill the filesystem data
  m_DataAreaStart = firstDataSector;
  m_RootDirCount = rootDirSectors;
  m_BlockSize = m_Superblock.BPB_SecPerClus * m_Superblock.BPB_BytsPerSec;
  
  // TODO: read in the FAT32 FSInfo structure
  // TODO: m_Superblock16 and m_Superblock32 are not set yet...
  
  uint32_t type = m_Type;
  ERROR("FAT success, type is " << type);
  
  File rootdir = getRoot();
  File a = getDirectoryChild(&rootdir, 0);
  ERROR("FAT: First ent is " << a.getName());
  
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
  // we want to cater to unusual formats. So we check either:
  // a) the entire root directory (UNTIL found) for FAT12/16
  // b) the first cluster of the root directory for FAT32
  
  if(m_Type == FAT32)
  {
    // TODO: write
  }
  else
  {
    uint32_t rootSector = m_RootDir.sector;
    uint32_t rootDirSize = m_RootDirCount * m_Superblock.BPB_BytsPerSec;

    uint8_t* buffer = new uint8_t[rootDirSize];
    
    m_pDisk->read(static_cast<uint64_t>(m_Superblock.BPB_BytsPerSec)*static_cast<uint64_t>(rootSector), rootDirSize, reinterpret_cast<uintptr_t> (buffer));
    
    String volid;
    
    size_t i;
    for(i = 0; i < rootDirSize; i += sizeof(Dir))
    {
      Dir* ent = reinterpret_cast<Dir*>(&buffer[i]);
      if(ent->DIR_Attr & ATTR_VOLUME_ID)
      {
        volid = String(reinterpret_cast<const char*>(ent->DIR_Name));
        break;
      }
    }
    
    delete buffer;
    
    if(volid != "")
      return volid;
  }

  // none found, do a default
  NormalStaticString str;
  str += "no-volume-label@";
  str.append(reinterpret_cast<uintptr_t>(this), 16);
  return String(static_cast<const char*>(str));
}

uint64_t FatFilesystem::read(File *pFile, uint64_t location, uint64_t size, uintptr_t buffer)
{

  // Sanity check.
  if (pFile->isDirectory())
    return 0;

  /// \todo Write this
  return 0;
}

uint64_t FatFilesystem::write(File *pFile, uint64_t location, uint64_t size, uintptr_t buffer)
{
  return 0;
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
      
    uint32_t sec = m_RootDir.sector;
    
    sz = m_RootDirCount * m_Superblock.BPB_BytsPerSec;
    
    buffer = new uint8_t[sz];
    m_pDisk->read(static_cast<uint64_t>(m_Superblock.BPB_BytsPerSec)*static_cast<uint64_t>(sec), sz, reinterpret_cast<uintptr_t> (buffer));
  }
  else
  {
    // Read in the first cluster of the directory
    buffer = new uint8_t[m_BlockSize];
    readCluster(clus, reinterpret_cast<uintptr_t> (buffer));
  }
  
  // Parse
  size_t i, j = 0;
  for(i = 0; i < sz; i += sizeof(Dir), j++)
  {
    Dir* ent = reinterpret_cast<Dir*>(&buffer[i]);
    if(j == n)
    {
      File ret(String(reinterpret_cast<const char*>(ent->DIR_Name)), 0, 0, 0, ent->DIR_FstClusLO | (ent->DIR_FstClusHI << 16), false, ent->DIR_Attr & ATTR_DIRECTORY, this, ent->DIR_FileSize);
      delete buffer;
      return ret;
    }
  }
  
  // n too high?
  delete buffer;

  return File();
}

bool FatFilesystem::readCluster(uint32_t block, uintptr_t buffer)
{
  block = getSectorNumber(block);
  m_pDisk->read(static_cast<uint64_t>(m_Superblock.BPB_BytsPerSec)*static_cast<uint64_t>(block), m_BlockSize, buffer);
  return true;
}

uint32_t FatFilesystem::getSectorNumber(uint32_t cluster)
{
  return ((cluster - 2) * m_Superblock.BPB_SecPerClus) + m_DataAreaStart;
}

/* not for FAT
FatFilesystem::Inode FatFilesystem::getInode(uint32_t inode)
{
  inode--; // Inode zero is undefined, so it's not used.

  uint32_t index = inode % LITTLE_TO_HOST32(m_Superblock.s_inodes_per_group);
  uint32_t group = inode / LITTLE_TO_HOST32(m_Superblock.s_inodes_per_group);

  uint64_t inodeTableBlock = LITTLE_TO_HOST32(m_pGroupDescriptors[group].bg_inode_table);

  // The inode table may extend onto multiple blocks - if the inode we want is in another block,
  // calculate that now.
  while ( (index*sizeof(Inode)) > m_BlockSize )
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

void FatFilesystem::getBlockNumbersIndirect(uint32_t inode_block, int32_t startBlock, int32_t endBlock, List<uint32_t*> &list)
{
  if (endBlock < 0)
    return;
  if (startBlock < 0)
    startBlock = 0;

  uint32_t *buffer = new uint32_t[m_BlockSize/4];
  readCluster(inode_block, reinterpret_cast<uintptr_t> (buffer));

  for (int i = 0; i < static_cast<int32_t>((m_BlockSize/4)); i++)
  {
    if (i >= startBlock && i <= endBlock)
    {
      list.pushBack(reinterpret_cast<uint32_t*>(LITTLE_TO_HOST32(buffer[i])));
    }
  }
}

void FatFilesystem::getBlockNumbersBiindirect(uint32_t inode_block, int32_t startBlock, int32_t endBlock, List<uint32_t*> &list)
{
  if (endBlock < 0)
    return;
  if (startBlock < 0)
    startBlock = 0;

  uint32_t *buffer = new uint32_t[m_BlockSize/4];
  readCluster(inode_block, reinterpret_cast<uintptr_t> (buffer));

  for (unsigned int i = 0; i < (m_BlockSize/4); i++)
  {
    getBlockNumbersIndirect(LITTLE_TO_HOST32(buffer[i]), startBlock-(i*(m_BlockSize/4)),
                            endBlock-(i*(m_BlockSize/4)), list);
  }
}

void FatFilesystem::getBlockNumbers(Inode inode, uint32_t startBlock, uint32_t endBlock, List<uint32_t*> &list)
{
  for (unsigned int i = 0; i < 12; i++)
  {
    if (i >= startBlock && i <= endBlock)
    {
      list.pushBack(reinterpret_cast<uint32_t*>(LITTLE_TO_HOST32(inode.i_block[i])));
    }
  }

  getBlockNumbersIndirect(LITTLE_TO_HOST32(inode.i_block[12]), startBlock-12, endBlock-12, list);

  uint32_t numBlockNumbersPerBlock = m_BlockSize/4;

  getBlockNumbersBiindirect(LITTLE_TO_HOST32(inode.i_block[13]), startBlock-(12+numBlockNumbersPerBlock),
                            endBlock-(12+numBlockNumbersPerBlock), list);
  // I really hope triindirect isn't needed :(
}

void FatFilesystem::readInodeData(Inode inode, uintptr_t buffer, uint32_t startBlock, uint32_t endBlock)
{
  List<uint32_t*> list;
  getBlockNumbers(inode, startBlock, endBlock, list);

  for (List<uint32_t*>::Iterator it = list.begin();
       it != list.end();
       it++)
  {
    readCluster(reinterpret_cast<uint32_t>(*it), buffer);
    buffer += m_BlockSize;
  }
}
*/

void FatFilesystem::truncate(File *pFile)
{
}

bool FatFilesystem::createFile(File parent, String filename)
{
}

bool FatFilesystem::createDirectory(File parent, String filename)
{
}

bool FatFilesystem::createSymlink(File parent, String filename, String value)
{
}

bool FatFilesystem::remove(File parent, File file)
{
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

