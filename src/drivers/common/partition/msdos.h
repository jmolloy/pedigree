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

#ifndef MSDOS_H
#define MSDOS_H

#include <machine/Device.h>
#include <machine/Disk.h>

// Identification of an MBR - 0x55 0xAA in bytes 510 and 511 respectively.
#define MSDOS_IDENT_1 0x55
#define MSDOS_IDENT_2 0xAA

/** An MS-DOS partition table entry. */
typedef struct
{
  uint8_t active;
  uint8_t start_head;
  uint8_t start_cylinder_low;
  uint8_t start_cylinder_high;
  uint8_t type;
  uint8_t end_head;
  uint8_t end_cylinder_low;
  uint8_t end_cylinder_high;
  uint32_t start_lba;
  uint32_t size;
} __attribute__((packed)) MsdosPartitionInfo;

/** Attempts to find a MS-DOS partition table on pDisk. If found, new MsdosPartition objects are created
 * and added as children of pDisk.
 * \return true if a ms-dos partition table was found.
 */
bool msdosProbeDisk(Disk *pDisk);


#endif
