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

#include "Ext2File.h"
#include "Ext2Filesystem.h"
#include "Ext2Symlink.h"
#include <syscallError.h>

Ext2File::Ext2File(String name, uintptr_t inode_num, Inode *inode,
                   Ext2Filesystem *pFs, File *pParent) :
    File(name, LITTLE_TO_HOST32(inode->i_atime),
         LITTLE_TO_HOST32(inode->i_mtime),
         LITTLE_TO_HOST32(inode->i_ctime),
         inode_num,
         static_cast<Filesystem*>(pFs),
         LITTLE_TO_HOST32(inode->i_size), /// \todo Deal with >4GB files here.
         pParent),
    Ext2Node(inode_num, inode, pFs),
    m_FileBlockCache()
{
    // Enable cache writebacks for this file.
    m_FileBlockCache.setCallback(writeCallback, static_cast<File*>(this));

    uint32_t mode = LITTLE_TO_HOST32(inode->i_mode);
    uint32_t permissions = 0;
    if (mode & EXT2_S_IRUSR) permissions |= FILE_UR;
    if (mode & EXT2_S_IWUSR) permissions |= FILE_UW;
    if (mode & EXT2_S_IXUSR) permissions |= FILE_UX;
    if (mode & EXT2_S_IRGRP) permissions |= FILE_GR;
    if (mode & EXT2_S_IWGRP) permissions |= FILE_GW;
    if (mode & EXT2_S_IXGRP) permissions |= FILE_GX;
    if (mode & EXT2_S_IROTH) permissions |= FILE_OR;
    if (mode & EXT2_S_IWOTH) permissions |= FILE_OW;
    if (mode & EXT2_S_IXOTH) permissions |= FILE_OX;

    setPermissions(permissions);
    setUid(LITTLE_TO_HOST16(inode->i_uid));
    setGid(LITTLE_TO_HOST16(inode->i_gid));
}

Ext2File::~Ext2File()
{
}

uintptr_t Ext2File::readBlock(uint64_t location)
{
    m_FileBlockCache.startAtomic();
    uintptr_t buffer = m_FileBlockCache.insert(location);
    static_cast<Ext2Node*>(this)->doRead(location, getBlockSize(), buffer);

    // Clear any dirty flag that may have been applied to the buffer
    // by performing this read. Cache uses the dirty flag to figure
    // out whether or not to write the block back to disk...
    VirtualAddressSpace &va = Processor::information().getVirtualAddressSpace();
    for (size_t off = 0; off < getBlockSize(); off += PhysicalMemoryManager::getPageSize())
    {
        void *p = reinterpret_cast<void *>(buffer + off);
        if(va.isMapped(p))
        {
            physical_uintptr_t phys = 0;
            size_t flags = 0;
            va.getMapping(p, phys, flags);

            if(flags & VirtualAddressSpace::Dirty)
            {
                flags &= ~(VirtualAddressSpace::Dirty);
                va.setFlags(p, flags);
            }
        }
    }
    m_FileBlockCache.endAtomic();

    return buffer;
}

void Ext2File::writeBlock(uint64_t location, uintptr_t addr)
{
    // Don't accidentally extend the file when writing the block.
    size_t sz = getBlockSize();
    uint64_t end = location + sz;
    if(end > getSize())
        sz = getSize() - location;

    static_cast<Ext2Node*>(this)->doWrite(location, sz, addr);
}

void Ext2File::truncate()
{
    // Wipe all our blocks. (Ext2Node).
    Ext2Node::wipe();

    // Clear caches.
    m_DataCache.clear();
    // empty() wipes out the cache, and outright ignores refcounts.
    m_FileBlockCache.empty();
    m_Size = m_nSize;
}

void Ext2File::fileAttributeChanged()
{
    static_cast<Ext2Node*>(this)->fileAttributeChanged(m_Size, m_AccessedTime, m_ModifiedTime, m_CreationTime);

    uint32_t mode = 0;
    uint32_t permissions = getPermissions();
    if (permissions & FILE_UR) mode |= EXT2_S_IRUSR;
    if (permissions & FILE_UW) mode |= EXT2_S_IWUSR;
    if (permissions & FILE_UX) mode |= EXT2_S_IXUSR;
    if (permissions & FILE_GR) mode |= EXT2_S_IRGRP;
    if (permissions & FILE_GW) mode |= EXT2_S_IWGRP;
    if (permissions & FILE_GX) mode |= EXT2_S_IXGRP;
    if (permissions & FILE_OR) mode |= EXT2_S_IROTH;
    if (permissions & FILE_OW) mode |= EXT2_S_IWOTH;
    if (permissions & FILE_OX) mode |= EXT2_S_IXOTH;
    static_cast<Ext2Node*>(this)->updateMetadata(getUid(), getGid(), mode);
}

void Ext2File::sync(size_t offset, bool async)
{
    m_FileBlockCache.sync(offset, async);
}

void Ext2File::pinBlock(uint64_t location)
{
    m_FileBlockCache.pin(location);
}

void Ext2File::unpinBlock(uint64_t location)
{
    m_FileBlockCache.release(location);
}
