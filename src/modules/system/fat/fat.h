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
#ifndef FAT_H
#define FAT_H

#include <utilities/PointerGuard.h>
#include <processor/types.h>

// FAT Attributes
#define ATTR_READONLY   0x01
#define ATTR_HIDDEN     0x02
#define ATTR_SYSTEM     0x04
#define ATTR_VOLUME_ID  0x08
#define ATTR_DIRECTORY  0x10
#define ATTR_ARCHIVE    0x20

#define ATTR_LONG_NAME      ( ATTR_READONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID )
#define ATTR_LONG_NAME_MASK ( ATTR_READONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID | ATTR_DIRECTORY | ATTR_ARCHIVE )

/** FAT type */
enum FatType
{
  FAT12 = 0, FAT16, FAT32
};

/** The Fat 'superblock', or boot parameter block as we call it normally
    Note that this is from the FAT whitepaper, so the names are as specified
      there (in order to avoid confusion)
    Note also that Superblock32 and Superblock16 could probably go in Superblock
      as a union (perhaps) */
struct Superblock
{
  uint8_t   BS_jmpBoot[3];
  uint8_t   BS_OEMName[8];
  uint16_t  BPB_BytsPerSec;
  uint8_t   BPB_SecPerClus;
  uint16_t  BPB_RsvdSecCnt;
  uint8_t   BPB_NumFATs;
  uint16_t  BPB_RootEntCnt;
  uint16_t  BPB_TotSec16;
  uint8_t	  BPB_Media;
  uint16_t  BPB_FATSz16;
  uint16_t  BPB_SecPerTrk;
  uint16_t  BPB_NumHeads;
  uint32_t  BPB_HiddSec;
  uint32_t  BPB_TotSec32;
} __attribute__((packed));

/** Special FAT BPB starting after the Superblock (FAT32) */
struct Superblock32
{
  uint32_t  BPB_FATSz32;
  uint16_t  BPB_ExtFlags;
  uint16_t  BPB_FsVer;
  uint32_t  BPB_RootClus;
  uint16_t  BPB_FsInfo;
  uint16_t  BPB_BkBootSec;
  uint8_t   BPB_Reserved[12];
  uint8_t   BS_DrvNum;
  uint8_t   BS_Reserved1;
  uint8_t   BS_BootSig;
  uint32_t  BS_VolID;
  uint8_t   BS_VolLab[11];
  uint8_t   BS_FilSysType[8];
} __attribute__((packed));

/** Special FAT BPB starting after the Superblock (FAT16/12) */
struct Superblock16
{
  uint8_t   BS_DrvNum;
  uint8_t   BS_Reserved1;
  uint8_t   BS_BootSig;
  uint32_t  BS_VolID;
  int8_t    BS_VolLab[11];
  int8_t    BS_FilSysType[8];
} __attribute__((packed));

/** FAT32 FSInfo structure */
struct FSInfo32
{
  uint32_t  FSI_LeadSig; // always 0x41615252
  uint8_t   FSI_Reserved1[480];
  uint32_t  FSI_StrucSig; // always 0x61417272
  uint32_t  FSI_Free_Count; // free cluster count, 0xFFFFFFFF if unknown
  uint32_t  FSI_NxtFree;
  uint8_t   FSI_Reserved2[12];
  uint32_t  FSI_TrailSig; // always 0xAA550000
} __attribute__((packed));

/** A Fat directory entry. */
struct Dir
{
  uint8_t   DIR_Name[11];
  uint8_t   DIR_Attr;
  uint8_t   DIR_NTRes;
  uint8_t   DIR_CrtTimeTenth;
  uint16_t  DIR_CrtTime;
  uint16_t  DIR_CrtDate;
  uint16_t	DIR_LstAccDate;
  uint16_t	DIR_FstClusHI;
  uint16_t	DIR_WrtTime;
  uint16_t	DIR_WrtDate;
  uint16_t	DIR_FstClusLO;
  uint32_t	DIR_FileSize;
} __attribute__((packed));

/** A Fat long filename directory entry. */
struct DirLongFilename
{
  uint8_t   LDIR_Ord;
  uint8_t   LDIR_Name1[10];
  uint8_t   LDIR_Attr;
  uint8_t   LDIR_Type; // always zero for LFN entries
  uint8_t   LDIR_Chksum;
  uint8_t   LDIR_Name2[12];
  uint16_t  LDIR_FstClusLO; // meaningless, and MUST be zero
  uint8_t   LDIR_Name3[4];
} __attribute__((packed));

/** A Fat timestamp */
struct Timestamp
{
  uint32_t secCount : 5;
  uint32_t minutes : 6;
  uint32_t hours : 5;
} __attribute__((packed));

/** A Fat date */
struct Date
{
  uint32_t day : 5;
  uint32_t month : 4;
  uint32_t years : 7; // years from 1980, not 1970
} __attribute__((packed));

/** Common FAT file information */
struct FatFileInfo
{
  Time accessedTime;
  Time modifiedTime;
  Time creationTime;
};

#endif

