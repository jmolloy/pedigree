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
#include <vfs/VFS.h>
#include <Module.h>
#include <utilities/utility.h>
#include <Log.h>
#include <utilities/List.h>
#include <processor/Processor.h>
#include <utilities/StaticString.h>
#include <syscallError.h>

Ext2Filesystem::Ext2Filesystem() :
  m_pDisk(0), m_Superblock(), m_pGroupDescriptors(0), m_BlockSize(0)
{
}

Ext2Filesystem::~Ext2Filesystem()
{
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
  {
    return pFs;
  }
}

File Ext2Filesystem::getRoot()
{
  return File(String(""), 0, 0, 0, EXT2_ROOT_INO, false, true, this, 0);
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

uint64_t Ext2Filesystem::read(File *pFile, uint64_t location, uint64_t size, uintptr_t buffer)
{

  // Sanity check.
  if (pFile->isDirectory())
    return 0;

  // Get the inode to read from.
  Inode inode = getInode(pFile->getInode());

  // Sanity check the location, size parameters.
  uint64_t fileSize = LITTLE_TO_HOST32(inode.i_size); /// \todo Handle >4GB files.
  if (location >= fileSize)
    // Fatal - we can't read past the end of the file.
    return 0;

  if ( (location+size) >= fileSize)
    // Not fatal - we just clamp the size.
    size = fileSize-location;

  uint64_t firstByte = location;
  uint64_t lastByte = location+size-1;

  // Find the largest contiguous number of blocks possible to read.
  // Remember that "buffer" is only "size" long, so we can't overflow by reading
  // a full block when a partial one was requested.
  int32_t firstBlock = firstByte / static_cast<uint64_t>(m_BlockSize);
  int32_t lastBlock = lastByte / static_cast<uint64_t>(m_BlockSize);

  // Perform the read.
  uint8_t *buf = new uint8_t[m_BlockSize+1];
  uint32_t currentLocation = 0;

  buf[m_BlockSize] = 0xEE;

  bool lessThanOneBlock = firstBlock == lastBlock;

  // Check if firstByte lies on a block boundary, or if there is only one block to be read.
  if ( (firstByte % m_BlockSize) != 0 || lessThanOneBlock)
  {
    // First byte is not on a block boundary - read in the block the first byte is in and
    // memcpy the amount required to make it up to a block boundary, or less if the end byte is under that.
    uint32_t firstBlockPartialOffset = firstByte % m_BlockSize;
    uint32_t firstBlockPartialLength;
    if (firstBlock == lastBlock)
      firstBlockPartialLength = (lastByte % m_BlockSize) - firstBlockPartialOffset + 1;
    else
      firstBlockPartialLength = m_BlockSize - firstBlockPartialOffset;

    readInodeData(inode, reinterpret_cast<uintptr_t>(buf), firstBlock, firstBlock);
    memcpy(reinterpret_cast<void*>(buffer), reinterpret_cast<void*>(&buf[firstByte%m_BlockSize]), firstBlockPartialLength);
    currentLocation += firstBlockPartialLength;
    firstBlock++;
  }

  // If the last byte doesn't lie on a block boundary, fetch one less block
  // so we don't memory overflow. We'll deal with this case in the next section.
  if ( (lastByte % m_BlockSize) != (m_BlockSize-1))
    lastBlock--;

  if (firstBlock <= lastBlock && !lessThanOneBlock)
  {
    readInodeData(inode, buffer+currentLocation, firstBlock, lastBlock);
    currentLocation += (lastBlock-firstBlock+1)*m_BlockSize;
  }

  // If we deducted a block in the previous if(), we need to deal with it now, only if the first block != last block, as that is dealt with
  // in the first if ().
  if ( (lastByte % m_BlockSize) != (m_BlockSize-1) && !lessThanOneBlock)
  {
    uint32_t lastBlockPartialLength = (lastByte % m_BlockSize)+1;
    readInodeData(inode, reinterpret_cast<uintptr_t>(buf), lastBlock+1, lastBlock+1);
    memcpy(reinterpret_cast<void*>(buffer+currentLocation), reinterpret_cast<void*>(buf), lastBlockPartialLength);
    currentLocation += lastBlockPartialLength;
  }

  // Sanity check.
  if (currentLocation != size)
  {
    ERROR("Ext2: read(), size calculation error!");
  }

  if (buf[m_BlockSize] != 0xEE)
  {
    ERROR("Ext2: read(), buffer overflow!");
  }

  delete [] buf;

  return size;
}

uint64_t Ext2Filesystem::write(File *pFile, uint64_t location, uint64_t size, uintptr_t buffer)
{
  //  test whether the entire Filesystem is read-only.
  if(bReadOnly)
  {
    SYSCALL_ERROR(ReadOnlyFilesystem);
    return 0;
  }
  return 0;
}

void Ext2Filesystem::fileAttributeChanged(File *pFile)
{
}

File Ext2Filesystem::getDirectoryChild(File *pFile, size_t n)
{
  // Sanity check.
  if (!pFile->isDirectory())
    return File();

  Inode inode = getInode(pFile->getInode());

  // Is the directory in hashed mode?
  if (LITTLE_TO_HOST32(inode.i_flags) & 0x10000)
  {
    /// \todo Implement hashed mode directories.
    ERROR ("Hashed mode directories not implemented yet.");
    Processor::breakpoint();
    return File();
  }

  uintptr_t bufferSize = LITTLE_TO_HOST32(inode.i_size);
  if (LITTLE_TO_HOST32(inode.i_size) % m_BlockSize)
    bufferSize += m_BlockSize - (LITTLE_TO_HOST32(inode.i_size)%m_BlockSize);

  uint8_t *buffer = new uint8_t[bufferSize];

  readInodeData(inode, reinterpret_cast<uintptr_t> (buffer), 0, (LITTLE_TO_HOST32(inode.i_size)/m_BlockSize)-1);

  // Traverse the directory.
  Dir *directory = reinterpret_cast<Dir*> (buffer);

  unsigned int i = 0;
  while (reinterpret_cast<uintptr_t>(directory)-reinterpret_cast<uintptr_t>(buffer) < LITTLE_TO_HOST32(inode.i_size))
  {
    if (i == n)
    {
      // Create a file.
      char *name = new char[directory->d_namelen+1];
      strncpy(name, directory->d_name, directory->d_namelen);
      name[directory->d_namelen] = '\0';

      String sName = String(name);
      delete [] name;

      Inode inode = getInode(LITTLE_TO_HOST32(directory->d_inode));

      return File (sName,                // Name
                   0,                    // Accessed time
                   0,                    // Modified time
                   0,                    // Creation time
                   LITTLE_TO_HOST32(directory->d_inode),   // Inode
                   (directory->d_file_type==EXT2_SYMLINK)?true:false, // Symlink
                   (directory->d_file_type==EXT2_DIRECTORY)?true:false, // Directory
                   this,
                   LITTLE_TO_HOST32(inode.i_size));
    }

    directory = reinterpret_cast<Dir*> ( reinterpret_cast<uintptr_t> (directory) +
                                         LITTLE_TO_HOST16(directory->d_reclen) );
    i++;
  }

  // n too big, return invalid file.
  return File();
}

Ext2Filesystem::Inode Ext2Filesystem::getInode(uint32_t inode)
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

bool Ext2Filesystem::readBlock(uint32_t block, uintptr_t buffer)
{
  m_pDisk->read(static_cast<uint64_t>(m_BlockSize)*static_cast<uint64_t>(block), m_BlockSize, buffer);
  return true;
}

void Ext2Filesystem::getBlockNumbersIndirect(uint32_t inode_block, int32_t startBlock, int32_t endBlock, List<uint32_t*> &list)
{
  if (endBlock < 0)
    return;
  if (startBlock < 0)
    startBlock = 0;

  uint32_t *buffer = new uint32_t[m_BlockSize/4];
  readBlock(inode_block, reinterpret_cast<uintptr_t> (buffer));

  for (int i = 0; i < static_cast<int32_t>((m_BlockSize/4)); i++)
  {
    if (i >= startBlock && i <= endBlock)
    {
      list.pushBack(reinterpret_cast<uint32_t*>(LITTLE_TO_HOST32(buffer[i])));
    }
  }
}

void Ext2Filesystem::getBlockNumbersBiindirect(uint32_t inode_block, int32_t startBlock, int32_t endBlock, List<uint32_t*> &list)
{
  if (endBlock < 0)
    return;
  if (startBlock < 0)
    startBlock = 0;

  uint32_t *buffer = new uint32_t[m_BlockSize/4];
  readBlock(inode_block, reinterpret_cast<uintptr_t> (buffer));

  for (unsigned int i = 0; i < (m_BlockSize/4); i++)
  {
    getBlockNumbersIndirect(LITTLE_TO_HOST32(buffer[i]), startBlock-(i*(m_BlockSize/4)),
                            endBlock-(i*(m_BlockSize/4)), list);
  }
}

void Ext2Filesystem::getBlockNumbers(Inode inode, uint32_t startBlock, uint32_t endBlock, List<uint32_t*> &list)
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

void Ext2Filesystem::readInodeData(Inode inode, uintptr_t buffer, uint32_t startBlock, uint32_t endBlock)
{
  List<uint32_t*> list;
  getBlockNumbers(inode, startBlock, endBlock, list);

  for (List<uint32_t*>::Iterator it = list.begin();
       it != list.end();
       it++)
  {
    readBlock(reinterpret_cast<uint32_t>(*it), buffer);
    buffer += m_BlockSize;
  }
}

void Ext2Filesystem::truncate(File *pFile)
{
}

bool Ext2Filesystem::createFile(File parent, String filename)
{
}

bool Ext2Filesystem::createDirectory(File parent, String filename)
{
}

bool Ext2Filesystem::createSymlink(File parent, String filename, String value)
{
}

bool Ext2Filesystem::remove(File parent, File file)
{
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

