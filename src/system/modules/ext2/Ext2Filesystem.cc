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

#include "Ext2Filesystem.h"
#include <vfs/VFS.h>
#include <Module.h>
#include <utilities/utility.h>
#include <Log.h>
#include <utilities/List.h>
#include <processor/Processor.h>
#include <utilities/StaticString.h>
#include <syscallError.h>

Ext2Filesystem::Ext2Filesystem() :
  m_pDisk(0), m_Superblock(), m_pGroupDescriptors(0), m_BlockSize(0), m_WriteLock(false),
  m_SuperblockDirty(false)
{
}

Ext2Filesystem::~Ext2Filesystem()
{
}

bool Ext2Filesystem::initialise(Disk *pDisk)
{
  m_pDisk = pDisk;

  // Attempt to read the superblock.
  uint8_t buffer[512];
  m_pDisk->read(1024, 512,
                reinterpret_cast<uintptr_t> (buffer));

  memcpy(reinterpret_cast<void*> (&m_Superblock), reinterpret_cast<void*> (buffer), sizeof(Superblock));

  // Read correctly?
  if (LITTLE_TO_HOST16(m_Superblock.s_magic) != 0xEF53)
  {
    String devName;
    pDisk->getName(devName);
    ERROR("Ext2: Superblock not found on device " << devName);
    return false;
  }

  // Calculate the block size.
  m_BlockSize = 1024 << LITTLE_TO_HOST32(m_Superblock.s_log_block_size);

  // Where is the group descriptor table?
  /// \todo Logic needs improvement - surely there's a specific way to calculate this?
  uint32_t gdBlock = 1;
  if (m_BlockSize == 1024)
    gdBlock = 2;

  // How large is the group descriptor table?
  uint32_t gdSize = m_BlockSize;
  while (gdSize <= (( LITTLE_TO_HOST32(m_Superblock.s_inodes_count) / LITTLE_TO_HOST32(m_Superblock.s_inodes_per_group) )
                    * sizeof(GroupDesc) ))
    gdSize += m_BlockSize;

  // Load the group descriptor table.
  uint8_t *tmp = new uint8_t[gdSize];
  m_pDisk->read(m_BlockSize*gdBlock, gdSize,
                reinterpret_cast<uintptr_t> (tmp));
  m_pGroupDescriptors = reinterpret_cast<GroupDesc*> (tmp);

  return true;
}

Filesystem *Ext2Filesystem::probe(Disk *pDisk)
{
  Ext2Filesystem *pFs = new Ext2Filesystem();
  if (!pFs->initialise(pDisk))
  {
    delete pFs;
    return 0;
  }
  else
  {
    return pFs;
  }
}

File Ext2Filesystem::getRoot()
{
  return File(String(""), 0, 0, 0, EXT2_ROOT_INO, false, true, this, 0);
}

String Ext2Filesystem::getVolumeLabel()
{
  if (m_Superblock.s_volume_name[0] == '\0')
  {
    NormalStaticString str;
    str += "no-volume-label@";
    str.append(reinterpret_cast<uintptr_t>(this), 16);
    return String(static_cast<const char*>(str));
  }
  char buffer[17];
  strncpy(buffer, m_Superblock.s_volume_name, 16);
  buffer[16] = '\0';
  return String(buffer);
}

void Ext2Filesystem::fileAttributeChanged(File *pFile)
{
}

void Ext2Filesystem::truncate(File *pFile)
{
}

bool Ext2Filesystem::createFile(File parent, String filename, uint32_t mask)
{
  // Grab the parent's inode.
  Inode inode = getInode(parent.getInode());

  // Sanity check - the parent must be a directory!
  if (!parent.isDirectory())
  {
    ERROR("EXT2: createFile called with non-directory as parent!");
    return false;
  }

  // And the filename must be non-NULL, and not "." or "..".
  if (strlen(filename) == 0 || !strcmp(filename, ".") || !strcmp(filename, ".."))
  {
    ERROR("EXT2: createFile: filename was NULL, '.' or '..'!");
    return false;
  }

  // We're all good. Let's find a new inode.
  uint32_t node_num = findFreeInodeInBitmap();
  if (node_num == 0)
  {
    SYSCALL_ERROR(NoSpaceLeftOnDevice);
    return false;
  }

  // Got one, let's populate it!
  Inode newInode;
  newInode.i_mode = mask | EXT2_FILE;
  newInode.i_uid = 0;
  newInode.i_size = 0;
  newInode.i_atime = 0;
  newInode.i_ctime = 0;
  newInode.i_mtime = 0;
  newInode.i_dtime = 0;
  newInode.i_gid = 0;
  newInode.i_links_count = 1;
  newInode.i_blocks = 0;
  newInode.i_flags = 0;
  newInode.i_osd1 = 0;
  newInode.i_generation = 0;
  newInode.i_file_acl = 0;
  newInode.i_dir_acl = 0;
  newInode.i_faddr = 0;
  
  // Write back.
  setInode(node_num, newInode);

  // Inode created, now it needs to be added to the parent's directory structure.
  addDirectoryEntry(parent.getInode(), node_num, filename, EXT2_FILE);

  return true;
}

bool Ext2Filesystem::createDirectory(File parent, String filename)
{
}

bool Ext2Filesystem::createSymlink(File parent, String filename, String value)
{
}

bool Ext2Filesystem::remove(File parent, File file)
{
}

void initExt2()
{
  VFS::instance().addProbeCallback(&Ext2Filesystem::probe);
}

void destroyExt2()
{
}

MODULE_NAME("ext2");
MODULE_ENTRY(&initExt2);
MODULE_EXIT(&destroyExt2);
MODULE_DEPENDS("VFS");

