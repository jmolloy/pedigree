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

uint64_t Ext2Filesystem::write(File *pFile, uint64_t location, uint64_t size, uintptr_t buffer)
{
  // Test whether the entire Filesystem is read-only.
  if(m_bReadOnly)
  {
    SYSCALL_ERROR(ReadOnlyFilesystem);
    return 0;
  }

  // Sanity check.
  if (pFile->isDirectory())
  {
    SYSCALL_ERROR(IsADirectory);
    return 0;
  }

  // Grab the inode.
  Inode inode = getInode(pFile->getInode());

  // Is the area we are writing to fully enclosed in the current size of the file?
  if ( (location+size) >= (static_cast<uint64_t>(inode.i_blocks) * m_BlockSize) )
  {
    // We need more blocks.

    // How many blocks do we need?
    uint64_t bytesOver = (location+size) -
      (static_cast<uint64_t>(LITTLE_TO_HOST32(inode.i_blocks)) * m_BlockSize);
    uint32_t blocksOver = bytesOver / m_BlockSize + 1;

    // Write protect the FS structures and go a-lookin' for blocks.
    m_WriteLock.acquire();

    for (size_t i = 0; i < blocksOver; i++)
    {
      uint32_t block = findFreeBlockInBitmap(pFile->getInode());
      if (block == 0)
      {
        SYSCALL_ERROR(NoSpaceLeftOnDevice);
        WARNING("EXT2: Filesystem full!");
        m_WriteLock.release();
        return 0;
      }
      setBlockNumber(pFile->getInode(), inode, i + LITTLE_TO_HOST32(inode.i_blocks), block);
      m_Superblock.s_blocks_count = HOST_TO_LITTLE32(LITTLE_TO_HOST32(m_Superblock.s_blocks_count)+1);
      m_SuperblockDirty = true;
    }

    inode.i_blocks = HOST_TO_LITTLE32(LITTLE_TO_HOST32(inode.i_blocks)+blocksOver);
    setInode(pFile->getInode(), inode);
    
    m_WriteLock.release();
  }

  /// \todo Each file should have a mutex. It'll have to be a pointer to one, so it's
  /// shared between all File instances.

  // Now we are certain we have the right number of blocks, we can write the data.
  size_t nBytes = size;
  size_t curBlock = location / m_BlockSize;
  size_t offset = location % m_BlockSize;
  while (nBytes)
  {
    if ( nBytes < m_BlockSize )
    {
      uint8_t *buf = new uint8_t[m_BlockSize];
      readInodeData(inode, reinterpret_cast<uintptr_t>(buf), curBlock, curBlock);
      memcpy(reinterpret_cast<void*>(buf+offset), reinterpret_cast<void*>(buffer), nBytes);
      writeInodeData (inode, reinterpret_cast<uintptr_t>(buf), curBlock);
      nBytes = 0;
      // Offset may not be zero, but we're exiting anyway. No need to set.
      break;
    }
    // If the location is on a block boundary, we can just write directly.
    else if (offset == 0)
    {
      writeInodeData(inode, buffer, curBlock);
      curBlock ++;
      nBytes -= m_BlockSize;
      offset = 0; // We are now on a block boundary, offset is 0.
      buffer += m_BlockSize;
    }
    // Else the location is not on a block boundary.
    else
    {
      uint8_t *buf = new uint8_t[m_BlockSize];
      readInodeData(inode, reinterpret_cast<uintptr_t>(buf), curBlock, curBlock);
      size_t n = m_BlockSize-offset;
      memcpy(reinterpret_cast<void*>(buf+offset), reinterpret_cast<void*>(buffer), n);
      writeInodeData (inode, reinterpret_cast<uintptr_t>(buf), curBlock);
      curBlock ++;
      offset = 0; // We are now on a block boundary, offset is 0.
      buffer += n;
      nBytes -= n;
    }
  }

  // Update the inode size.
  /// \todo Handle >4GB files.
  if ( (location+size) > inode.i_size )
  {
    inode.i_size = HOST_TO_LITTLE32(location+size);
    setInode(pFile->getInode(), inode);
  }

  // Write finished successfully.
  return size;
}

void Ext2Filesystem::setBlockNumber(uint32_t inode_num, Inode inode, size_t blockIdx, uint32_t blockValue)
{
  uint32_t blockSize = 1024 << m_Superblock.s_log_block_size;

  if (blockIdx < 12)
  {
    inode.i_block[blockIdx] = blockValue;
    // Rewrite the inode to disk.
    setInode(inode_num, inode);
    return;
  }

  blockIdx -= 12;

  // Check that a indirect data block exists.
  if (inode.i_block[12] == 0)
  {
    // Allocate a block.
    inode.i_block[12] = findFreeBlockInBitmap(1);
    if (inode.i_block[12] == 0)
    {
      SYSCALL_ERROR(NoSpaceLeftOnDevice);
      return;
    }
    // Write to disk.
    setInode(inode_num, inode);
  }

  if (blockIdx < (blockSize/sizeof(uint32_t)))
  {
    uint32_t *buffer = new uint32_t[blockSize/sizeof(uint32_t)];
    m_pDisk->read(static_cast<uint64_t>(m_BlockSize)*inode.i_block[12], m_BlockSize,
                  reinterpret_cast<uintptr_t> (buffer));
    buffer[blockIdx] = blockValue;
    m_pDisk->write(static_cast<uint64_t>(m_BlockSize)*inode.i_block[12], m_BlockSize,
                   reinterpret_cast<uintptr_t> (buffer));
    // Done, return.
    return;
  }

  // Bi-indirect time.
//  blockIdx -= blockSize/sizeof(uint32_t);

  FATAL("EXT2: Bi-indirect setBlockNumber needed!");
//  if (blockIdx < 
  
}

void Ext2Filesystem::addDirectoryEntry(uint32_t dir_inode, uint32_t child_inode, String filename, size_t node_type)
{
  Inode d_ino = getInode(dir_inode);
  Inode c_ino = getInode(child_inode);
  NOTICE("Add file: name " << filename << ", inode: " << child_inode);
  if (LITTLE_TO_HOST32(d_ino.i_flags) & 0x10000)  
  {
    ERROR("EXT2: addDirectoryEntry: hashed mode directories not implemented yet!");
    return;
  }

  uint32_t reclen = sizeof(uint32_t) + /* d_inode */
    sizeof(uint16_t) + /* d_reclen */
    sizeof(uint8_t) + /* d_namelen */
    sizeof(uint8_t) + /* d_file_type */
    strlen(filename); /* Null-terminator */

  // Read in the directory.
  uintptr_t bufferSize = LITTLE_TO_HOST32(d_ino.i_size);
  if (LITTLE_TO_HOST32(d_ino.i_size) % m_BlockSize)
    bufferSize += m_BlockSize - (LITTLE_TO_HOST32(d_ino.i_size)%m_BlockSize);

  uint8_t *buffer = new uint8_t[bufferSize+m_BlockSize];

  readInodeData(d_ino, reinterpret_cast<uintptr_t> (buffer), 0, (LITTLE_TO_HOST32(d_ino.i_size)/m_BlockSize)-1);

  // Find the insert point by cycling through the directory.
  bool failed = false;
  Dir *lastDirectory = getDirectoryEntry(d_ino, buffer, -1, failed);
  lastDirectory->d_reclen = HOST_TO_LITTLE16(4 + 2 + 1 + 1 + lastDirectory->d_namelen);
  Dir *directory = reinterpret_cast<Dir*> ( reinterpret_cast<uintptr_t> (lastDirectory) +
                                         LITTLE_TO_HOST16(lastDirectory->d_reclen) );

  uint32_t insertLocation = reinterpret_cast<uint32_t>(directory) - reinterpret_cast<uint32_t> (buffer);
  NOTICE("insert location: " << insertLocation);
  // Failed will be set to true, but that's fine. We wanted to fall off the end.

  // We're adding to the end of a directory - are we going to spill onto a new block?
  // Is the area we are writing to fully enclosed in the current size of the file?
  if ( insertLocation+reclen > (static_cast<uint64_t>(LITTLE_TO_HOST32(d_ino.i_blocks)) * m_BlockSize) )
  {
    // We need more blocks.

    // How many blocks do we need?
    uint64_t bytesOver = static_cast<uint64_t>(insertLocation)+reclen -
      (static_cast<uint64_t>(LITTLE_TO_HOST32(d_ino.i_blocks)) * m_BlockSize);
    uint32_t blocksOver = bytesOver / m_BlockSize + 1;

    // Write protect the FS structures and go a-lookin' for blocks.
    m_WriteLock.acquire();

    for (size_t i = 0; i < blocksOver; i++)
    {
      uint32_t block = findFreeBlockInBitmap(dir_inode);
      if (block == 0)
      {
        SYSCALL_ERROR(NoSpaceLeftOnDevice);
        WARNING("EXT2: Filesystem full!");
        m_WriteLock.release();
        return;
      }
      setBlockNumber(dir_inode, d_ino, i + LITTLE_TO_HOST32(d_ino.i_blocks), block);
      m_Superblock.s_blocks_count = HOST_TO_LITTLE32(LITTLE_TO_HOST32(m_Superblock.s_blocks_count)+1);
      m_SuperblockDirty = true;
    }

    d_ino.i_blocks = HOST_TO_LITTLE32(LITTLE_TO_HOST32(d_ino.i_blocks)+blocksOver);
    setInode(dir_inode, d_ino);
    
    m_WriteLock.release();
  }
  
  // Find the last block.
  uint32_t blockNum = (reinterpret_cast<uintptr_t>(lastDirectory)-reinterpret_cast<uintptr_t> (buffer)) / m_BlockSize;
  // Where does that block start in the buffer?
  uintptr_t bufferBlockStart = reinterpret_cast<uintptr_t> (buffer) + blockNum * m_BlockSize;

  bool extendsToNextBlock = (insertLocation+reclen)/m_BlockSize != blockNum;

  // Write our record.
  directory->d_inode = HOST_TO_LITTLE32(child_inode);
  directory->d_reclen = LITTLE_TO_HOST32(d_ino.i_size)-(reinterpret_cast<uintptr_t>(directory)-reinterpret_cast<uintptr_t> (buffer));
  directory->d_namelen = HOST_TO_LITTLE32(strlen(filename));
  directory->d_file_type = node_type;
  strcpy(directory->d_name, filename);

  // Write that block back.
  if (extendsToNextBlock)
  {
    writeInodeData(d_ino, bufferBlockStart, blockNum);
    writeInodeData(d_ino, bufferBlockStart+m_BlockSize, blockNum+1);
  }
  else
    writeInodeData(d_ino, bufferBlockStart, blockNum);

  delete [] buffer;

  // Change the directory's size.
  if (insertLocation+reclen > d_ino.i_size)
  {
    d_ino.i_size = insertLocation+reclen;
    setInode(dir_inode, d_ino);
  }
 
}
