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

Ext2Filesystem::Inode Ext2Filesystem::getInode(uint32_t inode)
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

void Ext2Filesystem::setInode(uint32_t inode, Inode in)
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

uint32_t Ext2Filesystem::findFreeBlockInBitmap(uint32_t inode)
{
  inode--; // Inode zero is undefined, so it's not used.

  uint32_t group = inode / LITTLE_TO_HOST32(m_Superblock.s_inodes_per_group);

  uint32_t nGroups = m_Superblock.s_inodes_count / m_Superblock.s_inodes_per_group;

  uint8_t *buffer = new uint8_t[m_BlockSize];
  while (group < nGroups)
  {
    uint64_t bitmapBlock = LITTLE_TO_HOST32(m_pGroupDescriptors[group].bg_block_bitmap);
    m_pDisk->read(static_cast<uint64_t>(m_BlockSize)*bitmapBlock, m_BlockSize,
                  reinterpret_cast<uintptr_t> (buffer));

    for (size_t i = 0; i < m_Superblock.s_blocks_per_group; i += 8*sizeof(uint32_t))
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
            return (group*m_Superblock.s_blocks_per_group) + i*8*sizeof(uint32_t) +
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

uint32_t Ext2Filesystem::findFreeInodeInBitmap()
{
  uint32_t group = 0;
  uint32_t nGroups = m_Superblock.s_inodes_count / m_Superblock.s_inodes_per_group;

  uint8_t *buffer = new uint8_t[m_BlockSize];
  while (group < nGroups)
  {
    uint64_t inodeBlock = LITTLE_TO_HOST32(m_pGroupDescriptors[group].bg_inode_bitmap);
    m_pDisk->read(static_cast<uint64_t>(m_BlockSize)*inodeBlock, m_BlockSize,
                  reinterpret_cast<uintptr_t> (buffer));

    for (size_t i = 0; i < m_Superblock.s_inodes_per_group; i += 8*sizeof(uint32_t))
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
            return (group*m_Superblock.s_inodes_per_group) + i*8*sizeof(uint32_t) +
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

void Ext2Filesystem::readInodeData(Inode inode, uintptr_t buffer, uint32_t startBlock, uint32_t endBlock)
{
  if (inode.i_blocks > 0)
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
  else
  {
    memcpy(reinterpret_cast<void*>(buffer), reinterpret_cast<void*>(inode.i_block), 4*15);
  }
}

void Ext2Filesystem::writeInodeData(Inode inode, uintptr_t buffer, uint32_t block)
{
  List<uint32_t*> list;
  getBlockNumbers(inode, block, block, list);

  for (List<uint32_t*>::Iterator it = list.begin();
       it != list.end();
       it++)
  {
    m_pDisk->write(static_cast<uint64_t>(m_BlockSize)*reinterpret_cast<uint32_t>(*it),
                   m_BlockSize,
                   buffer);
    buffer += m_BlockSize;
  }
}

Ext2Filesystem::Dir *Ext2Filesystem::getDirectoryEntry(Inode inode, uint8_t *buffer, ssize_t n, bool &failed)
{
  failed = false;
  // Is the directory in hashed mode?
  if (LITTLE_TO_HOST32(inode.i_flags) & 0x10000)
  {
    /// \todo Implement hashed mode directories.
    ERROR ("Hashed mode directories not implemented yet.");
    Processor::breakpoint();
    return 0;
  }
  else
  {
    // Traverse the directory.
    Dir *lastdir = 0;
    Dir *directory = reinterpret_cast<Dir*> (buffer);

    unsigned int i = 0;
    while (reinterpret_cast<uintptr_t>(directory)-reinterpret_cast<uintptr_t>(buffer) < LITTLE_TO_HOST32(inode.i_size))
    {
      if (i == n)
      {
        return directory;
      }
      lastdir = directory;
      directory = reinterpret_cast<Dir*> ( reinterpret_cast<uintptr_t> (directory) +
                                         LITTLE_TO_HOST16(directory->d_reclen) );
      i++;
    }
    // n not found, return the directory and set failed to true.
    failed = true;
    return lastdir;
  }
}
