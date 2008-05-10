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

#ifndef KERNEL_PROCESSOR_X86_COMMON_MULTIPROCESSOR_H
#define KERNEL_PROCESSOR_X86_COMMON_MULTIPROCESSOR_H

#include <compiler.h>
#include <processor/types.h>

/** @addtogroup kernelprocessorx86common
 * @{ */

class Multiprocessor
{
  public:
    /** Information about one processor. This information is provided by the
     *  SMP or the ACPI tables. */
    struct ProcessorInformation
    {
      inline ProcessorInformation(uint8_t processorid, uint8_t apicid)
        : processorId(processorid), apicId(apicid){}
    
      /** The id of the processor */
      uint8_t processorId;
      /** The id of the processor's local APIC */
      uint8_t apicId;
    };

    /** Information about one I/O APIC. This information is provided by the
     *  SMP or the ACPI tables. */
    struct IoApicInformation
    {
      inline IoApicInformation(uint8_t apicid, physical_uintptr_t Address)
        : apicId(apicid), address(Address){}
    
      /** The id of the I/O APIC */
      uint8_t apicId;
      /** The physical address of the I/O APIC register set */
      physical_uintptr_t address;
    };

    /** Startup and initialise all processors
    *\return the number of initialised processors */
    static size_t initialise() INITIALISATION_ONLY;
};

/** @} */

#endif
