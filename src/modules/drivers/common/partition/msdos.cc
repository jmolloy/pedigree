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

#include "msdos.h"
#include <processor/Processor.h>
#include <Log.h>
#include "Partition.h"
#include <Spinlock.h>
#include <LockGuard.h>

// Partition number for a DOS extended partition (another partition table)
const uint8_t g_ExtendedPartitionNumber = 5;
// Partition number for an empty partition.
const uint8_t g_EmptyPartitionNumber = 0;

Spinlock g_Lock;

const char *g_pPartitionTypes[256] = {
    "Empty",
    "FAT12",
    "XENIX root",
    "XENIX usr",
    "FAT16 <32M",
    "Extended",
    "FAT16",
    "HPFS/NTFS",
    "AIX",
    "AIX bootable",
    "OS/2 Boot Manag",
    "W95 FAT32",
    "W95 FAT32 (LBA)",
    "",
    "W95 FAT16 (LBA)",
    "W95 Ext",
    "OPUS",
    "Hidden FAT12",
    "Compaq diagnost",
    "",
    "Hidden FAT16 <3",
    "",
    "Hidden FAT16",
    "Hidden HPFS/NTF",
    "AST SmartSleep",
    "",
    "",
    "Hidden W95 FAT3",
    "Hidden W95 FAT3",
    "",
    "Hidden W95 FAT1",
    "",
    "",
    "",
    "",
    "",
    "NEC DOS",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "Plan 9",
    "",
    "",
    "PartitionMagic",
    "",
    "",
    "",
    "Venix 80286",
    "PPC PReP Boot",
    "SFS",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "QNX4.x",
    "QNX4.x 2nd part",
    "QNX4.x 3rd part",
    "OnTrack DM",
    "OnTrack DM6 Aux",
    "CP/M",
    "OnTrack DM6 Aux",
    "OnTrackDM6",
    "EZ-Drive",
    "Golden Bow",
    "",
    "",
    "",
    "",
    "",
    "Priam Edisk",
    "",
    "",
    "",
    "",
    "SpeedStor",
    "",
    "GNU HURD or Sys",
    "Novell Netware",
    "Novell Netware",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "DiskSecure Mult",
    "",
    "",
    "",
    "",
    "PC/IX",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "Old Minix",
    "Minix / old Lin",
    "Linux swap / So",
    "Linux",
    "OS/2 hidden C:",
    "Linux extended",
    "NTFS volume set",
    "NTFS volume set",
    "Linux plaintext",
    "",
    "",
    "",
    "",
    "",
    "Linux LVM",
    "",
    "",
    "",
    "",
    "Amoeba",
    "Amoeba BBT",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "BSD/OS",
    "IBM Thinkpad hi",
    "",
    "",
    "",
    "",
    "FreeBSD",
    "OpenBSD",
    "NeXTSTEP",
    "Darwin UFS",
    "NetBSD",
    "",
    "Darwin boot",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "BSDI fs",
    "BSDI swap",
    "",
    "",
    "Boot Wizard hid",
    "",
    "",
    "Solaris boot",
    "Solaris",
    "",
    "DRDOS/sec (FAT-",
    "",
    "",
    "DRDOS/sec (FAT-",
    "",
    "DRDOS/sec (FAT-",
    "Syrinx",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "Non-FS data",
    "CP/M / CTOS / .",
    "",
    "",
    "Dell Utility",
    "BootIt",
    "",
    "DOS access",
    "",
    "DOS R/O",
    "SpeedStor",
    "",
    "",
    "",
    "",
    "",
    "",
    "BeOS fs",
    "",
    "",
    "EFI GPT",
    "EFI (FAT-12/16/",
    "Linux/PA-RISC b",
    "SpeedStor",
    "DOS secondary",
    "",
    "SpeedStor",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "Linux RAID auto",
    "LANstep",
    "BBT"
};

/// Holds the next partition number to mount
static int gNextPartition = 0;

void msdosRegPartition(MsdosPartitionInfo *pPartitions, int i, Disk *pDisk)
{
    LockGuard<Spinlock> guard(g_Lock);

    // Look up the partition string.
    const char *pStr = g_pPartitionTypes[pPartitions[i].type];
    NormalStaticString sstr("(");
    sstr += gNextPartition++;
    sstr += ") ";
    sstr += pStr;
    String str(sstr);

    // Create a partition object.
    Partition *pObj = new Partition(str,
                                    static_cast<uint64_t>(LITTLE_TO_HOST32(pPartitions[i].start_lba))*512ULL, /* start_lba is in /sectors/. */
                                    static_cast<uint64_t>(LITTLE_TO_HOST32(pPartitions[i].size))*512ULL);
    pObj->setParent(static_cast<Device*> (pDisk));
    pDisk->addChild(static_cast<Device*> (pObj));
}

bool msdosReadExtTable(MsdosPartitionInfo *pPartitions, Disk *pDisk, int n, uint64_t partitionBase, uint64_t currentBase)
{
    for (int i = 0; i < MSDOS_EXT_PARTTAB_NUM; i++)
    {
        // Legit?
        if ((pPartitions[i].active != 0) && (pPartitions[i].active != 0x80))
        {
            WARNING("Invalid partition record found");
            continue;
        }

        // Check the type of the partition.
        if (pPartitions[i].type == g_ExtendedPartitionNumber)
        {
            // In a linked extended partition record, the LBA start is the difference between the start
            // of the actual extended partition and the next extended partition record's MBR sector.
            uint64_t startLba = LITTLE_TO_HOST32(pPartitions[i].start_lba) + partitionBase;

            // Update the partition information. Forget about turning it back into whatever
            // endianness it was in before.
            pPartitions[i].start_lba = static_cast<uint32_t>(startLba & 0xFFFFFFFF);

            // Extended partition - read in 512 bytes and recurse.
            uintptr_t buff;
            if ((buff = pDisk->read(pPartitions[i].start_lba * 512ULL)) == 0)
            {
                WARNING("Couldn't read next sector for the extended partition.");
                continue;
            }

            uint8_t *buffer = reinterpret_cast<uint8_t*>(buff);

            // Is it a "valid" MBR?
            if(buffer[510] != MSDOS_IDENT_1 || buffer[511] != MSDOS_IDENT_2)
            {
                WARNING("Extended partition record read failed.");
                continue;
            }

            // Call the extended partition reader. We pass in the current extended partition record's base,
            // along with the base of the extended partition record we're about to parse.
            MsdosPartitionInfo *pNextPartitions = reinterpret_cast<MsdosPartitionInfo*> (&buffer[MSDOS_PARTTAB_START]);
            if(!msdosReadExtTable(pNextPartitions, pDisk, MSDOS_PARTTAB_NUM, partitionBase, startLba))
                WARNING("Reading the extended partition table failed");
        }
        else if (pPartitions[i].type == g_EmptyPartitionNumber)
        {
            // Empty partition - end of chain
            return true;
        }
        else
        {
            // The start LBA of a logical partition is relative to the extended partition record which describes it
            uint64_t startLba = LITTLE_TO_HOST32(pPartitions[i].start_lba) + currentBase;

            pPartitions[i].start_lba = HOST_TO_LITTLE32(static_cast<uint32_t>(startLba & 0xFFFFFFFF));
            msdosRegPartition(pPartitions, i, pDisk);
        }
    }
    return true;
}

bool msdosReadTable(MsdosPartitionInfo *pPartitions, Disk *pDisk)
{
    for (int i = 0; i < MSDOS_PARTTAB_NUM; i++)
    {
        // Legit?
        if ((pPartitions[i].active != 0) && (pPartitions[i].active != 0x80))
        {
            WARNING("Invalid partition record found");
            continue;
        }

        // Check the type of the partition.
        if (pPartitions[i].type == g_ExtendedPartitionNumber)
        {
            uint64_t startLba = LITTLE_TO_HOST32(pPartitions[i].start_lba);

            // Extended partition - read in 512 bytes and recurse. The first sector will always be relative to this sector (zero).
            uintptr_t buff;
            if ((buff=pDisk->read(startLba * 512ULL)) == 0)
            {
                WARNING("Couldn't read next sector for the extended partition.");
                continue;
            }

            uint8_t *buffer = reinterpret_cast<uint8_t*>(buff);

            // Is it valid?
            if(buffer[510] != MSDOS_IDENT_1 || buffer[511] != MSDOS_IDENT_2)
            {
                WARNING("Extended partition record read failed.");
                continue;
            }

            // Call the extended partition reader, give it the base of this partition entry for its calculations.
            MsdosPartitionInfo *pPartitions = reinterpret_cast<MsdosPartitionInfo*> (&buffer[MSDOS_PARTTAB_START]);
            if(!msdosReadExtTable(pPartitions, pDisk, MSDOS_PARTTAB_NUM, startLba, startLba))
                WARNING("Reading the extended partition table failed");
        }
        else if (pPartitions[i].type == g_EmptyPartitionNumber)
        {
            // Empty partition - do nothing.
        }
        else
        {
            msdosRegPartition(pPartitions, i, pDisk);
        }
    }
    return true;
}

bool msdosProbeDisk(Disk *pDisk)
{
    // Read the first sector (512 bytes) of the disk into a buffer.
    uintptr_t buff;
    if ((buff=pDisk->read(0ULL)) == 0)
    {
        WARNING("Disk read failure during partition table search.");
        return false;
    }

    uint8_t *buffer = reinterpret_cast<uint8_t*>(buff);

    // Check for the magic bytes.
    if (buffer[510] != MSDOS_IDENT_1 || buffer[511] != MSDOS_IDENT_2)
    {
        NOTICE("MS-DOS partition not found.");
        return false;
    }

    String diskName;
    pDisk->getName(diskName);
    NOTICE("MS-DOS partition table found on disk " << diskName);

    MsdosPartitionInfo *pPartitions = reinterpret_cast<MsdosPartitionInfo*> (&buffer[MSDOS_PARTTAB_START]);
    return msdosReadTable(pPartitions, pDisk);
}
