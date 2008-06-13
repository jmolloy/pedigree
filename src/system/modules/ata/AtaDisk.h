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

/** An ATA disk device. Most read/write commands get channeled upstream
 * to the controller, as it has to multiplex between multiple disks. */
class AtaDisk : public Disk
{
public:
  AtaDisk(class AtaController *pDev, bool isMaster);
  ~AtaDisk();

  virtual void getName(NormalStaticString &str)
  {
    str += m_pName;
  }

  /** Tries to detect if this device is present.
   * \return True if the device is present and was successfully initialised. */
  bool initialise();

private:
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
};

#endif
