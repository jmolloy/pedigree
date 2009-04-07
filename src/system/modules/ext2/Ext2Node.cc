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

#include "Ext2Node.h"
#include "Ext2Filesystem.h"
#include <syscallError.h>

Ext2Node::Ext2Node(uintptr_t inode_num, Inode inode, Ext2Filesystem *pFs) :
  m_Inode(inode), m_InodeNumber(inode_num), m_pExt2Fs(pFs), m_Blocks(0),
  m_nBlocks(LITTLE_TO_HOST32(inode.i_blocks)), m_nSize(LITTLE_TO_HOST32(inode.i_size))
{
}

Ext2Node::~Ext2Node()
{
}

uint64_t Ext2Node::read(uint64_t location, uint64_t size, uintptr_t buffer)
{
  // Make sure we know what blocks we're using.
  if (m_Blocks.count() != m_nBlocks)
  {
    if (!getBlockNumbers())
    {
      FATAL("EXT2: Unable to get block numbers!");
    }
  }

  // Reads get clamped to the filesize.
  if (location >= m_nSize) return 0;
  if ( (location+size) >= m_nSize) size = m_nSize - location;

  if (size == 0) return 0;

  // Special case for symlinks - if we have no blocks but have positive size,
  // We interpret the i_blocks member as data.
  if (m_Inode.i_blocks == 0 && m_nSize > 0)
  {
    memcpy(reinterpret_cast<uint8_t*>(buffer+location),
           reinterpret_cast<uint8_t*>(m_Inode.i_block),
           size);
    return size;
  }

  size_t nBs = m_pExt2Fs->m_BlockSize;

  size_t nBytes = size;
  uint32_t nBlock = location / nBs;
  while (nBytes)
  {
    // If the current location is block-aligned and we have to read at least a
    // block in, we can read directly to the buffer.
    if ( (location % nBs) == 0 && nBytes >= nBs )
    {
      m_pExt2Fs->readBlock(reinterpret_cast<uint32_t>(m_Blocks[nBlock]), buffer);
      buffer += nBs;
      location += nBs;
      nBytes -= nBs;
      nBlock++;
    }
    // Else we have to read in a partial block.
    else
    {
      // Create a buffer for the block.
      uint8_t *pBuf = new uint8_t[nBs];
      m_pExt2Fs->readBlock(reinterpret_cast<uint32_t>(m_Blocks[nBlock]),
                           reinterpret_cast<uintptr_t>(pBuf));
      // memcpy the relevant block area.
      uintptr_t start = location % nBs;
      uintptr_t size = (start+nBytes >= nBs) ? nBs-start : nBytes;
      memcpy(reinterpret_cast<uint8_t*>(buffer), &pBuf[start], size);
      delete [] pBuf;
      buffer += size;
      location += size;
      nBytes -= size;
      nBlock++;
    }
  }

  return size;
}

uint64_t Ext2Node::write(uint64_t location, uint64_t size, uintptr_t buffer)
{
  // Make sure we know what blocks we're using.
  if (m_Blocks.count() != m_nBlocks)
  {
    if (!getBlockNumbers())
    {
      FATAL("EXT2: Unable to get block numbers!");
    }
  }

  ensureLargeEnough(location+size);

  size_t nBs = m_pExt2Fs->m_BlockSize;

  size_t nBytes = size;
  uint32_t nBlock = location / nBs;
  while (nBytes)
  {
    // If the current location is block-aligned and we have to write at least a
    // block out, we can write directly to the buffer.
    if ( (location % nBs) == 0 && nBytes >= nBs )
    {
      m_pExt2Fs->writeBlock(reinterpret_cast<uint32_t>(m_Blocks[nBlock]), buffer);
      buffer += nBs;
      location += nBs;
      nBytes -= nBs;
      nBlock++;
    }
    // Else we have to read in a block, partially edit it and write back.
    else
    {
      // Create a buffer for the block.
      uint8_t *pBuf = new uint8_t[nBs];
      m_pExt2Fs->readBlock(reinterpret_cast<uint32_t>(m_Blocks[nBlock]),
                           reinterpret_cast<uintptr_t>(pBuf));
      // memcpy the relevant block area.
      uintptr_t start = location % nBs;
      uintptr_t size = (start+nBytes >= nBs) ? nBs-start : nBytes;
      memcpy(&pBuf[start], reinterpret_cast<uint8_t*>(buffer), size);
      m_pExt2Fs->writeBlock(reinterpret_cast<uint32_t>(m_Blocks[nBlock]),
                            reinterpret_cast<uintptr_t>(pBuf));
      delete [] pBuf;
      buffer += size;
      location += size;
      nBytes -= size;
      nBlock++;
    }
  }

  return size;
}

void Ext2Node::truncate()
{
  // Make sure we know what blocks we're using.
  if (m_Blocks.count() != m_nBlocks)
  {
    if (!getBlockNumbers())
    {
      FATAL("EXT2: Unable to get block numbers!");
    }
  }

  // Truncation is just setting the size to 0 and removing all blocks.
  for (Vector<uint32_t*>::Iterator it = m_Blocks.begin();
       it != m_Blocks.end();
       it++)
  {
    m_pExt2Fs->releaseBlock(reinterpret_cast<uintptr_t>(*it));
  }

  m_nSize = 0;
  m_nBlocks = 0;
  m_Blocks.clear();
  m_Inode.i_size = 0;
  m_Inode.i_blocks = 0;
}

bool Ext2Node::ensureLargeEnough(size_t size)
{
  if (size > m_nSize)
  {
    m_nSize = size;
    fileAttributeChanged(m_nSize, LITTLE_TO_HOST32(m_Inode.i_atime),
                         LITTLE_TO_HOST32(m_Inode.i_mtime),
                         LITTLE_TO_HOST32(m_Inode.i_ctime));
  }

  while (size > m_nBlocks*m_pExt2Fs->m_BlockSize)
  {
    uint32_t block = m_pExt2Fs->findFreeBlock(m_InodeNumber);
    if (block == 0)
    {
      // We had a problem.
      SYSCALL_ERROR(NoSpaceLeftOnDevice);
      return false;
    }
    if (!addBlock(block)) return false;
    // Load the block and zero it.
    uint8_t *pBuffer = new uint8_t[m_pExt2Fs->m_BlockSize];
    m_pExt2Fs->readBlock(block, reinterpret_cast<uintptr_t>(pBuffer));
    memset(pBuffer, 0, m_pExt2Fs->m_BlockSize);
    m_pExt2Fs->writeBlock(block, reinterpret_cast<uintptr_t>(pBuffer));
    delete [] pBuffer;
  }
  return true;
}

bool Ext2Node::getBlockNumbers()
{
  // Add the first twelve blocks...
  size_t i;
  for (i = 0; i < 12 && i < m_nBlocks; i++)
  {
    m_Blocks.pushBack(reinterpret_cast<uint32_t*>(LITTLE_TO_HOST32(m_Inode.i_block[i])));
  }
  if (i >= m_nBlocks) return true;

  getBlockNumbersIndirect(LITTLE_TO_HOST32(m_Inode.i_block[12]), i);
  if (i >= m_nBlocks) return true;

  getBlockNumbersBiindirect(LITTLE_TO_HOST32(m_Inode.i_block[13]), i);
  if (i >= m_nBlocks) return true;

  getBlockNumbersTriindirect(LITTLE_TO_HOST32(m_Inode.i_block[14]), i);
  if (i >= m_nBlocks) return true;

  // If we haven't reached m_nBlocks by now, we've gone seriously wrong somewhere.
  FATAL("EXT2: m_nBlocks far too massive (" << m_nBlocks << ") - something is wrong.");
  return false;
}

bool Ext2Node::getBlockNumbersIndirect(uint32_t inode_block, size_t &nBlocks)
{
  uint32_t *buffer = new uint32_t[m_pExt2Fs->m_BlockSize/4];
  m_pExt2Fs->readBlock(inode_block, reinterpret_cast<uintptr_t>(buffer));

  for (size_t i = 0; i < m_pExt2Fs->m_BlockSize/4 && nBlocks < m_nBlocks; i++)
  {
    m_Blocks.pushBack(reinterpret_cast<uint32_t*>(LITTLE_TO_HOST32(buffer[i])));
    nBlocks++;
  }

  delete [] buffer;
}

bool Ext2Node::getBlockNumbersBiindirect(uint32_t inode_block, size_t &nBlocks)
{
  uint32_t *buffer = new uint32_t[m_pExt2Fs->m_BlockSize/4];
  m_pExt2Fs->readBlock(inode_block, reinterpret_cast<uintptr_t>(buffer));

  for (size_t i = 0; i < m_pExt2Fs->m_BlockSize/4 && nBlocks < m_nBlocks; i++)
  {
    getBlockNumbersIndirect(LITTLE_TO_HOST32(buffer[i]), nBlocks);
  }

  delete [] buffer;
}

bool Ext2Node::getBlockNumbersTriindirect(uint32_t inode_block, size_t &nBlocks)
{
  uint32_t *buffer = new uint32_t[m_pExt2Fs->m_BlockSize/4];
  m_pExt2Fs->readBlock(inode_block, reinterpret_cast<uintptr_t>(buffer));

  for (size_t i = 0; i < m_pExt2Fs->m_BlockSize/4 && nBlocks < m_nBlocks; i++)
  {
    getBlockNumbersBiindirect(LITTLE_TO_HOST32(buffer[i]), nBlocks);
  }

  delete [] buffer;
}

bool Ext2Node::addBlock(uint32_t blockValue)
{
  size_t nEntriesPerBlock = m_pExt2Fs->m_BlockSize/4;

  // Calculate whether direct, indirect or tri-indirect addressing is needed.
  if (m_nBlocks < 12)
  {
    // Direct addressing is possible.
    m_Inode.i_block[m_nBlocks] = HOST_TO_LITTLE32(blockValue);
  }
  else if (m_nBlocks < 12 + nEntriesPerBlock)
  {
    // Indirect addressing needed.

    // If this is the first indirect block, we need to reserve a new table block.
    if (m_nBlocks == 12)
    {
      m_Inode.i_block[12] = HOST_TO_LITTLE32(m_pExt2Fs->findFreeBlock(m_InodeNumber));
      if (m_Inode.i_block[12] == 0)
      {
        // We had a problem.
        SYSCALL_ERROR(NoSpaceLeftOnDevice);
        return false;
      }
    }

    // Now we can set the block.
    uint32_t *buffer = new uint32_t[nEntriesPerBlock];
    m_pExt2Fs->readBlock(LITTLE_TO_HOST32(m_Inode.i_block[12]), reinterpret_cast<uintptr_t>(buffer));

    buffer[m_nBlocks-12] = HOST_TO_LITTLE32(blockValue);

    m_pExt2Fs->writeBlock(LITTLE_TO_HOST32(m_Inode.i_block[12]), reinterpret_cast<uintptr_t>(buffer));

    delete [] buffer;
  }
  else if (m_nBlocks < 12 + nEntriesPerBlock + nEntriesPerBlock*nEntriesPerBlock)
  {
    // Bi-indirect addressing required.

    // Index from the start of the bi-indirect block (i.e. ignore the 12 direct entries and one indirect block).
    size_t biIdx = m_nBlocks - 12 - nEntriesPerBlock;
    // Block number inside the bi-indirect table of where to find the indirect block table.
    size_t indirectBlock = biIdx / nEntriesPerBlock;
    // Index inside the indirect block table.
    size_t indirectIdx = biIdx % nEntriesPerBlock;

    // If this is the first bi-indirect block, we need to reserve a bi-indirect table block.
    if (biIdx == 0)
    {
      m_Inode.i_block[13] = HOST_TO_LITTLE32(m_pExt2Fs->findFreeBlock(m_InodeNumber));
      if (m_Inode.i_block[13] == 0)
      {
        // We had a problem.
        SYSCALL_ERROR(NoSpaceLeftOnDevice);
        return false;
      }
    }

    // Now we can safely read the bi-indirect block.
    uint32_t *pBlock = new uint32_t[nEntriesPerBlock];

    m_pExt2Fs->readBlock(LITTLE_TO_HOST32(m_Inode.i_block[13]), reinterpret_cast<uintptr_t>(pBlock));

    // Do we need to start a new indirect block?
    if (indirectIdx == 0)
    {
      pBlock[indirectBlock] = HOST_TO_LITTLE32(m_pExt2Fs->findFreeBlock(m_InodeNumber));
      if (pBlock[indirectBlock] == 0)
      {
        // We had a problem.
        SYSCALL_ERROR(NoSpaceLeftOnDevice);
        return false;
      }

      // Write the block back, as we've modified it.
      m_pExt2Fs->writeBlock(LITTLE_TO_HOST32(m_Inode.i_block[13]), reinterpret_cast<uintptr_t>(pBlock));
    }

    // Cache this as it gets clobbered by the readBlock call (using the same buffer).
    uint32_t nIndirectBlockNum = LITTLE_TO_HOST32(pBlock[indirectBlock]);

    // Grab the indirect block.
    m_pExt2Fs->readBlock(nIndirectBlockNum, reinterpret_cast<uintptr_t>(pBlock));

    // Set the correct entry.
    pBlock[indirectIdx] = HOST_TO_LITTLE32(blockValue);

    // Write back.
    m_pExt2Fs->writeBlock(nIndirectBlockNum, reinterpret_cast<uintptr_t>(pBlock));

    // Done.
    delete [] pBlock;
  }
  else
  {
    // Tri-indirect addressing required.
    FATAL("EXT2: Tri-indirect addressing required, but not implemented.");
    return false;
  }

  m_Blocks.pushBack(reinterpret_cast<uint32_t*>(blockValue));
  m_nBlocks++;
  m_Inode.i_blocks = HOST_TO_LITTLE32(m_nBlocks);
  m_pExt2Fs->setInode(m_InodeNumber, m_Inode);

  return true;
}

void Ext2Node::fileAttributeChanged(size_t size, size_t atime, size_t mtime, size_t ctime)
{
  // Reconstruct the inode from the cached fields.
  m_Inode.i_blocks = HOST_TO_LITTLE32(m_nBlocks);
  m_Inode.i_size = HOST_TO_LITTLE32(size); /// \todo 4GB files.
  m_Inode.i_atime = HOST_TO_LITTLE32(atime);
  m_Inode.i_mtime = HOST_TO_LITTLE32(mtime);
  m_Inode.i_ctime = HOST_TO_LITTLE32(ctime);

  m_pExt2Fs->setInode(m_InodeNumber, m_Inode);
}
