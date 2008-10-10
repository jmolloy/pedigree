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
#ifndef EXT2FILESYSTEM_H
#define EXT2FILESYSTEM_H

#include <vfs/Filesystem.h>
#include <utilities/List.h>

#define EXT2_UNKNOWN   0x0
#define EXT2_FILE      0x1
#define EXT2_DIRECTORY 0x2
#define EXT2_CHAR_DEV  0x3
#define EXT2_BLOCK_DEV 0x4
#define EXT2_FIFO      0x5
#define EXT2_SOCKET    0x6
#define EXT2_SYMLINK   0x7
#define EXT2_MAX       0x8

#define EXT2_BAD_INO         0x01 // Bad blocks inode
#define EXT2_ROOT_INO        0x02 // root directory inode
#define EXT2_ACL_IDX_INO     0x03 // ACL index inode (deprecated?)
#define EXT2_ACL_DATA_INO    0x04 // ACL data inode (deprecated?)
#define EXT2_BOOT_LOADER_INO 0x05 // boot loader inode
#define EXT2_UNDEL_DIR_INO   0x06

/** This class provides an implementation of the second extended filesystem. */
class Ext2Filesystem : public Filesystem
{
public:
  Ext2Filesystem();

  virtual ~Ext2Filesystem();

  //
  // Filesystem interface.
  //

  virtual bool initialise(Disk *pDisk);
  static Filesystem *probe(Disk *pDisk);
  virtual File getRoot();
  virtual String getVolumeLabel();
  virtual uint64_t read(File *pFile, uint64_t location, uint64_t size, uintptr_t buffer);
  virtual uint64_t write(File *pFile, uint64_t location, uint64_t size, uintptr_t buffer);
  virtual void truncate(File *pFile);
  virtual void fileAttributeChanged(File *pFile);
  virtual File getDirectoryChild(File *pFile, size_t n);

protected:

  virtual bool createFile(File parent, String filename);
  virtual bool createDirectory(File parent, String filename);
  virtual bool createSymlink(File parent, String filename, String value);
  virtual bool remove(File parent, File file);

  /** The Ext2 superblock structure. */
  struct Superblock
  {
    uint32_t s_inodes_count;
    uint32_t s_blocks_count;
    uint32_t s_r_blocks_count;
    uint32_t s_free_blocks_count;
    uint32_t s_free_inodes_count;
    uint32_t s_first_data_block;
    uint32_t s_log_block_size;
    uint32_t s_log_frag_size;
    uint32_t s_blocks_per_group;
    uint32_t s_frags_per_group;
    uint32_t s_inodes_per_group;
    uint32_t s_mtime;
    uint32_t s_wtime;
    uint16_t s_mnt_count;
    uint16_t s_max_mnt_count;
    uint16_t s_magic;
    uint16_t s_state;
    uint16_t s_errors;
    uint16_t s_minor_rev_level;
    uint32_t s_lastcheck;
    uint32_t s_checkinterval;
    uint32_t s_creator_os;
    uint32_t s_rev_level;
    uint32_t s_def_resuid;
    uint32_t s_def_resgid;
//   -- EXT2_DYNAMIC_REV Specific --
    //uint32_t s_first_ino; // - Should this actually be commented out? it makes the volume name align up properly... :S
    uint16_t s_inode_size;
    uint16_t s_block_group_nr;
    uint32_t s_feature_compat;
    uint32_t s_feature_incompat;
    uint32_t s_feature_ro_compat;
    char s_uuid[16];

    char s_volume_name[16];
    char s_last_mounted[64];
    uint32_t s_algo_bitmap;
//   -- Performance Hints         --
    uint8_t s_prealloc_blocks;
    uint8_t s_prealloc_dir_blocks;
    uint16_t alignment;
//   -- Journaling Support        --
    char s_journal_uuid[16];
    uint32_t s_journal_inum;
    uint32_t s_journal_dev;
    uint32_t s_last_orphan;
  } __attribute__((packed));

  /** The ext2 block group descriptor structure */
  struct GroupDesc
  {
    uint32_t bg_block_bitmap;
    uint32_t bg_inode_bitmap;
    uint32_t bg_inode_table;
    uint16_t bg_free_blocks_count;
    uint16_t bg_free_inodes_count;
    uint16_t bg_used_dirs_count;
    uint16_t bg_pad;
    uint8_t  bg_reserved[12];
  } __attribute__((packed));

  /** An ext2 Inode. */
  struct Inode
  {
    uint16_t i_mode;
    uint16_t i_uid;
    uint32_t i_size;
    uint32_t i_atime;
    uint32_t i_ctime;
    uint32_t i_mtime;
    uint32_t i_dtime;
    uint16_t i_gid;
    uint16_t i_links_count;
    uint32_t i_blocks;
    uint32_t i_flags;
    uint32_t i_osd1;
    uint32_t i_block[15];
    uint32_t i_generation;
    uint32_t i_file_acl;
    uint32_t i_dir_acl;
    uint32_t i_faddr;
    uint8_t  i_osd2[12];
  } __attribute__((packed));

  /** An ext2 directory entry. */
  struct Dir
  {
    uint32_t d_inode;
    uint16_t d_reclen;
    uint8_t  d_namelen;
    uint8_t  d_file_type;
    char     d_name[256];
  } __attribute__((packed));

  Ext2Filesystem(const Ext2Filesystem&);
  void operator =(const Ext2Filesystem&);

  /** Obtains a given numbered Inode. */
  Inode getInode(uint32_t inode);

  /** Reads a block of data from the disk. */
  bool readBlock(uint32_t block, uintptr_t buffer);

  /** Obtains an array of block numbers for the given inode, given the starting and
      ending block indices. */
  void getBlockNumbers(Inode inode, uint32_t startBlock, uint32_t endBlock, List<uint32_t*> &list);

  void getBlockNumbersIndirect(uint32_t inode_block, int32_t startBlock, int32_t endBlock, List<uint32_t*> &list);
  void getBlockNumbersBiindirect(uint32_t inode_block, int32_t startBlock, int32_t endBlock, List<uint32_t*> &list);

  void readInodeData(Inode inode, uintptr_t buffer, uint32_t startBlock, uint32_t endBlock);
  
  /** Our raw device. */
  Disk *m_pDisk;

  /** Our superblock. */
  Superblock m_Superblock;

  /** Group descriptors. */
  GroupDesc *m_pGroupDescriptors;

  /** Size of a block. */
  uint32_t m_BlockSize;
};

#endif
