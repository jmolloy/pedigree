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

Ext2File::~Ext2File()
{
}

void Ext2File::extend(size_t newSize)
{
    Ext2Node::extend(newSize);
    m_Size = m_nSize;
}

void Ext2File::truncate()
{
    // Wipe all our blocks. (Ext2Node).
    Ext2Node::wipe();
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

uintptr_t Ext2File::readBlock(uint64_t location)
{
    return Ext2Node::readBlock(location);
}

void Ext2File::writeBlock(uint64_t location, uintptr_t addr)
{
    Ext2Node::writeBlock(location);
}

void Ext2File::pinBlock(uint64_t location)
{
    Ext2Node::pinBlock(location);
}

void Ext2File::unpinBlock(uint64_t location)
{
    Ext2Node::unpinBlock(location);
}

void Ext2File::sync(size_t offset, bool async)
{
    Ext2Node::sync(offset, async);
}
