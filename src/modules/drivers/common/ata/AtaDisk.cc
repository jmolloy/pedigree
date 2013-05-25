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

#include <processor/types.h>
#include <machine/Device.h>
#include <machine/Disk.h>
#include <machine/Controller.h>
#include <Log.h>
#include <processor/Processor.h>
#include <panic.h>
#include <machine/Machine.h>
#include <utilities/assert.h>
#include "AtaController.h"
#include "AtaDisk.h"
#include "ata-common.h"

// Note the IrqReceived mutex is deliberately started in the locked state.
AtaDisk::AtaDisk(AtaController *pDev, bool isMaster, IoBase *commandRegs, IoBase *controlRegs, BusMasterIde *busMaster) :
        Disk(), m_IsMaster(isMaster), m_SupportsLBA28(true), m_SupportsLBA48(false),
        m_IrqReceived(true), m_Cache(), m_nAlignPoints(0), m_CommandRegs(commandRegs),
        m_ControlRegs(controlRegs), m_BusMaster(busMaster), m_PrdTableLock(false), m_PrdTable(0),
        m_LastPrdTableOffset(0), m_PrdTablePhys(0), m_PrdTableMemRegion("ata-prdtable"), m_bDma(true)
{
    m_pParent = pDev;
}

AtaDisk::~AtaDisk()
{
}

bool AtaDisk::initialise()
{
    // Grab our parent.
    AtaController *pParent = static_cast<AtaController*> (m_pParent);

    // Grab our parent's IoPorts for command and control accesses.
    IoBase *commandRegs = m_CommandRegs;
    // Commented out - unused variable.
    // IoBase *controlRegs = m_ControlRegs;

    // Drive spin-up (go from standby to active, if necessary)
    setFeatures(0x07, 0, 0, 0, 0);

    // Check for device presence
    uint8_t devSelect = (m_IsMaster) ? 0xA0 : 0xB0;
    commandRegs->write8(devSelect, 6);
    commandRegs->write8(0xEC, 7);
    if(commandRegs->read8(7) == 0)
    {
        NOTICE("ATA: No device present here");
        return false;
    }

    // Select the device to transmit to
    devSelect = (m_IsMaster) ? 0xA0 : 0xB0;
    commandRegs->write8(devSelect, 6);

    // Wait for it to be selected
    ataWait(commandRegs);

    // DEVICE RESET
    commandRegs->write8(8, 7);

    // Wait for the drive to reset before requesting a device change
    ataWait(commandRegs);

    //
    // Start IDENTIFY command.
    //

    AtaStatus status;

    // Disable IRQs, for the moment.
    // controlRegs->write8(0x01, 6);

    // Send IDENTIFY.
    commandRegs->read8(7);
    commandRegs->write8(0xEC, 7);

    // Read status register.
    status = ataWait(commandRegs);

    // Check that the device actually exists
    if (status.__reg_contents == 0)
        return false;

    // If ERR was set we had an error
    if (status.reg.err)
    {
        WARNING("ATA drive errored on IDENTIFY!");
        return false;
    }

    // Read the data.
    for (int i = 0; i < 256; i++)
    {
        m_pIdent[i] = commandRegs->read16(0);
    }

    if(commandRegs->read8(7) & 1)
    {
        WARNING("ATA drive now has an error status after reading IDENTIFY data.");
        return false;
    }

    // Interpret the data.

    // Get the device name.
    for (int i = 0; i < 20; i++)
    {
#ifdef LITTLE_ENDIAN
        m_pName[i*2] = m_pIdent[0x1B+i] >> 8;
        m_pName[i*2+1] = m_pIdent[0x1B+i] & 0xFF;
#else
        m_pName[i*2] = m_pIdent[0x1B+i] & 0xFF;
        m_pName[i*2+1] = m_pIdent[0x1B+i] >> 8;
#endif
    }

    // The device name is padded by spaces. Backtrack through converting spaces into NULL bytes.
    for (int i = 39; i > 0; i--)
    {
        if (m_pName[i] != ' ')
            break;
        m_pName[i] = '\0';
    }
    m_pName[40] = '\0';


    // Get the serial number.
    for (int i = 0; i < 10; i++)
    {
#ifdef LITTLE_ENDIAN
        m_pSerialNumber[i*2] = m_pIdent[0x0A+i] >> 8;
        m_pSerialNumber[i*2+1] = m_pIdent[0x0A+i] & 0xFF;
#else
        m_pSerialNumber[i*2] = m_pIdent[0x0A+i] & 0xFF;
        m_pSerialNumber[i*2+1] = m_pIdent[0x0A+i] >> 8;
#endif
    }

    // The serial number is padded by spaces. Backtrack through converting spaces into NULL bytes.
    for (int i = 19; i > 0; i--)
    {
        if (m_pSerialNumber[i] != ' ')
            break;
        m_pSerialNumber[i] = '\0';
    }
    m_pSerialNumber[20] = '\0';

    // Get the firmware revision.
    for (int i = 0; i < 4; i++)
    {
#ifdef LITTLE_ENDIAN
        m_pFirmwareRevision[i*2] = m_pIdent[0x17+i] >> 8;
        m_pFirmwareRevision[i*2+1] = m_pIdent[0x17+i] & 0xFF;
#else
        m_pFirmwareRevision[i*2] = m_pIdent[0x17+i] & 0xFF;
        m_pFirmwareRevision[i*2+1] = m_pIdent[0x17+i] >> 8;
#endif
    }

    // The device name is padded by spaces. Backtrack through converting spaces into NULL bytes.
    for (int i = 7; i > 0; i--)
    {
        if (m_pFirmwareRevision[i] != ' ')
            break;
        m_pFirmwareRevision[i] = '\0';
    }
    m_pFirmwareRevision[8] = '\0';

    NOTICE("can transfer " << Dec << (LITTLE_TO_HOST16(m_pIdent[47]) & 0xFF) << Hex << " blocks per transfer");
    NOTICE("word 63: " << m_pIdent[63]);
    NOTICE("word 88: " << m_pIdent[88]);

    uint16_t word83 = LITTLE_TO_HOST16(m_pIdent[83]);
    if (word83 & (1<<10))
    {
        m_SupportsLBA48 = true;
    }


    // Any form of DMA support?
    if(!(m_pIdent[49] & (1 << 8)))
    {
        NOTICE("ATA: Device does not support DMA");
        m_bDma = false;
    }
    else if(!m_BusMaster)
    {
        NOTICE("ATA: Controller does not support DMA");
        m_bDma = false;
    }

    NOTICE("Detected ATA device '" << m_pName << "', '" << m_pSerialNumber << "', '" << m_pFirmwareRevision << "'");
    return true;
}

uintptr_t AtaDisk::read(uint64_t location)
{
    if (location % 512)
        FATAL("AtaDisk: write request not on a sector boundary!");

    /// \todo Bounds checking.

    // Grab our parent.
    AtaController *pParent = static_cast<AtaController*> (m_pParent);

    // Look through the align points.
    uint64_t alignPoint = 0;
    for (size_t i = 0; i < m_nAlignPoints; i++)
        if (m_AlignPoints[i] <= location && m_AlignPoints[i] > alignPoint)
            alignPoint = m_AlignPoints[i];

    // Calculate the offset to get location on a page boundary.
    ssize_t offs =  -((location - alignPoint) % 4096);

    // Create room in the cache.
    uintptr_t buffer;
    if ( (buffer=m_Cache.lookup(location+offs)) )
    {
        return buffer-offs;
    }

#if 0
    Timer &timer = *Machine::instance().getTimer();
    uint64_t now = timer.getTickCount();
    NOTICE("Started read request at " << Dec << now << Hex);
#endif

    pParent->addRequest(0, ATA_CMD_READ, reinterpret_cast<uint64_t> (this), location+offs);

#if 0
    uint64_t end = timer.getTickCount();
    NOTICE("Ended read request at " << Dec << end << " [" << (end - now) << " seconds]");
#endif

    /// \todo Add speculative loading here.

    return m_Cache.lookup(location+offs) - offs;
}

void AtaDisk::write(uint64_t location)
{
    if (location % 512)
        FATAL("AtaDisk: write request not on a sector boundary!");

    // Grab our parent.
    AtaController *pParent = static_cast<AtaController*> (m_pParent);

    // Look through the align points.
    uint64_t alignPoint = 0;
    for (size_t i = 0; i < m_nAlignPoints; i++)
        if (m_AlignPoints[i] < location && m_AlignPoints[i] > alignPoint)
            alignPoint = m_AlignPoints[i];

    // Calculate the offset to get location on a page boundary.
    ssize_t offs =  -((location - alignPoint) % 4096);

    // Find the cache page.
    uintptr_t buffer;
    if ( !(buffer=m_Cache.lookup(location+offs)) )
        return;

    pParent->addRequest(1, ATA_CMD_WRITE, reinterpret_cast<uint64_t> (this), location+offs);
}

void AtaDisk::align(uint64_t location)
{
    assert (m_nAlignPoints < 8);
    m_AlignPoints[m_nAlignPoints++] = location;
}

void AtaDisk::flush(uint64_t location)
{
    if(location & 0xFFF)
        location &= ~0xFFF;

    uintptr_t buff = m_Cache.lookup(location);
    if(!buff)
        return;

    doWrite(location);
}

uint64_t AtaDisk::doRead(uint64_t location)
{
    // Handle the case where a read took place while we were waiting in the
    // RequestQueue - don't double up the cache.
    size_t nBytes = 65536;
    uint64_t oldLocation = location;
    location &= ~(nBytes - 1);
    uintptr_t buffer = m_Cache.lookup(location);
    if(buffer)
    {
        WARNING("AtaDisk::doRead(" << oldLocation << ") - buffer was already in cache");
        return 0;
    }
    buffer = m_Cache.insert(location, nBytes);
    if(!buffer)
    {
        FATAL("AtaDisk::doRead - no buffer");
    }

    // Grab our parent.
    AtaController *pParent = static_cast<AtaController*> (m_pParent);

    // Grab our parent's IoPorts for command and control accesses.
    IoBase *commandRegs = m_CommandRegs;
#ifndef PPC_COMMON
    IoBase *controlRegs = m_ControlRegs;
#endif

    // Get the buffer in pointer form.
    uint16_t *pTarget = reinterpret_cast<uint16_t*> (buffer);

    // How many sectors do we need to read?
    uint32_t nSectors = nBytes / 512;

    // Wait for BSY and DRQ to be zero before selecting the device
    AtaStatus status;
    ataWait(commandRegs);

    // Select the device to transmit to
    uint8_t devSelect;
    if (m_SupportsLBA48)
        devSelect = (m_IsMaster) ? 0xE0 : 0xF0;
    else
        devSelect = (m_IsMaster) ? 0xA0 : 0xB0;
    commandRegs->write8(devSelect, 6);

    // Wait for it to be selected
    ataWait(commandRegs);

    while (nSectors > 0)
    {
        // Wait for status to be ready - spin until READY bit is set.
        while (!(commandRegs->read8(7) & 0x40))
            ;

        // Send out sector count.
        uint8_t nSectorsToRead = (nSectors>255) ? 255 : nSectors;
        nSectors -= nSectorsToRead;

        bool bDmaSetup = false;
        if(m_bDma)
        {
            bDmaSetup = m_BusMaster->add(buffer, nSectorsToRead * 512);
        }

        /// \todo CHS
        if (m_SupportsLBA48)
            setupLBA48(location, nSectorsToRead);
        else
        {
            if (location >= 0x2000000000ULL)
            {
                WARNING("Ata: Sector > 128GB requested but LBA48 addressing not supported!");
            }
            setupLBA28(location, nSectorsToRead);
        }

        // Enable disk interrupts
#ifndef PPC_COMMON
        controlRegs->write8(0x08, 6);
#endif

        // Make sure the IrqReceived mutex is locked.
        m_IrqReceived.tryAcquire();

        /// \bug Hello! I am a race condition! You find me in poorly written code, like the two lines below. Enjoy!

        // Enable IRQs.
        uintptr_t intNumber = pParent->getInterruptNumber();
        if(intNumber != 0xFF)
            Machine::instance().getIrqManager()->enable(intNumber, true);

        if(m_bDma && bDmaSetup)
        {
            if (!m_SupportsLBA48)
            {
                // Send command "read DMA"
                commandRegs->write8(0xC8, 7);
            }
            else
            {
                // Send command "read DMA EXT"
                commandRegs->write8(0x25, 7);
            }

            // Start the DMA command
            bDmaSetup = m_BusMaster->begin(false);
        }
        else
        {
            if (m_SupportsLBA48)
            {
                // Send command "read sectors EXT"
                commandRegs->write8(0x24, 7);
            }
            else
            {
                // Send command "read sectors with retry"
                commandRegs->write8(0x20, 7);
            }
        }

        // Acquire the 'outstanding IRQ' mutex, or use other means if no IRQ.
        while(true)
        {
            if(intNumber != 0xFF)
            {
                m_IrqReceived.acquire(1, 10);
            }
            else
            {
                // No IRQ line.
                if(m_bDma && bDmaSetup)
                {
                    while(!(m_BusMaster->hasCompleted() || m_BusMaster->hasInterrupt()))
                    {
                        Processor::haltUntilInterrupt();
                    }
                }
                else
                {
                    /// \todo Write non-DMA case (if needed?)
                }
            }

            if(Processor::information().getCurrentThread()->wasInterrupted())
            {
                // Interrupted! Fail! Assume nothing read so far.
                WARNING("ATA: Timed out while waiting for IRQ");
                return 0;
            }
            else if(m_bDma && bDmaSetup)
            {
                // Ensure we are not busy before continuing handling.
                status = ataWait(commandRegs);
                if(status.reg.err)
                {
                    /// \todo What's the best way to handle this?
                    m_BusMaster->commandComplete();
                    WARNING("ATA: read failed during DMA data transfer");
                    return 0;
                }

                if(m_BusMaster->hasInterrupt() || m_BusMaster->hasCompleted())
                {
                    // commandComplete effectively resets the device state, so we need
                    // to get the error register first.
                    bool bError = m_BusMaster->hasError();
                    m_BusMaster->commandComplete();
                    if(bError)
                        return 0;
                    else
                        break;
                }
            }
            else
                break;
        }

        if(!m_bDma && !bDmaSetup)
        {
            for (int i = 0; i < nSectorsToRead; i++)
            {
                // Wait until !BUSY
                status = ataWait(commandRegs);
                if(status.reg.err)
                {
                    // Ka-boom! Something went wrong :(
                    /// \todo What's the best way to handle this?
                    WARNING("ATA: read failed during data transfer");
                    return 0;
                }

                // Read the sector.
                for (int j = 0; j < 256; j++)
                    *pTarget++ = commandRegs->read16(0);
            }
        }
    }
    return 0;
}

uint64_t AtaDisk::doWrite(uint64_t location)
{
    if (location % 512)
        panic("AtaDisk: write request not on a sector boundary!");

    // Safety check
#ifdef CRIPPLE_HDD
    return 0;
#endif

    uintptr_t buffer = m_Cache.lookup(location);

    uintptr_t nBytes = 4096;

    /// \todo DMA?
    // Grab our parent.
    AtaController *pParent = static_cast<AtaController*> (m_pParent);

    // Grab our parent's IoPorts for command and control accesses.
    IoBase *commandRegs = m_CommandRegs;
#ifndef PPC_COMMON
    IoBase *controlRegs = m_ControlRegs;
#endif

    // How many sectors do we need to read?
    uint32_t nSectors = nBytes / 512;
    if (nBytes%512) nSectors++;

    // Wait for BSY and DRQ to be zero before selecting the device
    AtaStatus status;
    ataWait(commandRegs);

    // Select the device to transmit to
    uint8_t devSelect;
    if (m_SupportsLBA48)
        devSelect = (m_IsMaster) ? 0xE0 : 0xF0;
    else
        devSelect = (m_IsMaster) ? 0xA0 : 0xB0;
    commandRegs->write8(devSelect, 6);

    // Wait for it to be selected
    ataWait(commandRegs);

    uint16_t *tmp = reinterpret_cast<uint16_t*>(buffer);

    while (nSectors > 0)
    {
        // Wait for status to be ready - spin until READY bit is set.
        while (!(commandRegs->read8(7) & 0x40))
            ;

        // Send out sector count.
        uint8_t nSectorsToWrite = (nSectors>255) ? 255 : nSectors;
        nSectors -= nSectorsToWrite;

        /// \todo CHS
        if (m_SupportsLBA48)
            setupLBA48(location, nSectorsToWrite);
        else
        {
            if (location >= 0x2000000000ULL)
            {
                WARNING("Ata: Sector > 128GB requested but LBA48 addressing not supported!");
            }
            setupLBA28(location, nSectorsToWrite);
        }

        // Enable disk interrupts
#ifndef PPC_COMMON
        controlRegs->write8(0x08, 6);
#endif
        // Make sure the IrqReceived mutex is locked.
        m_IrqReceived.tryAcquire();

        /// \bug Hello! I am a race condition! You find me in poorly written code, like the two lines below. Enjoy!

        // Enable IRQs.
        Machine::instance().getIrqManager()->enable(getParent()->getInterruptNumber(), true);

        bool bDmaSetup = false;
        if(m_bDma)
        {
            if (!m_SupportsLBA48)
            {
                // Send command "write DMA"
                commandRegs->write8(0xCA, 7);
            }
            else
            {
                // Send command "read write EXT"
                commandRegs->write8(0x35, 7);
            }

            // Start the DMA command
            bDmaSetup = m_BusMaster->add(buffer, nSectorsToWrite * 512);
            bDmaSetup = m_BusMaster->begin(true);
        }
        else
        {
            if (m_SupportsLBA48)
            {
                // Send command "write sectors EXT"
                commandRegs->write8(0x34, 7);
            }
            else
            {
                // Send command "write sectors with retry"
                commandRegs->write8(0x30, 7);
            }
        }

        // Acquire the 'outstanding IRQ' mutex.
        while(true)
        {
            m_IrqReceived.acquire(1, 10);
            if(Processor::information().getCurrentThread()->wasInterrupted())
            {
                // Interrupted! Fail! Assume nothing read so far.
                WARNING("ATA: Timed out while waiting for IRQ");
                return 0;
            }
            else if(m_bDma && bDmaSetup)
            {
                if(m_BusMaster->hasInterrupt())
                {
                    // commandComplete effectively resets the device state, so we need
                    // to get the error register first.
                    bool bError = m_BusMaster->hasError();
                    m_BusMaster->commandComplete();
                    if(bError)
                        return 0;
                    else
                        break;
                }
            }
            else
                break;
        }

        if(!m_bDma && !bDmaSetup)
        {
            for (int i = 0; i < nSectorsToWrite; i++)
            {
                // Wait until !BUSY
                status = ataWait(commandRegs);
                if(status.reg.err)
                {
                    // Ka-boom! Something went wrong :(
                    /// \todo What's the best way to handle this?
                    WARNING("ATA: read failed during data transfer");
                    return 0;
                }

                // Grab the current sector
                uint8_t *currSector = new uint8_t[512];

                // We got the mutex, so an IRQ must have arrived.
                for (int j = 0; j < 256; j++)
                    commandRegs->write16(*tmp++, 0);

                // Delete used memory
                delete [] currSector;
            }
        }
        else
        {
            // Check for an error during the DMA command
            status = ataWait(commandRegs);
            if(status.reg.err)
            {
                /// \todo What's the best way to handle this?
                WARNING("ATA: read failed during DMA data transfer");
                return 0;
            }
        }
    }

    return nBytes;
}

void AtaDisk::irqReceived()
{
    m_IrqReceived.release();
}

void AtaDisk::setupLBA28(uint64_t n, uint32_t nSectors)
{
    // Grab our parent.
    AtaController *pParent = static_cast<AtaController*> (m_pParent);

    // Grab our parent's IoPorts for command and control accesses.
    IoBase *commandRegs = m_CommandRegs;
    // Unused variable.
    //IoBase *controlRegs = m_ControlRegs;

    commandRegs->write8(static_cast<uint8_t>(nSectors&0xFF), 2);

    // Get the sector number of the address.
    n /= 512;

    uint8_t sector = static_cast<uint8_t> (n&0xFF);
    uint8_t cLow = static_cast<uint8_t> ((n>>8) & 0xFF);
    uint8_t cHigh = static_cast<uint8_t> ((n>>16) & 0xFF);
    uint8_t head = static_cast<uint8_t> ((n>>24) & 0x0F);
    if (m_IsMaster) head |= 0xE0;
    else head |= 0xF0;

    commandRegs->write8(head, 6);
    commandRegs->write8(sector, 3);
    commandRegs->write8(cLow, 4);
    commandRegs->write8(cHigh, 5);
}

void AtaDisk::setupLBA48(uint64_t n, uint32_t nSectors)
{
    // Grab our parent.
    AtaController *pParent = static_cast<AtaController*> (m_pParent);

    // Grab our parent's IoPorts for command and control accesses.
    IoBase *commandRegs = m_CommandRegs;
    // Unused variable.
    //IoBase *controlRegs = m_ControlRegs;

    // Get the sector number of the address.
    n /= 512;

    uint8_t lba1 = static_cast<uint8_t> (n&0xFF);
    uint8_t lba2 = static_cast<uint8_t> ((n>>8) & 0xFF);
    uint8_t lba3 = static_cast<uint8_t> ((n>>16) & 0xFF);
    uint8_t lba4 = static_cast<uint8_t> ((n>>24) & 0xFF);
    uint8_t lba5 = static_cast<uint8_t> ((n>>32) & 0xFF);
    uint8_t lba6 = static_cast<uint8_t> ((n>>40) & 0xFF);

    commandRegs->write8((nSectors&0xFFFF)>>8, 2);
    commandRegs->write8(lba4, 3);
    commandRegs->write8(lba5, 4);
    commandRegs->write8(lba6, 5);
    commandRegs->write8((nSectors&0xFF), 2);
    commandRegs->write8(lba1, 3);
    commandRegs->write8(lba2, 4);
    commandRegs->write8(lba3, 5);
}

bool AtaDisk::setFeatures(uint8_t command, uint8_t countreg, uint8_t lowreg, uint8_t midreg, uint8_t hireg)
{
    // Grab our parent's IoPorts for command and control accesses.
    IoBase *commandRegs = m_CommandRegs;

    uint8_t devSelect = (m_IsMaster) ? 0xA0 : 0xB0;
    commandRegs->write8(devSelect, 6);

    commandRegs->write8(command, 1);
    commandRegs->write8(countreg, 2);
    commandRegs->write8(lowreg, 3);
    commandRegs->write8(midreg, 4);
    commandRegs->write8(hireg, 5);
    commandRegs->write8(0xEF, 7);
}

