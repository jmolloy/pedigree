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
#ifndef EXT2_NODE_H
#define EXT2_NODE_H

#include "ext2.h"
#include <utilities/Vector.h>
#include "Ext2Filesystem.h"

/** A node in an ext2 filesystem. */
class Ext2Node
{
private:
  /** Copy constructors are hidden - unused! */
  Ext2Node(const Ext2Node &file);
  Ext2Node& operator =(const Ext2Node&);
public:
  /** Constructor, should be called only by a Filesystem. */
  Ext2Node(uintptr_t inode_num, Inode inode, class Ext2Filesystem *pFs);
  /** Destructor */
  virtual ~Ext2Node();

  Inode *getInode()
  {return &m_Inode;}

  uint32_t getInodeNumber()
  {return m_InodeNumber;}

  /** Updates inode attributes. */
  void fileAttributeChanged(size_t size, size_t atime, size_t mtime, size_t ctime);

  uint64_t read(uint64_t location, uint64_t size, uintptr_t buffer);
  uint64_t write(uint64_t location, uint64_t size, uintptr_t buffer);

  void truncate();

protected:
  /** Ensures the inode is at least 'size' big. */
  bool ensureLargeEnough(size_t size);

  bool addBlock(uint32_t blockValue);

  bool getBlockNumbers();
  bool getBlockNumbersIndirect(uint32_t inode_block, size_t &nBlocks);
  bool getBlockNumbersBiindirect(uint32_t inode_block, size_t &nBlocks);
  bool getBlockNumbersTriindirect(uint32_t inode_block, size_t &nBlocks);

  bool setBlockNumber(size_t blockNum, uint32_t blockValue);

  Inode m_Inode;
  uint32_t m_InodeNumber;
  class Ext2Filesystem *m_pExt2Fs;

  Vector<uint32_t*> m_Blocks;

  uint32_t m_nBlocks;

  size_t m_nSize;
};

#endif
