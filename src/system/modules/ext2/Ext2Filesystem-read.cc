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

void Ext2Filesystem::cacheDirectoryContents(File *pFile)
{
  // Sanity check.
  if (!pFile->isDirectory())
    return;

  Inode inode = getInode(pFile->getInode());

  uintptr_t bufferSize = LITTLE_TO_HOST32(inode.i_size);
  if (LITTLE_TO_HOST32(inode.i_size) % m_BlockSize)
    bufferSize += m_BlockSize - (LITTLE_TO_HOST32(inode.i_size)%m_BlockSize);

  uint8_t *buffer = new uint8_t[bufferSize];

  readInodeData(inode, reinterpret_cast<uintptr_t> (buffer), 0, (LITTLE_TO_HOST32(inode.i_size)/m_BlockSize)-1);

  size_t i = 0;
  while (true)
  {
    bool failed = false;
    Dir *directory = getDirectoryEntry(inode, buffer, i, failed);
    if (!directory || failed || directory->d_inode == 0)
      break;

    // Create a file.
    char *name = new char[directory->d_namelen+1];
    strncpy(name, directory->d_name, directory->d_namelen);
    name[directory->d_namelen] = '\0';

    String sName = String(name);
    delete [] name;

    Inode _inode = getInode(LITTLE_TO_HOST32(directory->d_inode));

    File *pFile_;
    pFile_ = new File (sName,                // Name
                       0,                    // Accessed time
                       0,                    // Modified time
                       0,                    // Creation time
                       LITTLE_TO_HOST32(directory->d_inode),   // Inode
                       (directory->d_file_type==EXT2_SYMLINK)?true:false, // Symlink
                       (directory->d_file_type==EXT2_DIRECTORY)?true:false, // Directory
                       this,
                       LITTLE_TO_HOST32(_inode.i_size),
                       pFile /* Parent */,
                       false /* Cached, please don't delete! */);

    // Insert into the cache.
    pFile->m_Cache.insert(sName, pFile_);
    i++;
  }

  delete [] buffer;
}
