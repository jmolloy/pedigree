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

/// \file msdos.h Declares functions for probing Disk devices for MS-DOS partition tables.

#ifndef APPLE_H
#define APPLE_H

#include <machine/Device.h>
#include <machine/Disk.h>

#define APPLE_PM_SIG 0x504D // "PM"

typedef struct
{
  uint16_t pmSig;       // Partition signature: "PM"
  uint16_t pmSigPad;    // Reserved - signature padding.
  uint32_t pmMapBlkCnt; // Number of (512 byte) blocks in the partition map.
  uint32_t pmPyPartStart; // First physical block of partition.
  uint32_t pmPartBlkCnt; // Number of blocks in partition.
  char pmPartName[32];  // Partition name.
  char pmParType[32];   // Partition type.
  // We don't care about the rest of the partition map entry.
} ApplePartitionMap;

/** Attempts to find a MS-DOS partition table on pDisk. If found, new Partition objects are created
 * and added as children of pDisk.
 * \return true if a ms-dos partition table was found.
 */
bool appleProbeDisk(Disk *pDisk);

#endif
