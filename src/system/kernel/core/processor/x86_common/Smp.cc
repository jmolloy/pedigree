/*
 * Copyright (c) 2008 James Molloy, James Pritchett, Jörg Pfähler, Matthew Iselin
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
#include "Smp.h"
#include <Log.h>
#include <utilities/utility.h>

bool Smp::getProcessorList(Vector<ProcessorInformation*> &Processors,
                           bool &bPicMode)
{
  // Search for the multiprocessor floating pointer structure
  if (find() == false)
  {
    WARNING("smp: not compliant to the intel multiprocessor specification");
    return false;
  }

  NOTICE("smp: Intel multiprocessor specification 1." << Dec << m_pFloatingPointer->revision);
  NOTICE("     floating pointer at 0x" << Hex << reinterpret_cast<uintptr_t>(m_pFloatingPointer));

  // PIC-Mode implemented?
  bPicMode = ((m_pFloatingPointer->features[1] & 0x80) == 0x80);

  // One of the default configurations?
  if (m_pFloatingPointer->features[0] != 0)
  {
    ERROR("smp: default configurations (#" << Dec << m_pFloatingPointer->features[0] << ") not supported");
    return false;
  }

  // Check the configuration table
  m_pConfigTable = reinterpret_cast<ConfigTableHeader*>(m_pFloatingPointer->physicalAddress);
  if (m_pConfigTable == 0)
  {
    ERROR("smp: configuration table not present");
    return false;
  }

  NOTICE("     configuration table at 0x" << Hex << reinterpret_cast<uintptr_t>(m_pConfigTable));

  if (m_pConfigTable->signature != 0x504D4350 ||
      checksum(m_pConfigTable) != true ||
      m_pConfigTable->revision != m_pConfigTable->revision)
  {
    ERROR("smp: configuration table invalid");
    return false;
  }

  return true;
}

bool Smp::find()
{
  // Search in the first kilobyte of the EBDA
  uint16_t *ebdaSegment = reinterpret_cast<uint16_t*>(0x40E);
  m_pFloatingPointer = find(reinterpret_cast<void*>((*ebdaSegment) * 16), 0x400);

  if (m_pFloatingPointer == 0)
  {
    // Search in the last kilobyte of the base memory
    uint16_t *baseSize = reinterpret_cast<uint16_t*>(0x413);
    m_pFloatingPointer = find(reinterpret_cast<void*>((*baseSize - 1) * 1024), 0x400);

    if (m_pFloatingPointer == 0)
    {
      // Search in the BIOS ROM address space
      m_pFloatingPointer = find(reinterpret_cast<void*>(0xF0000), 0x10000);
    }
  }

  return (m_pFloatingPointer != 0);
}

Smp::FloatingPointer *Smp::find(void *pMemory, size_t sMemory)
{
  FloatingPointer *pFloatingPointer = reinterpret_cast<FloatingPointer*>(pMemory);
  while (reinterpret_cast<uintptr_t>(pFloatingPointer) < (reinterpret_cast<uintptr_t>(pMemory) + sMemory))
  {
    if (pFloatingPointer->signature == 0x5F504D5F &&
        checksum(pFloatingPointer)
        == true)
      return pFloatingPointer;
    pFloatingPointer = adjust_pointer(pFloatingPointer, 16);
  }
  return 0;
}

bool Smp::checksum(const FloatingPointer *pFloatingPointer)
{
  return ::checksum(reinterpret_cast<const uint8_t*>(pFloatingPointer), pFloatingPointer->length * 16);
}

bool Smp::checksum(const ConfigTableHeader *pConfigTable)
{
  // Base table checksum
  if (::checksum(reinterpret_cast<const uint8_t*>(pConfigTable), pConfigTable->baseTableLength) == false)
    return false;

  // Extended table checksum
  uint8_t sum = pConfigTable->extendedChecksum;
  for (size_t i = 0;i < pConfigTable->extendedTableLength;i++)
    sum += reinterpret_cast<const uint8_t*>(pConfigTable)[pConfigTable->baseTableLength + i];
  if (sum != 0)return false;

  return true;
}
