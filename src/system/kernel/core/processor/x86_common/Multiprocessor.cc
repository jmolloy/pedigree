#if defined(MULTIPROCESSOR)
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

#include <Log.h>
#include <utilities/Vector.h>
#include "Multiprocessor.h"
#include "../../../machine/x86_common/Pc.h"

#if defined(ACPI)
  #include "../../../machine/x86_common/Acpi.h"
#endif
#if defined(SMP)
  #include "../../../machine/x86_common/Smp.h"
#endif

#if !defined(APIC)
  #error APIC not defined
#endif
#if !defined(ACPI) && !defined(SMP)
  #error Neither ACPI nor SMP defined
#endif

size_t Multiprocessor::initialise()
{
  // Did we find a processr list?
  bool bMPInfoFound = false;
  // List of information about each usable processor
  const Vector<ProcessorInformation*> *Processors;

  #if defined(ACPI)
    // Search through the ACPI tables
    Acpi &acpi = Acpi::instance();
    if ((bMPInfoFound = acpi.validProcessorInfo()) == true)
      Processors = &acpi.getProcessorList();
  #endif

  #if defined(SMP)
    // Search through the SMP tables
    Smp &smp = Smp::instance();
    if (bMPInfoFound == false &&
        (bMPInfoFound = smp.valid()) == true)
      Processors = &smp.getProcessorList();
  #endif

  // No processor list found
  if (bMPInfoFound == false)
    return 1;

  NOTICE("Multiprocessor: Found " << Dec << Processors->count() << " processors");

  // Copy the trampoline code to 0x7000
  extern void *trampoline;
  extern void *trampoline_end;
  memcpy(reinterpret_cast<void*>(0x7000),
         &trampoline,
         reinterpret_cast<uintptr_t>(&trampoline_end) - reinterpret_cast<uintptr_t>(&trampoline));

  LocalApic &localApic = Pc::instance().getLocalApic();
  // Startup the application processors through startup interprocessor interrupt
  for (size_t i = 0;i < Processors->count();i++)
    if (localApic.getId() != (*Processors)[i]->apicId)
      {
        NOTICE("Multiprocessor: Booting processor #" << (*Processors)[i]->processorId);

        localApic.interProcessorInterrupt((*Processors)[i]->apicId,
                                          0x07,
                                          LocalApic::deliveryModeStartup,
                                          true,
                                          false);
      }

  return Processors->count();
}

#endif
