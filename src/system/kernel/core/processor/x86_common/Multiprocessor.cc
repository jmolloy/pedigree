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
#include "Multiprocessor.h"
#include <utilities/Vector.h>

#if defined(ACPI)
  #include "../../../machine/x86_common/Acpi.h"
#endif
#if defined(SMP)
  #include "Smp.h"
#endif

size_t initialiseMultiprocessor()
{
  bool bMPInfoFound = false;

  bool bPicMode;
  Vector<ProcessorInformation*> Processors;

  // Search through the ACPI tables
  #if defined(ACPI)
    Acpi &acpi = Acpi::instance();
    bMPInfoFound = acpi.getProcessorList(Processors, bPicMode);
  #endif

  // Search through the SMP tables
  if (bMPInfoFound == false)
  {
    #if defined(SMP)
      Smp smp;
      bMPInfoFound = smp.getProcessorList(Processors, bPicMode);
    #endif
  }

  if (bMPInfoFound == false)
    return 1;

  // TODO: Evaluate the table and start the application processors
  //       Set mProcessors
  return 1;
}
