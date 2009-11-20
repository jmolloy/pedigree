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
#ifndef ATA_ATAPI_DISK_H
#define ATA_ATAPI_DISK_H

#include <processor/types.h>
#include <machine/Device.h>
#include <machine/Disk.h>
#include <machine/Controller.h>
#include <process/Mutex.h>
#include <utilities/Cache.h>
#include <processor/MemoryRegion.h>
#include <processor/PhysicalMemoryManager.h>
#include "AtaDisk.h"
#include "BusMasterIde.h"

/** An ATAPI disk device. Most read/write commands get channeled upstream
 * to the controller, as it has to multiplex between multiple disks. */
class AtapiDisk : public AtaDisk
{
private:
  enum AtapiType
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
    None = 0x7F
  };

  struct Sense
  {
    uint8_t ResponseCode;
    uint8_t SenseKey;
    uint8_t Asc;
    uint8_t AscQ;
    uint8_t Rsvd[3];
    uint8_t Add_Len;
  } __attribute__((packed));

  struct Inquiry
  {
    uint8_t Peripheral;
    uint8_t Removable;
    uint8_t Version;
    uint8_t Flags1;
    uint8_t AddLength;
    uint8_t Rsvd[2];
    uint8_t Flags2;
    uint8_t VendIdent[8];
    uint8_t ProdIdent[16];
    uint8_t ProdRev[4];
  } __attribute__((packed));

  struct TocEntry
  {
    uint8_t Rsvd1;
    uint8_t Flags;
    uint8_t TrackNum;
    uint8_t Rsvd2;
    uint32_t TrackStart;
  } __attribute__((packed));

public:
  AtapiDisk(class AtaController *pDev, bool isMaster, IoBase *commandRegs, IoBase *controlRegs, BusMasterIde *busMaster = 0);
  virtual ~AtapiDisk();

  virtual void getName(String &str)
  {
    str = m_pName;
  }

  virtual SubType getSubType()
  {
    return ATAPI;
  }

  /** Tries to detect if this device is present.
   * \return True if the device is present and was successfully initialised. */
  bool initialise();

  // These are the internal functions that the controller calls when it is ready to process our request.
  virtual uint64_t doRead(uint64_t location);
  virtual uint64_t doRead2(uint64_t location, uintptr_t buffer, size_t buffSize);
  virtual uint64_t doWrite(uint64_t location);

  // Called by our controller when an IRQ has been received.
  // It may not actually apply to us!
  void irqReceived();

  // Is this an ATAPI device?
  inline virtual bool isAtapi()
  {
    return true;
  }

  // Obtains the type of this ATAPI device
  inline AtapiType getAtapiType()
  {
    return m_Type;
  }

  /** Gets the capacity of the disk */
  inline void getCapacity(size_t *blockNumber, size_t *blockSize)
  {
    *blockNumber = m_NumBlocks;
    *blockSize = m_BlockSize;
  }

private:
  /** \note This interface assumes you know what you're doing when you call it.
    *       It'd be safer to integrate it into one big function, but it's also
    *       a little easier to read code using this interface style. If it
    *       ends up causing problsm, it's easy to change.
    */

  /** Sends a command packet (ATAPI)
    * \param nRespBytes The number of bytes to be read/written as a result of this command
    * \param nPackBytes The number of bytes in the packet buffer
    * \param packet Location of the command packet to write
    */
  bool sendCommand(size_t nRespBytes, uintptr_t respBuff, size_t nPackBytes, uintptr_t packet, bool bWrite = false);
  /** Reads the response from the command packet (as sent above) */
  bool readPacket(size_t maxBytes, uintptr_t buffer);
  /** Writes data required by the command packet (as sent above) */
  bool writePacket(size_t maxBytes, uintptr_t buffer);

  /** Reads sense data from an ATAPI device */
  bool readSense(uintptr_t buff);
  /** Tests to see if the unit is ready */
  bool unitReady();

  /** Default block size for a device */
  inline size_t defaultBlockSize()
  {
    if(m_Type == CdDvd)
      return 2048;
    else
    {
        //FATAL("Uhhhhhhhh :(");
      return 512;
    }
  }

  /** Gets the capacity of the disk (for internal use) */
  bool getCapacityInternal(size_t *blockNumber, size_t *blockSize);

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
  /** Type of this ATAPI device */
  AtapiType m_Type;

  /** Number of blocks, and size of each block */
  size_t m_NumBlocks;
  size_t m_BlockSize;

  /** Command packet size */
  uint8_t m_PacketSize;

  /** Device flags */
  bool m_Removable;

  /** This mutex is released by the IRQ handler when an IRQ is received, to wake the working thread.
   * \todo A condvar would really be better here. */
  Mutex m_IrqReceived;

  /** MemoryRegion for the PRD table */
  MemoryRegion m_PrdTableMemRegion;
};

#endif
