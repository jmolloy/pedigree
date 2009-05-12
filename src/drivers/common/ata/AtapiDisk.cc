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
 * WHATSOEVER RESULTING FROM LOSS OF USE, DAtapi OR PROFITS, WHETHER IN AN
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
#include <utilities/PointerGuard.h>
#include "AtaController.h"

// Note the IrqReceived mutex is deliberately started in the locked state.
AtapiDisk::AtapiDisk(AtaController *pDev, bool isMaster) :
  AtaDisk(pDev, isMaster), m_IsMaster(isMaster), m_SupportsLBA28(true), m_SupportsLBA48(false),
  m_Type(None), m_NumBlocks(0), m_BlockSize(0), m_PacketSize(0), m_Removable(true), m_IrqReceived(true),
  m_SectorCache()
{
  m_pParent = pDev;
}

AtapiDisk::~AtapiDisk()
{
}

bool AtapiDisk::initialise()
{
  NOTICE("ATAPI init");

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

  // Check for an AtapiPI device
  if(commandRegs->read8(2) == 0x01 &&
      commandRegs->read8(3) == 0x01 &&
      commandRegs->read8(4) == 0x14 &&
      commandRegs->read8(5) == 0xeb)
  {
    // Run IDENTIFY PACKET DEVICE instead
    commandRegs->write8(0xA1, 7);
    status = commandRegs->read8(7);

    // Poll until BSY is clear and either ERR or DRQ are set
    while ( ((status&0x80) != 0) && ((status&0x9) == 0) )
      status = commandRegs->read8(7);
  }
  else
  {
    WARNING("NON-ATAPI device, skipping");
    return false;
  }

  // If ERR was set we had an err0r.
  if (status & 0x1)
  {
    WARNING("ATAPI drive errored on IDENTIFY!");
    return false;
  }

  // Read the dAtapi.
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

  // Packet size?
  m_PacketSize = ((m_pIdent[0] & 0x0003) == 0) ? 12 : 16;

  // Grab the capacity of the disk for future reference
  getCapacityInternal(&m_NumBlocks, &m_BlockSize);

  NOTICE("This disk has " << Dec << m_NumBlocks << " blocks, each of which is " << m_BlockSize << Hex << " bytes.");

  // Send an INQUIRY command to find more information about the disk
  Inquiry inquiry;
  struct InquiryCommand
  {
    uint8_t Opcode;
    uint8_t LunEvpd;
    uint8_t PageCode;
    uint16_t AllocLen;
    uint8_t Control;
  } __attribute__((packed)) inqCommand;
  memset(&inqCommand, 0, sizeof(InquiryCommand));
  inqCommand.Opcode = 0x12; // INQUIRY command
  inqCommand.AllocLen = HOST_TO_BIG16(sizeof(Inquiry));
  bool success = sendCommand(sizeof(Inquiry), sizeof(InquiryCommand), reinterpret_cast<uintptr_t>(&inqCommand));
  if(!success)
  {
    // Uh oh!
    WARNING("Couldn't run INQUIRY command on ATAPI device!");
    return false;
  }

  success = readPacket(sizeof(Inquiry), reinterpret_cast<uintptr_t>(&inquiry));
  if(!success)
  {
    WARNING("Couldn't read response to INQUIRY on ATAPI device!");
    return false;
  }

  // Removable - and get the type too
  m_Removable = ((inquiry.Removable & (1 << 7)) != 0);
  m_Type = static_cast<AtapiType>(inquiry.Peripheral);

  NOTICE("peripheral = " << inquiry.Peripheral << " and type = " << static_cast<int>(m_Type) << "!");

  // Supported device?
  if(m_Type != CdDvd && m_Type != Block)
  {
    /// \todo Support more than just CDs and DVDs
    WARNING("Pedigree currently only supports CD/DVD and block ATAPI devices.");
    return false;
  }

  // Have some fun <3
  uint8_t *whee = new uint8_t[m_BlockSize];
  uint64_t ret = doRead(0x8000, m_BlockSize, reinterpret_cast<uintptr_t>(whee));
  NOTICE("Read " << ret << " bytes:");
  for(size_t i = 0; i < 32; i++)
    NOTICE("At " << Dec << i << Hex << ": " << whee[i]);

  FATAL("ATAPI device init done!");

  NOTICE("Detected ATAPI device '" << m_pName << "', '" << m_pSerialNumber << "', '" << m_pFirmwareRevision << "'");
  return true;
}

uint64_t AtapiDisk::read(uint64_t location, uint64_t nBytes, uintptr_t buffer)
{
  NOTICE("AtapiDisk::read");

  // Grab our parent.
  AtaController *pParent = static_cast<AtaController*> (m_pParent);
  return pParent->addRequest(ATA_CMD_READ, reinterpret_cast<uint64_t> (this), location,
                             nBytes, static_cast<uint64_t> (buffer));
}

uint64_t AtapiDisk::write(uint64_t location, uint64_t nBytes, uintptr_t buffer)
{
  // Grab our parent.
  AtaController *pParent = static_cast<AtaController*> (m_pParent);
  return pParent->addRequest(ATA_CMD_WRITE, reinterpret_cast<uint64_t> (this), location,
                             nBytes, static_cast<uint64_t> (buffer));
}

uint64_t AtapiDisk::doRead(uint64_t location, uint64_t nBytes, uintptr_t buffer)
{
  NOTICE("doRead");

  if(!nBytes || !buffer)
  {
    ERROR("Bad arguments to AtapiDisk::doRead");
    return 0;
  }

  if(location > (m_BlockSize * m_NumBlocks))
  {
    WARNING("ATAPI: Attempted read beyond disk size");
    return 0;
  }

  size_t blockNum = location / m_BlockSize;
  size_t numBlocks = nBytes / m_BlockSize;

  if(m_Type == CdDvd)
  {
    // Read the TOC in order to get the data track.
    // This is added to the block number to read the data
    // off the disk properly.
    struct ReadTocCommand
    {
      uint8_t Opcode;
      uint8_t Flags;
      uint8_t Format;
      uint8_t Rsvd[3];
      uint8_t Track;
      uint16_t AllocLen;
      uint8_t Control;
    } __attribute__((packed)) tocCommand;
    memset(&tocCommand, 0, sizeof(ReadTocCommand));
    tocCommand.Opcode = 0x43; // READ TOC command
    tocCommand.AllocLen = HOST_TO_BIG16(m_BlockSize);

    NOTICE("Reading TOC");

    uint8_t *tmpBuff = new uint8_t[m_BlockSize];
    PointerGuard<uint8_t> tmpBuffGuard(tmpBuff);
    bool success = sendCommand(m_BlockSize, sizeof(ReadTocCommand), reinterpret_cast<uintptr_t>(&tocCommand));
    if(!success)
    {
      WARNING("Could not read the TOC!");
      return 0;
    }

    success = readPacket(m_BlockSize, reinterpret_cast<uintptr_t>(tmpBuff));
    if(!success)
    {
      WARNING("Could not read the TOC response!");
      return 0;
    }

    uint16_t i;
    bool bHaveTrack = false;
    uint16_t bufLen = BIG_TO_HOST16(*reinterpret_cast<uint16_t*>(tmpBuff));
    TocEntry *Toc = reinterpret_cast<TocEntry*>(tmpBuff + 4);
    for(i = 0; i < (bufLen / 8); i++)
    {
      if(Toc[i].Flags == 0x14)
      {
        bHaveTrack = true;
        break;
      }
    }

    if(!bHaveTrack)
    {
      WARNING("ATAPI: CD does not have a data track!");
      return 0;
    }

    uint32_t trackStart = BIG_TO_HOST32(Toc[i].TrackStart); // Start of the track, LBA
    if((blockNum + trackStart) < blockNum)
    {
      WARNING("ATAPI TOC overflow");
      return 0;
    }

    NOTICE("Data track starts at " << trackStart << ".");
    blockNum += trackStart;
  }

  // Read the data now
  struct ReadCommand
  {
    uint8_t Opcode;
    uint8_t Flags;
    uint32_t StartLBA;
    uint8_t Rsvd;
    uint16_t BlockCount;
    uint8_t Control;
  } __attribute__((packed)) command;
  memset(&command, 0, sizeof(ReadCommand));
  command.Opcode = 0x25; // READ(10) command
  command.StartLBA = HOST_TO_BIG32(blockNum);
  command.BlockCount = HOST_TO_BIG16(numBlocks);

  bool success = sendCommand(nBytes, sizeof(ReadCommand), reinterpret_cast<uintptr_t>(&command));
  if(!success)
  {
    WARNING("Read command failed on an ATAPI device");
    return 0;
  }

  success = readPacket(nBytes, buffer);
  if(!success)
  {
    WARNING("Data read failed on ATAPI device");
    return 0;
  }

  return nBytes;
}

uint64_t AtapiDisk::doWrite(uint64_t location, uint64_t nBytes, uintptr_t buffer)
{
  return 0;

/*  if (location % 512)
    panic("AtapiDisk: write request not on a sector boundary!");
  if (nBytes % 512)
    panic("AtapiDisk: write request length not a multiple of 512!");

  uint64_t origNBytes = nBytes;

  uint64_t end = location+nBytes;
  for (uint64_t i = location; i < end; i += 512)
  {
    m_SectorCache.insert(i/512, reinterpret_cast<uint8_t*> (buffer));

    buffer += 512;
    location += 512;
    nBytes -= 512;
  }

  return origNBytes;*/
}

void AtapiDisk::irqReceived()
{
  m_IrqReceived.release();
}

bool AtapiDisk::sendCommand(size_t nRespBytes, size_t nPackBytes, uintptr_t packet)
{
  /// \todo Round nPackBytes up to an even number, check that it's valid, etc...

  // Grab our parent.
  AtaController *pParent = static_cast<AtaController*> (m_pParent);

  // Grab our parent's IoPorts for command and control accesses.
  IoBase *commandRegs = pParent->m_pCommandRegs;

  // Round up the packet size to the nearest even number
  if(nPackBytes & 0x1)
    nPackBytes++;

  // PACKET command
  commandRegs->write8(0, 1); // no overlap, no DMA
  commandRegs->write8(0, 2); // tag = 0
  commandRegs->write8(0, 3); // n/a for PACKET command
  commandRegs->write8((nRespBytes == 0) ? 0x2 : (nRespBytes & 0xFF), 4); // byte count limit
  commandRegs->write8(((nRespBytes >> 8) & 0xFF), 5);
  commandRegs->write8((m_IsMaster)?0xA0:0xB0, 6);
  commandRegs->write8(0xA0, 7); // the command itself

  // Wait for the busy bit to be cleared before continuing
  uint8_t status = commandRegs->read8(7);
  while ( ((status&0x80) != 0) && ((status&0x9) == 0) )
    status = commandRegs->read8(7);

  // Error?
  if(status & 0x01)
  {
    ERROR("ATAPI Packet command error [status=" << status << "]!");
    return false;
  }

  // Transmit the command (padded as needed)
  uint16_t *commandPacket = reinterpret_cast<uint16_t*>(packet);
  for(size_t i = 0; i < (nPackBytes / 2); i++)
    commandRegs->write16(commandPacket[i], 0);
  NOTICE("current packet size = " << m_PacketSize << " packet bytes = " << nPackBytes << ".");
  for(size_t i = (nPackBytes / 2); i < (m_PacketSize / 2); i++)
    commandRegs->write16(0, 0);

  // Wait for the busy bit to be cleared once again
  while ( ((status&0x80) != 0) && ((status&0x9) == 0) )
    status = commandRegs->read8(7);

  // Complete
  return (!(status & 0x01));
}

bool AtapiDisk::readPacket(size_t maxBytes, uintptr_t buffer)
{
  // Grab our parent.
  AtaController *pParent = static_cast<AtaController*> (m_pParent);

  // Grab our parent's IoPorts for command and control accesses.
  IoBase *commandRegs = pParent->m_pCommandRegs;

  NOTICE("reading");

  // DRQ set?
  uint8_t status = commandRegs->read8(7);
  if(status & 0x1)
    return false;
  else if(status & 0x8)
  {
    if(maxBytes)
    {
      NOTICE("Reading " << maxBytes << " bytes...");

      /// \todo Handle odd size
      uint16_t *p = reinterpret_cast<uint16_t*>(buffer);
      uint16_t realSz = commandRegs->read8(4) | (commandRegs->read8(5) << 8);
      NOTICE("There are " << realSz << " bytes ready.");

      if(maxBytes > realSz)
        maxBytes = realSz;

      for(size_t i = 0; i < (maxBytes / 2); i++)
        p[i] = commandRegs->read16(0);
      if(realSz > maxBytes)
        for(size_t i = maxBytes; i < realSz; i += 2)
          commandRegs->read16(0);
    }
  }
  else
  {
    NOTICE("not in the right state");
    return false;
  }

  // Apparently we should re-read the status register...
  status = commandRegs->read8(7);
  return (!(status & 0x1));
}

bool AtapiDisk::writePacket(size_t maxBytes, uintptr_t buffer)
{
  // Grab our parent.
  AtaController *pParent = static_cast<AtaController*> (m_pParent);

  // Grab our parent's IoPorts for command and control accesses.
  IoBase *commandRegs = pParent->m_pCommandRegs;

  // DRQ set?
  uint8_t status = commandRegs->read8(7);
  if(status & 0x1)
    return false;
  else if(status & 0x8)
  {
    if(maxBytes)
    {
      /// \todo Handle odd size
      uint16_t *p = reinterpret_cast<uint16_t*>(buffer);
      for(size_t i = 0; i < (maxBytes / 2); i++)
        commandRegs->write16(p[i], 0);
    }
  }

  // Apparently we should re-read the status register...
  status = commandRegs->read8(7);
  return (!(status & 0x1));
}

bool AtapiDisk::readSense(uintptr_t buff)
{
  // Convert the passed buffer
  Sense *sense = reinterpret_cast<Sense*>(buff);
  memset(sense, 0xFF, sizeof(Sense));

  // Command
  struct SenseCommand
  {
    uint8_t Opcode;
    uint8_t Desc;
    uint8_t Rsvd[2];
    uint8_t AllocLength;
    uint8_t Control;
  } __attribute__((packed)) senseComm;
  memset(&senseComm, 0, sizeof(SenseCommand));
  senseComm.Opcode = 0x03; // REQUEST SENSE
  senseComm.AllocLength = sizeof(Sense);

  bool success = sendCommand(sizeof(Sense), sizeof(SenseCommand), reinterpret_cast<uintptr_t>(&senseComm));
  if(!success)
  {
    WARNING("ATAPI: SENSE command failed");
    return false;
  }

  success = readPacket(sizeof(Sense), reinterpret_cast<uintptr_t>(sense));
  if(!success)
  {
    WARNING("ATAPI: Reading SENSE response failed");
    return false;
  }

  return ((sense->ResponseCode & 0x70) == 0x70);
}

bool AtapiDisk::unitReady()
{
  // Command
  Sense sense;
  struct UnitReadyCommand
  {
    uint8_t Opcode;
    uint8_t Rsvd[4];
    uint8_t Control;
  } __attribute__((packed)) command;
  memset(&command, 0, sizeof(UnitReadyCommand));
  command.Opcode = 0x00; // TEST UNIT READY

  bool success = false;
  int retry = 5;
  do
  {
    success = sendCommand(0, sizeof(UnitReadyCommand), reinterpret_cast<uintptr_t>(&command));
  } while((success == false) && readSense(reinterpret_cast<uintptr_t>(&sense)) && --retry);

  if(m_Removable &&
      ((sense.ResponseCode & 0x70) == 0x70) &&
      ((sense.SenseKey == 0x06) || (sense.SenseKey == 0x02)) /* UNIT_ATTN or NOT_READY */
    )
  {
    success = true;
  }

  return success;
}

bool AtapiDisk::getCapacityInternal(size_t *blockNumber, size_t *blockSize)
{
  if(!unitReady())
  {
    *blockNumber = 0;
    *blockSize = defaultBlockSize();
    return false;
  }

  struct Capacity
  {
    uint32_t LBA;
    uint32_t BlockSize;
  } __attribute__((packed)) capacity;

  struct CapacityCommand
  {
    uint8_t Opcode;
    uint8_t LunRelAddr;
    uint32_t LBA;
    uint8_t Rsvd[2];
    uint8_t PMI;
    uint8_t Control;
  } __attribute__((packed)) command;
  memset(&command, 0, sizeof(CapacityCommand));
  command.Opcode = 0x25; // READ CAPACITY

  bool success = sendCommand(sizeof(Capacity), sizeof(CapacityCommand), reinterpret_cast<uintptr_t>(&command));
  if(!success)
  {
    WARNING("ATAPI READ CAPACITY command failed");
    return false;
  }

  success = readPacket(sizeof(Capacity), reinterpret_cast<uintptr_t>(&capacity));
  if(!success)
  {
    WARNING("ATAPI Reading READ CAPACITY response failed");
    return false;
  }

  *blockNumber = BIG_TO_HOST32(capacity.LBA);
  uint32_t blockSz = BIG_TO_HOST32(capacity.BlockSize);
  *blockSize = blockSz ? blockSz : defaultBlockSize();
  return true;
}
