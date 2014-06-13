/*
 * Copyright (c) 2008-2014, Pedigree Developers
 *
 * Please see the CONTRIB file in the root of the source tree for a full
 * list of contributors.
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

#include <utilities/Cache.h>

class RamFile : public File
{
    public:
        RamFile(String name, uintptr_t inode, Filesystem *pParentFS, File *pParent) :
            File(name, 0, 0, 0, inode, pParentFS, 0, pParent), m_FileBlocks(),
            m_nOwnerPid(0)
        {
            // Full permissions.
            setPermissions(0777);

            m_nOwnerPid = Processor::information().getCurrentThread()->getParent()->getId();
        }

        virtual ~RamFile()
        {
        }

        virtual void truncate()
        {
            if(canWrite())
            {
                // Empty the cache.
                m_FileBlocks.empty();
                setSize(0);
            }
        }

        virtual uint64_t write(uint64_t location, uint64_t size, uintptr_t buffer, bool bCanBlock = true)
        {
            if(canWrite())
                return File::write(location, size, buffer, bCanBlock);
            return 0;
        }

        bool canWrite();

    protected:
        virtual uintptr_t readBlock(uint64_t location)
        {
            // Super trivial. But we are a ram filesystem... can't compact.
            uintptr_t buffer = m_FileBlocks.insert(location);
            pinBlock(location);
            return buffer;
        }

        virtual void pinBlock(uint64_t location)
        {
            m_FileBlocks.pin(location);
        }

        virtual void unpinBlock(uint64_t location)
        {
            m_FileBlocks.release(location);
        }


    private:
        Cache m_FileBlocks;

        size_t m_nOwnerPid;
};

/** Defines a directory in the RamFS */
class RamDir : public Directory
{
private:
    RamDir(const RamDir &);
    RamDir& operator =(const RamDir&);
public:
    RamDir(String name, size_t inode, class Filesystem *pFs, File *pParent);
    virtual ~RamDir();

    virtual void cacheDirectoryContents()
    {
    }

    virtual bool addEntry(String filename, File *pFile);

    virtual bool removeEntry(File *pFile);
};

/** Defines a filesystem that is completely in RAM. */
class RamFs : public Filesystem
{
public:
    RamFs();
    virtual ~RamFs();

    virtual bool initialise(Disk *pDisk);

    void setProcessOwnership(bool bEnable)
    {
        m_bProcessOwners = bEnable;
    }

    bool getProcessOwnership() const
    {
        return m_bProcessOwners;
    }

    virtual File* getRoot()
    {
        return m_pRoot;
    }
    virtual String getVolumeLabel()
    {
        return String("ramfs");
    }

protected:
    virtual bool createFile(File* parent, String filename, uint32_t mask);
    virtual bool createDirectory(File* parent, String filename);
    virtual bool createSymlink(File* parent, String filename, String value);
    virtual bool remove(File* parent, File* file);

    RamFs(const RamFs&);
    void operator =(const RamFs&);

    /** Root filesystem node. */
    File *m_pRoot;

    bool m_bProcessOwners;
};

#endif
