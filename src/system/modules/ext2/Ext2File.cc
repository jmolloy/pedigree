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

#include "Ext2File.h"
#include "Ext2Filesystem.h"
#include <syscallError.h>

Ext2File::Ext2File(String name, uintptr_t inode_num, Inode inode,
                   Ext2Filesystem *pFs, File *pParent) :
  File(name, LITTLE_TO_HOST32(inode.i_atime),
       LITTLE_TO_HOST32(inode.i_mtime),
       LITTLE_TO_HOST32(inode.i_ctime),
       inode_num,
       static_cast<Filesystem*>(pFs),
       LITTLE_TO_HOST32(inode.i_size), /// \todo Deal with >4GB files here.
       pParent),
  Ext2Node(inode_num, inode, pFs)
{
  uint32_t mode = LITTLE_TO_HOST32(inode.i_mode);
  uint32_t permissions;
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
  setUid(LITTLE_TO_HOST16(inode.i_uid));
  setGid(LITTLE_TO_HOST16(inode.i_gid));
}

Ext2File::~Ext2File()
{
}

uint64_t Ext2File::read(uint64_t location, uint64_t size, uintptr_t buffer)
{
  uint64_t ret = static_cast<Ext2Node*>(this)->read(location, size, buffer);
  m_Size = m_nSize;
  return ret;
}

uint64_t Ext2File::write(uint64_t location, uint64_t size, uintptr_t buffer)
{
  uint64_t ret = static_cast<Ext2Node*>(this)->write(location, size, buffer);
  m_Size = m_nSize;
  return ret;
}

void Ext2File::truncate()
{
  static_cast<Ext2Node*>(this)->truncate();
  m_Size = m_nSize;
}

void Ext2File::fileAttributeChanged()
{
  static_cast<Ext2Node*>(this)->fileAttributeChanged(m_Size, m_AccessedTime, m_ModifiedTime, m_CreationTime);
}
