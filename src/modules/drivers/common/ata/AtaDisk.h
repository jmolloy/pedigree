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

#ifndef ATA_ATA_DISK_H
#define ATA_ATA_DISK_H

#include <processor/types.h>
#include <machine/Device.h>
#include <machine/Disk.h>
#include <machine/Controller.h>
#include <process/Mutex.h>
#include <utilities/Cache.h>
#include <processor/MemoryRegion.h>
#include <processor/PhysicalMemoryManager.h>
#include "BusMasterIde.h"

#include <scsi/ScsiDisk.h>

#include "ata-common.h"

/**
 * An ATA device.
 *
 * This could be anything from an ATAPI device (eg, CDROM) to a pure and simple
 * IDE disk. We inherit ScsiDisk as it is needed for ATAPI. Non-ATAPI disks
 * simply don't use the ScsiDisk interface.
 */
class AtaDisk : public ScsiDisk
{
private:
    enum AtaDiskType
    {
        Block = 0x00,
        Sequential = 0x01,
        Printer = 0x02,
        Processor = 0x03,
        WriteOnce = 0x04,
        CdDvd = 0x05,
        OpticalMemory = 0x07,
        MediumChanger = 0x08,
        Raid = 0x0C,
        Enclosure = 0x0D,
        None = 0x1F,
        NotPacket = 0xFF,
    };

public:
    AtaDisk(class AtaController *pDev, bool isMaster, IoBase *commandRegs,
            IoBase *controlRegs, BusMasterIde *busMaster = 0);
    virtual ~AtaDisk();

    virtual void getName(String &str)
    {
        str = m_pName;
    }

    /**
     * Initialises the device and detects its features, if it is present.
     * \return True if the device is present and was successfully initialised.
     */
    bool initialise(size_t nUnit = ~0);

    // Do-ers for read/write operations. For PACKET devices, these forward to
    // the ScsiDisk implementations. For non-PACKET devices, these perform real
    // disk I/O.
    virtual uint64_t doRead(uint64_t location);
    virtual uint64_t doWrite(uint64_t location);

    /** Called when an IRQ is received by the controller. */
    virtual void irqReceived();

    /** Retrieve the BusMaster IDE interface for this disk. */
    virtual BusMasterIde *getBusMaster() const
    {
        return m_BusMaster;
    }

    virtual size_t getSize() const;
    virtual size_t getBlockSize() const;
    virtual size_t getNativeBlockSize() const;
    virtual size_t getBlockCount() const;

    // SCSI controller interface calls this to send a PACKET command.
    virtual bool sendCommand(size_t nUnit, uintptr_t pCommand, uint8_t nCommandSize, uintptr_t pRespBuffer, uint16_t nRespBytes, bool bWrite);
private:
    /** Sets the drive up for reading from address 'n' in LBA28 mode. */
    void setupLBA28(uint64_t n, uint32_t nSectors);
    /** Sets the drive up for reading from address 'n' in LBA48 mode. */
    void setupLBA48(uint64_t n, uint32_t nSectors);

    /** Default block size for this device. */
    virtual size_t defaultBlockSize()
    {
        if(m_AtaDiskType == CdDvd)
        {
            return 2048;
        }
        else
        {
            return 512;
        }
    }

    /** Type for read/write buffer lists. */
    struct Buffer
    {
        /// Virtual address of buffer to read into (page sized).
        uintptr_t buffer;

        /// Offset into the read.
        size_t offset;
    };

    /** Is this the master device on the bus? */
    bool m_IsMaster;

    /** The result of the IDENTIFY command. */
    IdentifyData m_pIdent;
    /** The model name of the device. */
    char m_pName[64];
    /** The serial number of the device. */
    char m_pSerialNumber[64];
    /** The firmware revision */
    char m_pFirmwareRevision[64];
    /** Does the device support LBA28? */
    bool m_SupportsLBA28;
    /** Does the device support LBA48? */
    bool m_SupportsLBA48;
    /** Block size in bytes for all read/write operations. */
    size_t m_BlockSize;

    /**
     * When we wait for an IRQ, we create a Mutex and assign it here. When an
     * IRQ fires, the mutex is released and the locked thread can continue.
     * \todo A condvar would really be better here.
     */
    Mutex *m_IrqReceived;

    /** What type of disk are we? */
    AtaDiskType m_AtaDiskType;

    /** Command packet size */
    uint8_t m_PacketSize;

    /** Device flags */
    bool m_Removable;

    /** Performs the SET FEATURES command. */
    void setFeatures(uint8_t command, uint8_t countreg, uint8_t lowreg, uint8_t midreg, uint8_t hireg);

    /** Command & control registers, and DMA access for this disk */
    IoBase *m_CommandRegs;
    IoBase *m_ControlRegs;
    BusMasterIde *m_BusMaster;

    /** PRD table lock (only used when grabbing the offset) */
    Mutex m_PrdTableLock;

    /** PRD table (virtual) */
    PhysicalRegionDescriptor *m_PrdTable;

    /** Last used offset into the PRD table (so we can run multiple ops at once) */
    size_t m_LastPrdTableOffset;

    /** PRD table (physical) */
    physical_uintptr_t m_PrdTablePhys;

    /** MemoryRegion for the PRD table */
    MemoryRegion m_PrdTableMemRegion;

    /** Can we do DMA? */
    bool m_bDma;
};

#endif
