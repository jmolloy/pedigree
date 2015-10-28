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

#include "Ext2Symlink.h"
#include "Ext2Filesystem.h"
#include <syscallError.h>
#include <utilities/utility.h>

Ext2Symlink::Ext2Symlink(String name, uintptr_t inode_num, Inode *inode,
                         Ext2Filesystem *pFs, File *pParent) :
    Symlink(name, LITTLE_TO_HOST32(inode->i_atime),
            LITTLE_TO_HOST32(inode->i_mtime),
            LITTLE_TO_HOST32(inode->i_ctime),
            inode_num,
            static_cast<Filesystem*>(pFs),
            LITTLE_TO_HOST32(inode->i_size), /// \todo Deal with >4GB files here.
            pParent),
    Ext2Node(inode_num, inode, pFs)
{
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

Ext2Symlink::~Ext2Symlink()
{
}

uint64_t Ext2Symlink::read(uint64_t location, uint64_t size, uintptr_t buffer, bool canBlock)
{
    if (location >= getSize())
        return 0;
    if ((location + size) >= getSize())
        size = getSize() - location;
    if (!size)
        return 0;

    if (getSize() && Ext2Node::getInode()->i_blocks == 0)
    {
        memcpy(reinterpret_cast<void *>(buffer), adjust_pointer(m_pInode->i_block, location), size);
        return size;
    }

    if (getSize() > m_pExt2Fs->m_BlockSize)
    {
        WARNING("Ext2: rather large symlink found, not handled yet");
        return 0;
    }

    uintptr_t block = Ext2Node::readBlock(location);
    size_t offset = location % m_pExt2Fs->m_BlockSize;
    memcpy(reinterpret_cast<void *>(buffer), reinterpret_cast<void *>(block + offset), size);
    m_Size = m_nSize;
    return size;
}

uint64_t Ext2Symlink::write(uint64_t location, uint64_t size, uintptr_t buffer, bool canBlock)
{
    Ext2Node::extend(size);
    m_Size = m_nSize;

    if (getSize() > m_pExt2Fs->m_BlockSize)
    {
        WARNING("Ext2: rather large symlink found, not handled yet");
        return 0;
    }

    uintptr_t block = Ext2Node::readBlock(location);
    size_t offset = location % m_pExt2Fs->m_BlockSize;
    memcpy(adjust_pointer(reinterpret_cast<void *>(block), offset), reinterpret_cast<void *>(buffer), size);
    Ext2Node::writeBlock(location);
    return size;
}

void Ext2Symlink::truncate()
{
    Ext2Node::wipe();
    m_Size = m_nSize;
}

void Ext2Symlink::fileAttributeChanged()
{
    static_cast<Ext2Node*>(this)->fileAttributeChanged(m_Size, m_AccessedTime, m_ModifiedTime, m_CreationTime);
}
