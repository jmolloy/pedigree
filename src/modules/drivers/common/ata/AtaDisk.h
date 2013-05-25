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

/** An ATA disk device. Most read/write commands get channeled upstream
 * to the controller, as it has to multiplex between multiple disks. */
class AtaDisk : public Disk
{
public:
    AtaDisk(class AtaController *pDev, bool isMaster, IoBase *commandRegs, IoBase *controlRegs, BusMasterIde *busMaster = 0);
    ~AtaDisk();

    virtual void getName(String &str)
    {
        str = m_pName;
    }

    /** Tries to detect if this device is present.
     * \return True if the device is present and was successfully initialised. */
    bool initialise();

    // These are the functions that others call - they add a request to the parent controller's queue.
    virtual uintptr_t read(uint64_t location);
    virtual void write(uint64_t location);
    virtual void align(uint64_t location);

    virtual void flush(uint64_t location);

    // These are the internal functions that the controller calls when it is ready to process our request.
    virtual uint64_t doRead(uint64_t location);
    virtual uint64_t doWrite(uint64_t location);

    // Internal write function, actually writes to the disk
    uint64_t internalWrite(uint64_t location, uint64_t nBytes, uintptr_t buffer);

    // Called by our controller when an IRQ has been received.
    // It may not actually apply to us!
    void irqReceived();

    // Is this an ATAPI device?
    inline virtual bool isAtapi()
    {
      return false;
    }

private:
    /** Sets the drive up for reading from address 'n' in LBA28 mode. */
    void setupLBA28(uint64_t n, uint32_t nSectors);
    /** Sets the drive up for reading from address 'n' in LBA48 mode. */
    void setupLBA48(uint64_t n, uint32_t nSectors);

    /** Is this the master device on the bus? */
    bool m_IsMaster;

    /** The result of the IDENTIFY command. */
    uint16_t m_pIdent[256];
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

    /** This mutex is released by the IRQ handler when an IRQ is received, to wake the working thread.
     * \todo A condvar would really be better here. */
    Mutex m_IrqReceived;

protected:
    /** Performs the SET FEATURES command. */
    bool setFeatures(uint8_t command, uint8_t countreg, uint8_t lowreg, uint8_t midreg, uint8_t hireg);

    /** Sector cache. */
    Cache m_Cache;

    uint64_t m_AlignPoints[8];
    size_t m_nAlignPoints;

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
