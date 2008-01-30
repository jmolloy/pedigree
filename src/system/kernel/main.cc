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

// If we're being called standalone, we don't get passed any BootstrapInfo.
#include "BootstrapInfo.h"

#ifdef DEBUGGER
#include <Debugger.h>
#endif

// initialiseConstructors()
#include <cppsupport.h>
// initialiseArchitecture1(), initialiseArchitecture2()
#include <machine/initialiseMachine.h>

/// Kernel entry point.
extern "C" void _main(BootstrapStruct_t *bsInf)
{
  
  // Firstly call the constructors of all global objects.
  initialiseConstructors();

  // Create a BootstrapInfo object to parse bsInf.
  BootstrapInfo bootstrapInfo(bsInf);
  
  // First stage of the machine-dependant initialisation.
  // After that only the memory-management related classes and
  // functions can be used.
  initialiseMachine1();

  // We need a heap for dynamic memory allocation.
//  initialiseMemory();

  // First stage of the machine-dependant initialisation.
  // After that every machine dependant class & function can be used.
  initialiseMachine2();
  
#if defined(DEBUGGER) && defined(DEBUGGER_RUN_AT_START)
  Debugger::instance().breakpoint(DEBUGGER_RUN_AT_START);
#endif

  // Then get the BootstrapInfo object to convert its contents into
  // C++ classes.
//  g_pKernelMemoryMap = bsInf->getMemoryMap();  // Get the bios' memory map.
//  g_pProcessors      = bsInf->getProcessors(); // Parse the SMP table.
  
  
}
