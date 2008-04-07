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
#include "../../core/processor/x86_common/PhysicalMemoryManager.h"

Acpi Acpi::m_Instance;

void Acpi::initialise()
{
  // Search for the ACPI root system description pointer
  if (find() == false)
  {
    WARNING("Acpi: not compliant to the ACPI Specification");
    return;
  }

  NOTICE("ACPI Specification");
  NOTICE(" RSDT pointer at " << Hex << reinterpret_cast<uintptr_t>(m_pRsdtPointer));
  NOTICE(" RSDT at " << Hex << m_pRsdtPointer->rsdtAddress);

  // Get the ACPI memory ranges
  X86CommonPhysicalMemoryManager &physicalMemoryManager = X86CommonPhysicalMemoryManager::instance();
  const RangeList<uint64_t> &AcpiRanges = physicalMemoryManager.getAcpiRanges();
  if (AcpiRanges.size() == 0)
  {
    ERROR("Acpi: No ACPI memory range");
    return;
  }
  if (AcpiRanges.size() > 1)
  {
    ERROR("Acpi: More than one ACPI memory range");
    return;
  }

  // Allocate the ACPI memory as a MemoryRegion
  RangeList<uint64_t>::Range AcpiRange = AcpiRanges.getRange(0);
  physical_uintptr_t address = AcpiRange.address & (~(PhysicalMemoryManager::getPageSize() - 1));
  size_t sAddress = AcpiRange.length + (AcpiRange.address - address);
  size_t nPages = (sAddress + (PhysicalMemoryManager::getPageSize() - 1)) / PhysicalMemoryManager::getPageSize();
  if (physicalMemoryManager.allocateRegion(m_AcpiMemoryRegion,
                                           nPages,
                                           PhysicalMemoryManager::continuous,
                                           address)
      == false)
  {
    ERROR("Acpi: Could not allocate the MemoryRegion");
    return;
  }

  // Check the RSDT (Root System Description Table)
  m_pRsdt = m_AcpiMemoryRegion.convertPhysicalPointer<SystemDescriptionTableHeader>(m_pRsdtPointer->rsdtAddress);
  // HACK FIXME TODO
  // NOTICE("m_pRsdt " << Hex << reinterpret_cast<uintptr_t>(m_pRsdt));
  // NOTICE("Acpi range length " << Hex << m_AcpiMemoryRegion.size());
  // NOTICE("m_pRsdt->length " << Hex << m_pRsdt->length);
  if (m_pRsdt->signature != 0x54445352 ||
      checksum(m_pRsdt) != true)
  {
    ERROR("Acpi: RSDT invalid");
    m_pRsdt = 0;
    return;
  }

  // Go through the table's entries
  size_t sEntries = (m_pRsdt->length - sizeof(SystemDescriptionTableHeader)) / 4;
  for (size_t i = 0;i < sEntries;i++)
  {
    uint32_t *pTable = adjust_pointer(reinterpret_cast<uint32_t*>(m_pRsdt), sizeof(SystemDescriptionTableHeader) + 4 * i);

    SystemDescriptionTableHeader *pSystemDescTable = m_AcpiMemoryRegion.convertPhysicalPointer<SystemDescriptionTableHeader>(*pTable);

    char Signature[5];
    strncpy(Signature, reinterpret_cast<char*>(&pSystemDescTable->signature), 4);
    Signature[4] = '\0';

    NOTICE("  " << Signature << " at " << Hex << *pTable);

    // Is the table valid?
    if (checksum(pSystemDescTable) != true)
    {
      ERROR("  invalid");
      continue;
    }

    // Is Fixed ACPI Description Table?
    if (pSystemDescTable->signature == 0x50434146)
      m_pFacp = pSystemDescTable;
    // Is Multiple APIC Description Table?
    else if (pSystemDescTable->signature == 0x43495041)
      m_pApic = pSystemDescTable;
    else
      NOTICE("  unknown table");
  }
}

#if defined(MULTIPROCESSOR)
  bool Acpi::getProcessorList(physical_uintptr_t &localApicsAddress,
                              Vector<ProcessorInformation*> &Processors,
                              Vector<IoApicInformation*> &IoApics,
                              bool &bHasPics,
                              bool &bPicMode)
  {
    // Was the Multiple APIC Description Table found?
    if (m_pApic == 0)return false;

    NOTICE("ACPI: Multiple APIC Description Table");

    // Parse the Multiple APIC Description Table
    uint32_t *pLocalApicAddress = reinterpret_cast<uint32_t*>(adjust_pointer(m_pApic, sizeof(SystemDescriptionTableHeader)));
    uint32_t *pFlags = adjust_pointer(pLocalApicAddress, 4);

    localApicsAddress = *pLocalApicAddress;
    bHasPics = (((*pFlags) & 0x01) == 0x01);

    NOTICE(" local APICs at " << Hex << localApicsAddress);
    NOTICE(" dual 8259 PICs: " << bHasPics);

    uint8_t *pType = reinterpret_cast<uint8_t*>(adjust_pointer(pFlags, 4));
    for (;pType < reinterpret_cast<uint8_t*>(adjust_pointer(m_pApic, m_pApic->length));)
    {
      // Processor Local APIC
      if (*pType == 0)
      {
        ProcessorLocalApic *pLocalApic = reinterpret_cast<ProcessorLocalApic*>(adjust_pointer(pType, 2));

        // TODO: BSP?
        bool bUsable = ((pLocalApic->flags & 0x01) == 0x01);

        NOTICE("  processor #" << Dec << pLocalApic->processorId << (bUsable ? " usable" : " unusable"));

        // Is the processor usable?
        if (bUsable)
        {
          // Add the processor to the list
          ProcessorInformation *pProcessorInfo = new ProcessorInformation(false,// TODO BSP
                                                                          pLocalApic->processorId,
                                                                          pLocalApic->apicId);
          Processors.pushBack(pProcessorInfo);
        }
      }
      // I/O APIC
      else if (*pType == 1)
      {
        IoApic *pIoApic = reinterpret_cast<IoApic*>(adjust_pointer(pType, 2));

        NOTICE("  I/O APIC #" << Dec << pIoApic->apicId << " at " << Hex << pIoApic->address << ", global system interrupt base " << pIoApic->globalSystemInterruptBase);

        // TODO: What should we do with the global system interrupt base?

        // Add to the I/O APIC list
        IoApicInformation *pIoApicInfo = new IoApicInformation(pIoApic->apicId, pIoApic->address);
        IoApics.pushBack(pIoApicInfo);
      }
      // Interrupt Source override
      else if (*pType == 2)
      {
        InterruptSourceOverride *pInterruptSourceOverride = reinterpret_cast<InterruptSourceOverride*>(adjust_pointer(pType, 2));

        ERROR("  interrupt source override on " << ((pInterruptSourceOverride->bus == 0) ? "ISA" : "unknown") << " bus: source #" << Dec << pInterruptSourceOverride->source << ", global system interrupt #" << pInterruptSourceOverride->globalSystemInterrupt << ", flags " << Hex << pInterruptSourceOverride->flags);

        // TODO
      }
      // Non-maskable Interrupt Source (NMI)
      else if (*pType == 3)
      {
        ERROR("  NMI source");
      }
      // Local APIC NMI
      else if (*pType == 4)
      {
        ERROR("  Local APIC NMI");
      }
      // Local APIC Address Override
      else if (*pType == 5)
      {
        ERROR("  Local APIC address override");
      }
      // I/O SAPIC
      else if (*pType == 6)
      {
        ERROR("  I/O SAPIC");
      }
      // Local SAPIC
      else if (*pType == 7)
      {
        ERROR("  Local SAPIC");
      }
      // Platform Interrupt Source
      else if (*pType == 8)
      {
        ERROR("  Platform Interrupt Source");
      }
      else
      {
        NOTICE("  unknown entry #" << Dec << *pType);
      }

      // Go to the next entry;
      uint8_t *sTable = reinterpret_cast<uint8_t*>(adjust_pointer(pType, 1));
      pType = adjust_pointer(pType, *sTable);
    }

    // TODO: Set bPicMode!
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

bool Acpi::checksum(const SystemDescriptionTableHeader *pHeader)
{
  return ::checksum(reinterpret_cast<const uint8_t*>(pHeader), pHeader->length);
}
