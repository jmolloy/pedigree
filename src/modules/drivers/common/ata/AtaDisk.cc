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

// Note the IrqReceived mutex is deliberately started in the locked state.
AtaDisk::AtaDisk(AtaController *pDev, bool isMaster, IoBase *commandRegs, IoBase *controlRegs, IoBase *busMaster) :
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
    //IoBase *controlRegs = m_ControlRegs;

    // Drive spin-up
    commandRegs->write8(0x00, 6);

    //
    // Start IDENTIFY command.
    //

    // Wait for BSY and DRQ to be zero before selecting the device
    uint8_t status = commandRegs->read8(7);
    while (((status & 0x80) != 0) && ((status & 0x8) == 0))
    status = commandRegs->read8(7);

    // Select the device to transmit to
    uint8_t devSelect = (m_IsMaster) ? 0xA0 : 0xB0;
    commandRegs->write8(devSelect, 6);

    // Wait for it to be selected
    status = commandRegs->read8(7);
    while (((status & 0x80) != 0) && ((status & 0x8) == 0))
    status = commandRegs->read8(7);

    // Disable IRQs, for the moment.
//   controlRegs->write8(0x01, 6);

    // Send IDENTIFY.
    status = commandRegs->read8(7);
    commandRegs->write8(0xEC, 7);

    // Read status register.
    status = commandRegs->read8(7);

    if (status == 0)
        // Device does not exist.
        return false;

    // Poll until BSY is clear and either ERR or DRQ are set
    while ( ((status&0x80) != 0) && ((status&0x9) == 0) )
        status = commandRegs->read8(7);

    // If ERR was set we had an err0r.
    if (status & 0x1)
    {
        WARNING("ATA drive errored on IDENTIFY!");
        return false;
    }

    // Read the data.
    for (int i = 0; i < 256; i++)
    {
        m_pIdent[i] = commandRegs->read16(0);
    }

    status = commandRegs->read8(7);

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

    uint16_t word83 = LITTLE_TO_HOST16(m_pIdent[83]);
    if (word83 & (1<<10))
    {
        m_SupportsLBA48 = true;
    }
  
    // Any form of DMA support?
#if 0
    if(m_pIdent[49] & (1 << 8))
    {
      // Check that we have a bus master device to use
      if(m_BusMaster)
      {
          // We have a definite ATAPI device, set up the PRD and all that now
          if(!PhysicalMemoryManager::instance().allocateRegion(m_PrdTableMemRegion,
                                                               1,
                                                               PhysicalMemoryManager::continuous,
                                                               VirtualAddressSpace::Write,
                                                               -1))
          {
              ERROR("ATA: Couldn't allocate PRD table. Continuing without DMA support.");
              m_bDma = false;
          }
          else
          {
              /// \todo Store the type of DMA supported by this device
              m_PrdTable = reinterpret_cast<PhysicalRegionDescriptor *>(m_PrdTableMemRegion.virtualAddress());
              m_PrdTablePhys = m_PrdTableMemRegion.physicalAddress();
              NOTICE("ATA: PRD table at v=" << reinterpret_cast<uintptr_t>(m_PrdTableMemRegion.virtualAddress()) << ", p=" << m_PrdTablePhys << ".");
          }
      }
      else
      {
          WARNING("ATA: Device supports DMA, but the controller does not.");
          m_bDma = false;
      }
    }
    else
    {
        NOTICE("ATA: Device does not support DMA");
        m_bDma = false;
    }
#else
    m_bDma = false;
#endif

    NOTICE("Detected ATA device '" << m_pName << "', '" << m_pSerialNumber << "', '" << m_pFirmwareRevision << "'");
    return true;
}

uintptr_t AtaDisk::read(uint64_t location)
{
    if (location % 512)
        FATAL("AtaDisk: write request not on a sector boundary!");

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

    pParent->addRequest(0, ATA_CMD_READ, reinterpret_cast<uint64_t> (this), location+offs);

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

uint64_t AtaDisk::doRead(uint64_t location)
{
    uint64_t nBytes = 4096;
    uintptr_t buffer = m_Cache.insert(location);

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
    uint8_t status = commandRegs->read8(7);
    while (((status & 0x80) != 0) && ((status & 0x8) == 0))
    status = commandRegs->read8(7);

    // Select the device to transmit to
    uint8_t devSelect;
    if (m_SupportsLBA48)
        devSelect = (m_IsMaster) ? 0x40 : 0x50;
    else
        devSelect = (m_IsMaster) ? 0xA0 : 0xB0;
    commandRegs->write8(devSelect, 6);

    // Wait for it to be selected
    status = commandRegs->read8(7);
    while (((status & 0x80) != 0) && ((status & 0x8) == 0))
        status = commandRegs->read8(7);

    while (nSectors > 0)
    {
        // Wait for status to be ready - spin until READY bit is set.
        while (!(commandRegs->read8(7) & 0x40))
            ;

        // Send out sector count.
        uint8_t nSectorsToRead = (nSectors>255) ? 255 : nSectors;
        nSectors -= nSectorsToRead;

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

        // If DMA is enabled, set that up now
        /// \todo Definitely need a BusMaster class to keep this stuff out of
        ///       the individual drivers.
        bool bDmaSetup = false;
        if(m_bDma)
        {
#if 0
            // Grab the physical address of the buffer we've been given
            VirtualAddressSpace &va = Processor::information().getVirtualAddressSpace();
            if(va.isMapped(reinterpret_cast<void*>(buffer)))
            {
                // Grab the PRD table lock
                m_PrdTableLock.acquire();

                // Add it to the PRD table - ensuring that we stick to the 64K limit per PRD
                size_t nRemainingBytes = nBytes;
                size_t currOffset = 0;
                int i = 0;
                while(nRemainingBytes)
                {
                    physical_uintptr_t phys = 0;
                    size_t flags = 0;
                    va.getMapping(reinterpret_cast<void*>(buffer + currOffset), phys, flags);
                    if(i == 0)
                        phys += (buffer & 0xFFF);

                    m_PrdTable[i].physAddr = phys;

                    size_t transferSize = nRemainingBytes;
                    if(transferSize > 4096)
                        transferSize = 4096 - ((i == 0) ? (buffer & 0xFFF) : 0);
#if 0
                    NOTICE("m_PrdTable[" << Dec << i << Hex << "].physAddr = " << phys << ";");
                    NOTICE("m_PrdTable[" << Dec << i << Hex << "].byteCount = " << transferSize << ";");
#endif
                    m_PrdTable[i].byteCount = transferSize;

                    currOffset += transferSize;
                    nRemainingBytes -= transferSize;

                    if(!nRemainingBytes)
                        m_PrdTable[i].rsvdEot = 0x8000;
                    else
                        m_PrdTable[i].rsvdEot = 0;
                    i++;
                }

#if 0
                NOTICE("Set up " << i << " PRDs, " << Dec << currOffset << Hex << " bytes");
#endif

                // Wait for no other DMA command to be running
                uint8_t statusReg = m_BusMaster->read8(BusMasterIde::Status);
                while(statusReg & 0x01)
                  statusReg = m_BusMaster->read8(BusMasterIde::Status);
                statusReg = 0x60;
                m_BusMaster->write8(statusReg, BusMasterIde::Status);

                // Save our PRD physical address
                m_BusMaster->write32(m_PrdTablePhys, BusMasterIde::PrdTableAddr);

                bDmaSetup = true;

                m_PrdTableLock.release();
            }
            else
            {
                ERROR("ATA: buffer was not mapped!");
                return 0;
            }
#endif
        }

        // Make sure the IrqReceived mutex is locked.
        m_IrqReceived.tryAcquire();

        /// \bug Hello! I am a race condition! You find me in poorly written code, like the two lines below. Enjoy!

        // Enable IRQs.
        Machine::instance().getIrqManager()->enable(getParent()->getInterruptNumber(), true);

#if 0
        if(m_bDma && bDmaSetup)
        {
            if (m_SupportsLBA48)
            {
                // Send command "read DMA"
                commandRegs->write8(0xC8, 7);
            }
            else
            {
                // Send command "read DMA EXT"
                commandRegs->write8(0x25, 7);
            }

            // Start the DMA command, if needed
            uint8_t cmdReg = 0;
            cmdReg = m_BusMaster->read8(BusMasterIde::Command);
            if(cmdReg & 0x1)
                ERROR("ATA: Start/Stop Bus Master was already set!");
            else
                cmdReg = (cmdReg & 0xF6) | 0x1;
            m_BusMaster->write8(cmdReg, BusMasterIde::Command);
        }
        else
        {
#endif
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
#if 0
        }
#endif

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
#if 0
            else if(m_bDma && bDmaSetup)
            {
                // Check first that an IRQ has fired for us. If not, keep waiting.
                uint8_t statusReg = m_BusMaster->read8(BusMasterIde::Status);
                uint8_t clearReg = 0;
                if(statusReg & 0x4)
                {
                    bool bError = false;
                    clearReg |= 0x4;

                    // Yes, the device fired this interrupt. Error?
                    if(statusReg & 0x2)
                    {
                        WARNING("ATA: DMA transfer failed");
                        clearReg |= 0x2;
                        bError = true;
                    }

                    uint8_t cmdReg = m_BusMaster->read8(BusMasterIde::Command);
                    m_BusMaster->write8((cmdReg & 0xFE), BusMasterIde::Command);

                    m_BusMaster->write8(clearReg, BusMasterIde::Status);

                    if(bError)
                        return 0;
                    else
                        break;
                }
            }
            else
#endif
                break;
        }

        if(!m_bDma && !bDmaSetup)
        {
            for (int i = 0; i < nSectorsToRead; i++)
            {
                // Wait until !BUSY
                while (commandRegs->read8(7) & 0x80)
                    ;

                // Mark the start of the sector.
                uint8_t *pSector = reinterpret_cast<uint8_t*> (pTarget);

                // We got the mutex, so an IRQ must have arrived.
                for (int j = 0; j < 256; j++)
                    *pTarget++ = commandRegs->read16(0);
            }
        }
        else
        {
            /// \todo Check status register for command success
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
    uint8_t status = commandRegs->read8(7);
    while (((status & 0x80) != 0) && ((status & 0x8) == 0))
    status = commandRegs->read8(7);

    // Select the device to transmit to
    uint8_t devSelect;
    if (m_SupportsLBA48)
        devSelect = (m_IsMaster) ? 0x40 : 0x50;
    else
        devSelect = (m_IsMaster) ? 0xA0 : 0xB0;
    commandRegs->write8(devSelect, 6);

    // Wait for it to be selected
    status = commandRegs->read8(7);
    while (((status & 0x80) != 0) && ((status & 0x8) == 0))
    status = commandRegs->read8(7);

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

        // Acquire the 'outstanding IRQ' mutex.
        m_IrqReceived.acquire();

        for (int i = 0; i < nSectorsToWrite; i++)
        {
            // Wait until !BUSY
            while (commandRegs->read8(7) & 0x80)
                ;

            // Grab the current sector
            uint8_t *currSector = new uint8_t[512];

            // We got the mutex, so an IRQ must have arrived.
            for (int j = 0; j < 256; j++)
                commandRegs->write16(*tmp++, 0);

            // Delete used memory
            delete [] currSector;
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

/** \note I'm pretty sure this is correct. Time will tell I guess! */
bool AtaDisk::sendPacket(size_t nBytes, uintptr_t packet)
{
    // Grab our parent.
    AtaController *pParent = static_cast<AtaController*> (m_pParent);

    // Grab our parent's IoPorts for command and control accesses.
    IoBase *commandRegs = m_CommandRegs;

    // PACKET command
    commandRegs->write8(0, 1); // no overlap, no DMA
    commandRegs->write8(0, 2); // tag = 0
    commandRegs->write8(0, 3); // n/a for PACKET command
    commandRegs->write8((nBytes & 0xFF), 4); // byte count limit
    commandRegs->write8(((nBytes >> 8) & 0xFF), 5);
    commandRegs->write8((m_IsMaster)?0xA0:0xB0, 6);
    commandRegs->write8(0xA0, 7); // the command itself

    // Wait for the busy bit to be cleared before continuing
    uint8_t status = commandRegs->read8(7);
    while ( ((status&0x80) != 0) && ((status&0x9) == 0) )
        status = commandRegs->read8(7);

    // Error?
    if (status & 0x01)
    {
        ERROR("ATAPI Packet command error [status=" << status << "]!");
        return false;
    }

    // Transmit the command
    uint16_t *commandPacket = reinterpret_cast<uint16_t*>(packet);
    for (size_t i = 0; i < 6; i++)
        commandRegs->write16(commandPacket[i], 0);

    // Wait for the busy bit to be cleared once again
    while ( ((status&0x80) != 0) && ((status&0x9) == 0) )
        status = commandRegs->read8(7);

    // Complete
    return (!(status & 0x01));
}
