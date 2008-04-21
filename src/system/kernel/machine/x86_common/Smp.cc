#if defined(SMP)
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

#if !defined(SMP_NOTICE)
  #undef NOTICE
  #define NOTICE(x)
#endif
#if !defined(SMP_ERROR)
  #undef ERROR
  #define ERROR(x)
#endif

Smp Smp::m_Instance;

void Smp::initialise()
{
  // Search for the multiprocessor floating pointer structure
  if (find() == false)
  {
    ERROR("smp: not compliant to the Intel Multiprocessor Specification");
    return;
  }

  NOTICE("Intel Multiprocessor Specification 1." << Dec << m_pFloatingPointer->revision);
  NOTICE(" floating pointer at " << Hex << reinterpret_cast<uintptr_t>(m_pFloatingPointer));

  // One of the default configurations?
  if (m_pFloatingPointer->features[0] != 0)
  {
    ERROR("smp: default configurations (#" << Dec << m_pFloatingPointer->features[0] << ") not supported");
    return;
  }

  // Check the configuration table
  m_pConfigTable = reinterpret_cast<ConfigTableHeader*>(m_pFloatingPointer->physicalAddress);
  if (m_pConfigTable == 0)
  {
    ERROR("smp: configuration table not present");
    return;
  }
  if (reinterpret_cast<uintptr_t>(m_pConfigTable) >= 0x100000)
  {
    ERROR("smp: configuration table above 1MB");
    return;
  }

  NOTICE(" configuration table at " << Hex << reinterpret_cast<uintptr_t>(m_pConfigTable));

  if (m_pConfigTable->signature != 0x504D4350 ||
      checksum(m_pConfigTable) != true ||
      m_pConfigTable->revision != m_pConfigTable->revision)
  {
    ERROR("smp: configuration table invalid");
    return;
  }

  #if defined(APIC)

    // PIC-Mode implemented?
    m_bPICMode = ((m_pFloatingPointer->features[1] & 0x80) == 0x80);

    // Local APIC address
    m_LocalApicAddress = m_pConfigTable->localApicAddress;

  #endif

  // Loop through the configuration table base entries
  uint8_t *pType = reinterpret_cast<uint8_t*>(adjust_pointer(m_pConfigTable, sizeof(ConfigTableHeader)));
  for (size_t i = 0;i < m_pConfigTable->entryCount;i++)
  {
    // Size of this table entry
    size_t sEntry = 8;

    // Processor entry?
    if (*pType == 0)
    {
      Processor *pProcessor = reinterpret_cast<Processor*>(pType);

      bool bUsable = ((pProcessor->flags & 0x01) == 0x01);

      NOTICE("  Processor #" << Dec << pProcessor->localApicId << (bUsable ? " usable" : " unusable"));

      #if defined(MULTIPROCESSOR)
        // Is the processor usable?
        if (bUsable)
        {
          // Add the processor to the list
          Multiprocessor::ProcessorInformation *pProcessorInfo = new Multiprocessor::ProcessorInformation(pProcessor->localApicId,
                                                                                                          pProcessor->localApicId);
          m_Processors.pushBack(pProcessorInfo);
        }
      #endif

      sEntry = sizeof(Processor);
    }
    // Bus entry?
    else if (*pType == 1)
    {
      Bus *pBus = reinterpret_cast<Bus*>(pType);

      char name[7];
      strncpy(name, pBus->name, 6);
      name[6] = '\0';

      NOTICE("  Bus #" << Dec << pBus->busId << " \"" << name << "\"");

      // TODO: Figure out how to pass these entries to the calling function
    }
    // I/O APIC entry?
    else if (*pType == 2)
    {
      IoApic *pIoApic = reinterpret_cast<IoApic*>(pType);

      bool bUsable = ((pIoApic->flags & 0x01) == 0x01);

      NOTICE("  I/O APIC #" << Dec << pIoApic->id << (bUsable ? " usable" : "") << " at " << Hex << pIoApic->address);

      #if defined(APIC)
        // Is the I/O APIC usable?
        if (bUsable)
        {
          // Add the I/O APIC to the list
          Multiprocessor::IoApicInformation *pIoApicInfo = new Multiprocessor::IoApicInformation(pIoApic->id, pIoApic->address);
          m_IoApics.pushBack(pIoApicInfo);
        }
      #endif
    }
    // I/O interrupt assignment?
    else if (*pType == 3)
    {
      // IoInterruptAssignment *pIoInterruptAssignment = reinterpret_cast<IoInterruptAssignment*>(pType);

      // TODO: Figure out how to pass these entries to the calling function
    }
    // Local interrupt assignment?
    else if (*pType == 4)
    {
      // LocalInterruptAssignment *pLocalInterruptAssignment = reinterpret_cast<LocalInterruptAssignment*>(pType);

      // TODO: Figure out how to pass these entries to the calling function
    }

    pType = adjust_pointer(pType, sEntry);
  }

  // TODO: Parse the extended table entries

  m_bValid = true;
}

Smp::Smp()
  : m_bValid(false), m_pFloatingPointer(0), m_pConfigTable(0)
  #if defined(APIC)
    ,m_bPICMode(false), m_LocalApicAddress(0), m_IoApics()
    #if defined(MULTIPROCESSOR)
      ,m_Processors()
    #endif
  #endif
{
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
        checksum(pFloatingPointer) == true)
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

#endif
