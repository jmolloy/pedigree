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
      bool getProcessorList(Vector<ProcessorInformation*> &Processors,
                            bool &bPicMode);
    #endif

  private:
    inline Acpi()
      : m_pRsdtPointer(0){}
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

    bool find();
    RsdtPointer *find(void *pMemory, size_t sMemory);
    bool checksum(const RsdtPointer *pRdstPointer);

    RsdtPointer *m_pRsdtPointer;

    static Acpi m_Instance;
};

/** @} */

#endif
