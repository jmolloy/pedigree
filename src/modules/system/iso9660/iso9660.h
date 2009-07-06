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
#ifndef ISO9660_H
#define ISO9660_H

#include <utilities/PointerGuard.h>
#include <processor/types.h>

/** ISO 9660 Types */

struct Iso9660Timestamp
{
  uint8_t Year[4];
  uint8_t Month[2];
  uint8_t Day[2];
  uint8_t Hour[2];
  uint8_t Minute[2];
  uint8_t Second[2];
  uint8_t CentiSeconds[2];
  uint8_t Offset;
} __attribute__((packed));

struct Iso9660DirTimestamp
{
  uint8_t Year;
  uint8_t Month;
  uint8_t Day;
  uint8_t Hour;
  uint8_t Minute;
  uint8_t Second;
  uint8_t Offset;
} __attribute__((packed));

struct Iso9660VolumeDescriptor
{
  uint8_t Type;
  uint8_t Ident[5];
  uint8_t Version;
} __attribute__((packed));

struct Iso9660VolumeDescriptorPrimary
{
  Iso9660VolumeDescriptor Header;

  uint8_t   Unused1;

  uint8_t   SysIdent[32];
  uint8_t   VolIdent[32];

  uint8_t   Unused2[8];

  uint32_t  VolSpaceSize_LE; // little-endian
  uint32_t  VolSpaceSize_BE; // big-endian

  uint8_t   Unused3_EscSequences[32]; // Supplementary descriptors use this field

  uint16_t  VolSetSize_LE;
  uint16_t  VolSetSize_BE;

  uint16_t  VolSeqNum_LE;
  uint16_t  VolSeqNum_BE;

  uint16_t  LogicalBlockSize_LE;
  uint16_t  LogicalBlockSize_BE;

  uint32_t  PathTableSize_LE;
  uint32_t  PathTableSize_BE;

  uint32_t  TypeLPathTableOccurence;
  uint32_t  TypeLPathTableOptionOccurence;
  uint32_t  TypeMPathTableOccurence;
  uint32_t  TypeMPathTableOptionOccurence;

  uint8_t   RootDirRecord[34];

  uint8_t   VolSetIdent[128];
  uint8_t   PublisherIdent[128];
  uint8_t   DataPreparerIdent[128];
  uint8_t   ApplicationIdent[128];
  uint8_t   CopyrightFileIdent[37];
  uint8_t   AbstractFileIdent[37];
  uint8_t   BiblioFileIdent[37];

  Iso9660Timestamp VolumeCreationTime;
  Iso9660Timestamp VolumeModificationTime;
  Iso9660Timestamp VolumeExpiryTime;
  Iso9660Timestamp VolumeEffectiveTime;

  uint8_t FileStructVersion;

  uint8_t Rsvd1;

  uint8_t ApplicationUse[512];

  uint8_t Rsvd2[653];
} __attribute__((packed));

struct Iso9660DirRecord
{
  uint8_t RecLen;

  uint8_t ExtAttrRecordLen;

  uint32_t ExtentLocation_LE;
  uint32_t ExtentLocation_BE;

  uint32_t DataLen_LE;
  uint32_t DataLen_BE;

  Iso9660DirTimestamp Time;

  uint8_t FileFlags;
  uint8_t FileUnitSize;
  uint8_t InterleaveGapSize;

  uint16_t VolSeqNum_LE;
  uint16_t VolSeqNum_BE;

  uint8_t FileIdentLen;
  uint8_t FileIdent[];
} __attribute__((packed));

/** ISO9660 Defines */
#define PRIM_VOL_DESC     1
#define SUPP_VOL_DESC     2
#define TERM_VOL_DESC     255

/** Useful functions */
String WideToMultiByteStr(uint8_t *in, size_t inLen, size_t maxLen);

#endif
