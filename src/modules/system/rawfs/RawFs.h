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
#ifndef RAWFS_H
#define RAWFS_H

#include <process/Mutex.h>
#include <utilities/List.h>
#include <utilities/Tree.h>
#include <utilities/Vector.h>
#include <vfs/Directory.h>
#include <vfs/Filesystem.h>
#include <vfs/VFS.h>

/** Provides access to Disks without mounting their contained filesystem
    (raw device access). */
class RawFs : public Filesystem
{
public:
    RawFs();
    virtual ~RawFs();

    //
    // Filesystem interface.
    //
    virtual bool initialise(Disk *pDisk) {return false;}
    static Filesystem *probe (Disk *pDisk) {return 0;}
    virtual File *getRoot();
    virtual String getVolumeLabel() {return String("RawFs");}
    virtual uint64_t read(File *pFile, uint64_t location, uint64_t size, uintptr_t buffer, bool canBlock) {return 0;}
    virtual uint64_t write(File *pFile, uint64_t location, uint64_t size, uintptr_t buffer, bool canBlock) {return 0;}
    virtual void truncate(File *pFile) {return;}
    virtual void fileAttributeChanged(File *pFile) {return;}
    virtual void cacheDirectoryContents(File *pFile) {return;}

protected:
    virtual bool createFile(File* parent, String filename, uint32_t mask)
    {return false;}
    virtual bool createDirectory(File* parent, String filename)
    {return false;}
    virtual bool createSymlink(File* parent, String filename, String value)
    {return false;}
    virtual bool remove(File* parent, File* file)
    {return false;}

private:
    RawFs(const RawFs &);
    RawFs &operator = (const RawFs &);

    class RawFsDir *m_pRoot;
};

#endif

