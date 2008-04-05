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
#include <Log.h>
#include <utilities/utility.h>
#include "Acpi.h"

Acpi Acpi::m_Instance;

void Acpi::initialise()
{
  // Search for the ACPI root system description pointer
  if (find() == false)
  {
    WARNING("smp: not compliant to the ACPI specification");
    return;
  }

  NOTICE("ACPI specification RSDT pointer found at 0x" << Hex << reinterpret_cast<uintptr_t>(m_pRsdtPointer));
}

#if defined(MULTIPROCESSOR)
  bool Acpi::getProcessorList(Vector<ProcessorInformation*> &Processors,
                              bool &bPicMode)
  {
    if (m_pRsdtPointer == 0)return false;

    // TODO
    return false;
  }
#endif

bool Acpi::find()
{
  // Search in the first kilobyte of the EBDA
  uint16_t *ebdaSegment = reinterpret_cast<uint16_t*>(0x40E);
  m_pRsdtPointer = find(reinterpret_cast<void*>((*ebdaSegment) * 16), 0x400);

  if (m_pRsdtPointer == 0)
  {
    // Search in the BIOS ROM address space
    m_pRsdtPointer = find(reinterpret_cast<void*>(0xE0000), 0x20000);
  }

  return (m_pRsdtPointer != 0);
}

Acpi::RsdtPointer *Acpi::find(void *pMemory, size_t sMemory)
{
  RsdtPointer *pRdstPointer = reinterpret_cast<RsdtPointer*>(pMemory);
  while (reinterpret_cast<uintptr_t>(pRdstPointer) < (reinterpret_cast<uintptr_t>(pMemory) + sMemory))
  {
    if (pRdstPointer->signature == 0x2052545020445352ULL &&
        checksum(pRdstPointer)
        == true)
      return pRdstPointer;
    pRdstPointer = adjust_pointer(pRdstPointer, 16);
  }
  return 0;
}

bool Acpi::checksum(const RsdtPointer *pRdstPointer)
{
  // ACPI 1.0 checksum
  if (::checksum(reinterpret_cast<const uint8_t*>(pRdstPointer), 20) == false)
    return false;

  if (pRdstPointer->revision >= 2)
  {
    // ACPI 2.0+ checksum
    if (::checksum(reinterpret_cast<const uint8_t*>(pRdstPointer), pRdstPointer->length) == false)
      return false;
  }

  return true;
}
