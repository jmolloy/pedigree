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
#ifndef KERNEL_MACHINE_ACPI_H
#define KERNEL_MACHINE_ACPI_H

#include <compiler.h>
#include <processor/types.h>
#include <processor/MemoryRegion.h>
#include <utilities/Vector.h>
#include "../../core/processor/x86_common/Multiprocessor.h"

/** @addtogroup kernelmachinex86common
 * @{ */

class Acpi
{
  public:
    inline static Acpi &instance()
      {return m_Instance;}

    void initialise();

    #if defined(MULTIPROCESSOR)
      bool getProcessorList(physical_uintptr_t &localApicsAddress,
                            Vector<ProcessorInformation*> &Processors,
                            Vector<IoApicInformation*> &IoApics,
                            bool &bHasPics,
                            bool &bPicMode);
    #endif

  private:
    inline Acpi()
      : m_pRsdtPointer(0), m_AcpiMemoryRegion(), m_pRsdt(0), m_pFacp(0), m_pApic(0){}
    Acpi(const Acpi &);
    Acpi &operator = (const Acpi &);
    inline ~Acpi(){}

    struct RsdtPointer
    {
      // ACPI 1.0+
      uint64_t signature;
      uint8_t checksum;
      char oemId[6];
      uint8_t revision;
      uint32_t rsdtAddress;

      // ACPI 2.0+
      uint32_t length;
      uint64_t xsdtAddress;
      uint8_t extendedChecksum;
      uint8_t reserved[3];
    } PACKED;

    struct SystemDescriptionTableHeader
    {
      uint32_t signature;
      uint32_t length;
      uint8_t revision;
      uint8_t checksum;
      char oemId[6];
      char oemTableId[8];
      uint32_t oemRevision;
      uint32_t creatorId;
      uint32_t creatorRevision;
    } PACKED;

    struct ProcessorLocalApic
    {
      uint8_t processorId;
      uint8_t apicId;
      uint32_t flags;
    } PACKED;

    struct IoApic
    {
      uint8_t apicId;
      uint8_t reserved;
      uint32_t address;
      uint32_t globalSystemInterruptBase;
    } PACKED;

    struct InterruptSourceOverride
    {
      uint8_t bus;
      uint8_t source;
      uint8_t globalSystemInterrupt;
      uint16_t flags;
    } PACKED;

    bool find();
    RsdtPointer *find(void *pMemory, size_t sMemory);
    bool checksum(const RsdtPointer *pRdstPointer);
    bool checksum(const SystemDescriptionTableHeader *pHeader);

    RsdtPointer *m_pRsdtPointer;
    MemoryRegion m_AcpiMemoryRegion;
    SystemDescriptionTableHeader *m_pRsdt;
    SystemDescriptionTableHeader *m_pFacp;
    SystemDescriptionTableHeader *m_pApic;

    static Acpi m_Instance;
};

/** @} */

#endif
