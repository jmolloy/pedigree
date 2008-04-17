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
#include <processor/VirtualAddressSpace.h>
#include "Acpi.h"
#include "../../core/processor/x86_common/PhysicalMemoryManager.h"

#if !defined(ACPI_NOTICE)
  #undef NOTICE
  #define NOTICE(x)
#endif
#if !defined(ACPI_ERROR)
  #undef ERROR
  #define ERROR(x)
#endif

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

  // XSDT present?
  if (m_pRsdtPointer->revision >= 2)
    if (m_pRsdtPointer->xsdtAddress != 0)
      NOTICE(" XSDT at " << Hex << m_pRsdtPointer->xsdtAddress);

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
                                           VirtualAddressSpace::KernelMode | VirtualAddressSpace::Write,
                                           PhysicalMemoryManager::continuous,
                                           address)
      == false)
  {
    ERROR("Acpi: Could not allocate the MemoryRegion");
    return;
  }

  // Check the RSDT (Root System Description Table)
  m_pRsdt = m_AcpiMemoryRegion.convertPhysicalPointer<SystemDescriptionTableHeader>(m_pRsdtPointer->rsdtAddress);
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
      ERROR("   invalid");
      continue;
    }

    // Is Fixed ACPI Description Table?
    if (pSystemDescTable->signature == 0x50434146)
      m_pFacp = reinterpret_cast<FixedACPIDescriptionTable*>(pSystemDescTable);
    // Is Multiple APIC Description Table?
    #if defined(APIC)
    else if (pSystemDescTable->signature == 0x43495041)
      m_pApic = pSystemDescTable;
    #endif
    else
      NOTICE("   unknown table");
  }

  // Do we have a Fixed ACPI Description Table?
  if (m_pFacp == 0)
  {
    ERROR("Acpi: no Fixed ACPI Description Table (FACP)");
    return;
  }

  // Parse the FACP
  parseFixedACPIDescriptionTable();

  #if defined(APIC)
    // If we have an Multiple APIC Description Table parse it
    if (m_pApic != 0)
      parseMultipleApicDescriptionTable();
  #endif

  m_bValid = true;
}

Acpi::Acpi()
  : m_bValid(0), m_pRsdtPointer(0), m_AcpiMemoryRegion("ACPI"), m_pRsdt(0), m_pFacp(0)
  #if defined(APIC)
    , m_pApic(0), m_bValidApicInfo(false), m_bHasPICs(false), m_LocalApicAddress(0), m_IoApics()
    #if defined(MULTIPROCESSOR)
      , m_Processors()
    #endif
  #endif
{
}

void Acpi::parseFixedACPIDescriptionTable()
{
  NOTICE("ACPI: Fixed Description Table (FACP)");

  NOTICE(" Firmware ACPI Control Structure at " << Hex << m_pFacp->firmwareControl);
  NOTICE(" Differentiated System Description Table at " << Hex << m_pFacp->dsdt);
  NOTICE(" Interrupt Model: " << Dec << ((m_pFacp->interruptModel == 0) ? "dual 8259 PICs" : "multiple local APICs"));
  NOTICE(" SCI Interrupt #" << Dec << m_pFacp->sciInterrupt);
  NOTICE(" SMI Command Port at " << Hex << m_pFacp->smiCommandPort);
  NOTICE("  enable " << Hex << m_pFacp->acpiEnableCommand);
  NOTICE("  disable " << Hex << m_pFacp->acpiDisableCommand);
  if (m_pFacp->s4BiosCommand != 0)
    NOTICE("  S4 BIOS " << Hex << m_pFacp->s4BiosCommand);
  NOTICE(" Power-Management");
  NOTICE("  Event 1A at " << Hex << m_pFacp->pm1aEventBlock << " - " << (m_pFacp->pm1aEventBlock + m_pFacp->pm1EventLength));
  if (m_pFacp->pm1bEventBlock != 0)
    NOTICE("  Event 1B at " << Hex << m_pFacp->pm1bEventBlock << " - " << (m_pFacp->pm1bEventBlock + m_pFacp->pm1EventLength));
  NOTICE("  Control 1A at " << Hex << m_pFacp->pm1aControlBlock << " - " << (m_pFacp->pm1aControlBlock + m_pFacp->pm1ControlLength));
  if (m_pFacp->pm1bControlBlock != 0)
    NOTICE("  Control 1B at " << Hex << m_pFacp->pm1bControlBlock << " - " << (m_pFacp->pm1bControlBlock + m_pFacp->pm1ControlLength));
  if (m_pFacp->pm2ControlBlock != 0)
    NOTICE("  Control 2 at " << Hex << m_pFacp->pm2ControlBlock << " - " << (m_pFacp->pm2ControlBlock + m_pFacp->pm2ControlLength));
  NOTICE("  Timer at " << Hex << m_pFacp->pmTimerBlock << " - " << (m_pFacp->pmTimerBlock + m_pFacp->pmTimerLength));
  if (m_pFacp->gpe0Block != 0)
    NOTICE("  General Purpose Event 0 at " << Hex << m_pFacp->gpe0Block << " - " << (m_pFacp->gpe0Block + m_pFacp->gpe0BlockLength));
  if (m_pFacp->gpe1Block != 0)
    NOTICE("  General Purpose Event 1 at " << Hex << m_pFacp->gpe1Block << " - " << (m_pFacp->gpe1Block + m_pFacp->gpe1BlockLength));
  if (m_pFacp->gpe1Base != 0)
    NOTICE("  General Purpose Event Base: " << Hex << m_pFacp->gpe1Base);
  // TODO: Unimportant imho
  NOTICE(" Flags " << Hex << m_pFacp->flags);
}

#if defined(APIC)
  void Acpi::parseMultipleApicDescriptionTable()
  {
    NOTICE("ACPI: Multiple APIC Description Table (APIC)");
  
    // Parse the Multiple APIC Description Table
    uint32_t *pLocalApicAddress = reinterpret_cast<uint32_t*>(adjust_pointer(m_pApic, sizeof(SystemDescriptionTableHeader)));
    uint32_t *pFlags = adjust_pointer(pLocalApicAddress, 4);

    m_LocalApicAddress = *pLocalApicAddress;
    m_bHasPICs = (((*pFlags) & 0x01) == 0x01);

    uint8_t *pType = reinterpret_cast<uint8_t*>(adjust_pointer(pFlags, 4));
    for (;pType < reinterpret_cast<uint8_t*>(adjust_pointer(m_pApic, m_pApic->length));)
    {
      // Processor Local APIC
      if (*pType == 0)
      {
        ProcessorLocalApic *pLocalApic = reinterpret_cast<ProcessorLocalApic*>(adjust_pointer(pType, 2));
        // TODO: BSP?
        bool bUsable = ((pLocalApic->flags & 0x01) == 0x01);
        NOTICE(" Processor #" << Dec << pLocalApic->processorId << (bUsable ? " usable" : " unusable"));

        #if defined(MULTIPROCESSOR)
          // Is the processor usable?
          if (bUsable)
          {
            // Add the processor to the list
            ProcessorInformation *pProcessorInfo = new ProcessorInformation(pLocalApic->processorId,
                                                                            pLocalApic->apicId);
            m_Processors.pushBack(pProcessorInfo);
          }
        #endif
      }
      // I/O APIC
      else if (*pType == 1)
      {
        IoApic *pIoApic = reinterpret_cast<IoApic*>(adjust_pointer(pType, 2));
  
        NOTICE(" I/O APIC #" << Dec << pIoApic->apicId << " at " << Hex << pIoApic->address << ", global system interrupt base " << pIoApic->globalSystemInterruptBase);
  
        // TODO: What should we do with the global system interrupt base?
  
        // Add to the I/O APIC list
        IoApicInformation *pIoApicInfo = new IoApicInformation(pIoApic->apicId, pIoApic->address);
        m_IoApics.pushBack(pIoApicInfo);
      }
      // Interrupt Source override
      else if (*pType == 2)
      {
        InterruptSourceOverride *pInterruptSourceOverride = reinterpret_cast<InterruptSourceOverride*>(adjust_pointer(pType, 2));
  
        ERROR(" Interrupt override: " << ((pInterruptSourceOverride->bus == 0) ? "ISA" : "unknown") << " bus, #" << Dec << pInterruptSourceOverride->source << " -> #" << pInterruptSourceOverride->globalSystemInterrupt << ", flags " << Hex << pInterruptSourceOverride->flags);
  
        // TODO
      }
      // Non-maskable Interrupt Source (NMI)
      else if (*pType == 3)
      {
        ERROR(" NMI source");
      }
      // Local APIC NMI
      else if (*pType == 4)
      {
        ERROR(" Local APIC NMI");
      }
      // Local APIC Address Override
      else if (*pType == 5)
      {
        ERROR(" Local APIC address override");
      }
      // I/O SAPIC
      else if (*pType == 6)
      {
        ERROR(" I/O SAPIC");
      }
      // Local SAPIC
      else if (*pType == 7)
      {
        ERROR(" Local SAPIC");
      }
      // Platform Interrupt Source
      else if (*pType == 8)
      {
        ERROR(" Platform Interrupt Source");
      }
      else
      {
        NOTICE(" unknown entry #" << Dec << *pType);
      }
  
      // Go to the next entry;
      uint8_t *sTable = reinterpret_cast<uint8_t*>(adjust_pointer(pType, 1));
      pType = adjust_pointer(pType, *sTable);
    }

    m_bValidApicInfo = true;
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
        checksum(pRdstPointer) == true)
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
