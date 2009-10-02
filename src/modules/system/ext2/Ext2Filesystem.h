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
    uintptr_t readBlock(uint32_t block);

    uint32_t findFreeBlock(uint32_t inode);
    uint32_t findFreeInode();

    void releaseBlock(uint32_t block);

    Inode *getInode(uint32_t num);

    void ensureFreeBlockBitmapLoaded(size_t group);
    void ensureFreeInodeBitmapLoaded(size_t group);
    void ensureInodeTableLoaded(size_t group);

    /** Our superblock. */
    Superblock *m_pSuperblock;

    /** Group descriptors, in a tree because each GroupDesc* may be in a different block. */
    GroupDesc **m_pGroupDescriptors;

    /** Inode tables, indexed by group descriptor. */
    Vector<size_t> *m_pInodeTables;
    /** Free inode bitmaps, indexed by group descriptor. */
    Vector<size_t> *m_pInodeBitmaps;
    /** Free block bitmaps, indexed by group descriptor. */
    Vector<size_t> *m_pBlockBitmaps;

    /** Size of a block. */
    uint32_t m_BlockSize;

    /** Size of an Inode. */
    uint32_t m_InodeSize;

    /** Number of group descriptors. */
    size_t m_nGroupDescriptors;

    /** Write lock - we're finding some inodes and updating the superblock and block group structures. */
    Mutex m_WriteLock;

    /** The root filesystem node. */
    File *m_pRoot;
};

#endif
