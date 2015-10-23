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


#include "FatFilesystem.h"
#include "FatSymlink.h"

FatSymlink::FatSymlink(String name, Time::Timestamp accessedTime, Time::Timestamp modifiedTime,
        Time::Timestamp creationTime, uintptr_t inode, class Filesystem *pFs, size_t size,
        uint32_t dirClus, uint32_t dirOffset, File *pParent) :
    Symlink(name, accessedTime, modifiedTime, creationTime, inode, pFs, size,
        pParent), m_DirClus(dirClus), m_DirOffset(dirOffset)
{
    // No permissions on FAT - set all to RWX.
    setPermissions(
            FILE_UR | FILE_UW | FILE_UX |
            FILE_GR | FILE_GW | FILE_GX |
            FILE_OR | FILE_OW | FILE_OX);
}

uint64_t FatSymlink::read(uint64_t location, uint64_t size, uintptr_t buffer, bool bCanBlock)
{
    FatFilesystem *pFs = reinterpret_cast<FatFilesystem*>(m_pFilesystem);
    return pFs->read(this, location, size, buffer);
}

uint64_t FatSymlink::write(uint64_t location, uint64_t size, uintptr_t buffer, bool bCanBlock)
{
    FatFilesystem *pFs = reinterpret_cast<FatFilesystem*>(m_pFilesystem);
    uint64_t ret = pFs->write(this, location, size, buffer);

    // Reset the symlink target.
    initialise(true);

    return ret;
}
