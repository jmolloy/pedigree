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
#include "AtaController.h"

// Note the IrqReceived mutex is deliberately started in the locked state.
AtaDisk::AtaDisk(AtaController *pDev, bool isMaster) :
  Disk(), m_IsMaster(isMaster), m_SupportsLBA28(true), m_SupportsLBA48(false), m_IrqReceived(true)
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
  IoBase *commandRegs = pParent->m_pCommandRegs;
  // Commented out - unused variable.
  //IoBase *controlRegs = pParent->m_pControlRegs;

  // Drive spin-up
  commandRegs->write8(0x00, 6);

  //
  // Start IDENTIFY command.
  //

  // Send drive select.
  commandRegs->write8( (m_IsMaster)?0xA0:0xB0, 6 );
  // Read the status 5 times as a delay for the drive to go about its business.
  for (int i = 0; i < 5; i++)
    commandRegs->read8(7);

  // Disable IRQs, for the moment.
//   controlRegs->write8(0x01, 6);

  // Send IDENTIFY.
  uint8_t status = commandRegs->read8(7);
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
    WARNING("ATA drive errored on IDENTIFY! Possible ATAPI device?");
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
    m_SupportsLBA48 = /*true*/ false;
  }

  NOTICE("Detected ATA device '" << m_pName << "', '" << m_pSerialNumber << "', '" << m_pFirmwareRevision << "'");
  return true;
}

uint64_t AtaDisk::read(uint64_t location, uint64_t nBytes, uintptr_t buffer)
{
  // Grab our parent.
  AtaController *pParent = static_cast<AtaController*> (m_pParent);
  return pParent->addRequest(ATA_CMD_READ, reinterpret_cast<uint64_t> (this), location,
                             nBytes, static_cast<uint64_t> (buffer));
}

uint64_t AtaDisk::write(uint64_t location, uint64_t nBytes, uintptr_t buffer)
{
  // Grab our parent.
  AtaController *pParent = static_cast<AtaController*> (m_pParent);
  return pParent->addRequest(ATA_CMD_WRITE, reinterpret_cast<uint64_t> (this), location,
                             nBytes, static_cast<uint64_t> (buffer));
}

uint64_t AtaDisk::doRead(uint64_t location, uint64_t nBytes, uintptr_t buffer)
{
  if (location % 512)
    panic("AtaDisk: read request not on a sector boundary!");
  if (nBytes % 512)
    panic("AtaDisk: read request length not a multiple of 512!");

  /// \todo DMA?
  // Grab our parent.
  AtaController *pParent = static_cast<AtaController*> (m_pParent);

  // Grab our parent's IoPorts for command and control accesses.
  IoBase *commandRegs = pParent->m_pCommandRegs;
  IoBase *controlRegs = pParent->m_pControlRegs;

  // Get the buffer in pointer form.
  uint16_t *pTarget = reinterpret_cast<uint16_t*> (buffer);

  // How many sectors do we need to read?
  uint32_t nSectors = nBytes / 512;
  if (nBytes%512) nSectors++;

  // Send drive select.
  if (m_SupportsLBA48)
    commandRegs->write8( (m_IsMaster)?0x40:0x50, 6 );
  else
    commandRegs->write8( (m_IsMaster)?0xA0:0xB0, 6 );

  // Read the status 5 times as a delay for the drive to go about its business.
  for (int i = 0; i < 5; i++)
    commandRegs->read8(7);

  while (nSectors > 0)
  {
    // Wait for status to be ready - spin until READY bit is set.
    while (!(commandRegs->read8(7) & 0x40))
      ;

    // Send out sector count.
    uint8_t nSectorsToRead = (nSectors>255) ? 255 : nSectors;
    nSectors -= nSectorsToRead;

    // TODO: LBA28, LBA48 or CHS here.
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
    controlRegs->write8(0x08, 6);

    // Make sure the IrqReceived mutex is locked.
    m_IrqReceived.tryAcquire();

    /// \bug Hello! I am a race condition! You find me in poorly written code, like the two lines below. Enjoy!

    // Enable IRQs.
    Machine::instance().getIrqManager()->enable(getParent()->getInterruptNumber(), true);

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

    // Acquire the 'outstanding IRQ' mutex.
    m_IrqReceived.acquire();

    for (int i = 0; i < nSectorsToRead; i++)
    {
      // Wait until !BUSY
      while (commandRegs->read8(7) & 0x80)
        ;

      // We got the mutex, so an IRQ must have arrived.
      for (int j = 0; j < 256; j++)
        *pTarget++ = commandRegs->read16(0);
    }
  }
  return nBytes;
}

uint64_t AtaDisk::doWrite(uint64_t location, uint64_t nBytes, uintptr_t buffer)
{
  NOTICE("Write: " << Hex << location << ", " << nBytes << ", " << buffer);
  return 0;
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
  IoBase *commandRegs = pParent->m_pCommandRegs;
  // Unused variable.
  //IoBase *controlRegs = pParent->m_pControlRegs;

  commandRegs->write8(static_cast<uint8_t>(nSectors&0xFF), 2);

  // Get the sector number of the address.
  n /= 512;

  uint8_t sector = static_cast<uint8_t> (n&0xFF);
  uint8_t cLow = static_cast<uint8_t> ((n>>8) & 0xFF);
  uint8_t cHigh = static_cast<uint8_t> ((n>>16) & 0xFF);
  uint8_t head = static_cast<uint8_t> ((n>>24) & 0x0F);
  if (m_IsMaster) head |= 0xE0; else head |= 0xF0;

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
  IoBase *commandRegs = pParent->m_pCommandRegs;
  // Unused variable.
  //IoBase *controlRegs = pParent->m_pControlRegs;

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
