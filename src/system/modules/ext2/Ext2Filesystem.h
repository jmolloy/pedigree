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
#ifndef EXT2FILESYSTEM_H
#define EXT2FILESYSTEM_H

#include <vfs/VFS.h>
#include <vfs/Filesystem.h>
#include <utilities/List.h>
#include <process/Mutex.h>
#include <utilities/Tree.h>
#include <utilities/Vector.h>

#include "ext2.h"

/** This class provides an implementation of the second extended filesystem. */
class Ext2Filesystem : public Filesystem
{
  friend class Ext2File;
  friend class Ext2Node;
  friend class Ext2Directory;
  friend class Ext2Symlink;
public:
  Ext2Filesystem();

  virtual ~Ext2Filesystem();

  //
  // Filesystem interface.
  //
  virtual bool initialise(Disk *pDisk);
  static Filesystem *probe(Disk *pDisk);
  virtual File* getRoot();
  virtual String getVolumeLabel();
  virtual uint64_t read(File *pFile, uint64_t location, uint64_t size, uintptr_t buffer);
  virtual uint64_t write(File *pFile, uint64_t location, uint64_t size, uintptr_t buffer);
  virtual void truncate(File *pFile);
  virtual void fileAttributeChanged(File *pFile);
  virtual void cacheDirectoryContents(File *pFile);

protected:
  virtual bool createFile(File* parent, String filename, uint32_t mask);
  virtual bool createDirectory(File* parent, String filename);
  virtual bool createSymlink(File* parent, String filename, String value);
  virtual bool remove(File* parent, File* file);

private:
  virtual bool createNode(File* parent, String filename, uint32_t mask, String value, size_t type);

  /** Inaccessible copy constructor and operator= */
  Ext2Filesystem(const Ext2Filesystem&);
  void operator =(const Ext2Filesystem&);

  /** Reads a block of data from the disk. */
  bool readBlock(uint32_t block, uintptr_t buffer);
  bool writeBlock(uint32_t block, uintptr_t buffer);

  uint32_t findFreeBlock(uint32_t inode);
  uint32_t findFreeInode();

  void releaseBlock(uint32_t block);

  Inode getInode(uint32_t num);
  bool setInode(uint32_t num, Inode inode);

  /** Our raw device. */
  Disk *m_pDisk;

  /** Our superblock. */
  Superblock m_Superblock;

  /** Group descriptors. */
  GroupDesc *m_pGroupDescriptors;

  /** Size of a block. */
  uint32_t m_BlockSize;

  /** Write lock - we're finding some inodes and updating the superblock and block group structures. */
  Mutex m_WriteLock;

  /** Is the superblock dirty? Does it need to be written back to disk? */
  bool m_bSuperblockDirty;
  bool m_bGroupDescriptorsDirty;

  /** The root filesystem node. */
  File *m_pRoot;
};

#endif
