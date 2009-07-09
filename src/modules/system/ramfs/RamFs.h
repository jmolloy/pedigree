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
#ifndef RAMFS_H
#define RAMFS_H

/**\file  RamFs.h
 *\author Matthew Iselin
 *\date   Sun May 17 10:00:00 2009
 *\brief  An in-RAM filesystem. */

#include <vfs/VFS.h>
#include <vfs/Directory.h>
#include <processor/types.h>
#include <machine/Disk.h>

/** Defines a directory in the RamFS */
/// \todo Support nested directories
class RamDir : public Directory
{
private:
    RamDir(const RamDir &);
    RamDir& operator =(const RamDir&);
public:
    RamDir(String name, size_t inode, class Filesystem *pFs, File *pParent);
    virtual ~RamDir();

    uint64_t read(uint64_t location, uint64_t size, uintptr_t buffer)
    {
        return 0;
    }
    uint64_t write(uint64_t location, uint64_t size, uintptr_t buffer)
    {
        return 0;
    }

    void truncate()
    {}

    virtual void cacheDirectoryContents()
    {}

    virtual bool addEntry(String filename, File *pFile, size_t type);

    virtual bool removeEntry(File *pFile);

    void fileAttributeChanged()
    {};

private:
    // List of files we have available in this directory
    RadixTree<File*> m_FileTree;
};

/** Defines a filesystem that is completely in RAM. */
class RamFs : public Filesystem
{
public:
    RamFs();
    virtual ~RamFs();

    virtual bool initialise(Disk *pDisk);
    virtual File* getRoot()
    {
        return m_pRoot;
    }
    virtual String getVolumeLabel()
    {
        return String("ramfs");
    }
    virtual uint64_t read(File *pFile, uint64_t location, uint64_t size, uintptr_t buffer);
    virtual uint64_t write(File *pFile, uint64_t location, uint64_t size, uintptr_t buffer);
    virtual void truncate(File *pFile);
    virtual void fileAttributeChanged(File *pFile) {};
    virtual void cacheDirectoryContents(File *pFile) {};

protected:
    virtual bool createFile(File* parent, String filename, uint32_t mask);
    virtual bool createDirectory(File* parent, String filename);
    virtual bool createSymlink(File* parent, String filename, String value);
    virtual bool remove(File* parent, File* file);

    RamFs(const RamFs&);
    void operator =(const RamFs&);

    /** Root filesystem node. */
    File *m_pRoot;
};

#endif
