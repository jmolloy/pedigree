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

#ifndef FAT_SYMLINK_H
#define FAT_SYMLINK_H

#include <vfs/Symlink.h>
#include <time/Time.h>
#include <processor/types.h>
#include <utilities/String.h>

#include "FatFile.h"

/**
 * FAT Symlink.
 *
 * FAT itself does not support symlinks, so a Pedigree-specific extension has
 * been used. A filename with a suffix of '.__sym' will be turned into a symlink
 * and its contents will be considered the target of the link.
 */
class FatSymlink : public Symlink
{
public:
    FatSymlink(String name, Time::Timestamp accessedTime, Time::Timestamp modifiedTime, Time::Timestamp creationTime,
       uintptr_t inode, class Filesystem *pFs, size_t size, uint32_t dirClus = 0,
       uint32_t dirOffset = 0, File *pParent = 0);
    virtual ~FatSymlink() {}
    uint32_t getDirCluster()
    {
        return m_DirClus;
    }
    void setDirCluster(uint32_t custom)
    {
        m_DirClus = custom;
    }
    uint32_t getDirOffset()
    {
        return m_DirOffset;
    }
    void setDirOffset(uint32_t custom)
    {
        m_DirOffset = custom;
    }

    /** Reads from the file.
     *  \param[in] buffer Buffer to write the read data into. Can be null, in
     *      which case the data can be found by calling getPhysicalPage.
     *  \param[in] bCanBlock Whether or not the File can block when reading
     */
    virtual uint64_t read(uint64_t location, uint64_t size, uintptr_t buffer, bool bCanBlock = true);
    /** Writes to the file.
     *  \param[in] bCanBlock Whether or not the File can block when reading
     */
    virtual uint64_t write(uint64_t location, uint64_t size, uintptr_t buffer, bool bCanBlock = true);

private:
    uint32_t m_DirClus;
    uint32_t m_DirOffset;
};

#endif
