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

#include <Log.h>
#include <Elf32.h>

#include <utilities/StaticString.h>

// initialiseConstructors()
#include "cppsupport.h"
// initialiseProcessor1(), initialiseProcessor2()
#include <processor/initialise.h>
// Machine::initialise()
#include <machine/Machine.h>

Elf32 elf("kernel");

/// NOTE JamesM is doing some testing here.
#include <processor/Processor.h>
class Foo
{
public:
    Foo(){}
    ~Foo(){}
void mytestfunc(bool a, char b)
{
  //InterruptState state;
  //Debugger::instance().breakpoint();
  #ifdef X86_COMMON
    Processor::breakpoint();
  #else
    int c = 3/0;
#endif
}
};

extern "C" void woot()
{
  Foo foo;
  foo.mytestfunc(false, 'g');
#ifdef X86_COMMON
  Processor::breakpoint();
#endif
}

extern "C" void bar()
{
  woot();
#ifdef X86_COMMON
  Processor::breakpoint();
#endif
}

/// Kernel entry point.
extern "C" void _main(BootstrapStruct_t *bsInf)
{

  // Firstly call the constructors of all global objects.
  initialiseConstructors();

  // Create a BootstrapInfo object to parse bsInf.
  BootstrapInfo bootstrapInfo(bsInf);

  // Initialise the processor-specific interface
  initialiseProcessor1(*bsInf);

  // Initialise the machine-specific interface
  Machine &machine = Machine::instance();
  machine.initialise();

#if defined(DEBUGGER)
  Debugger::instance().initialise();
#endif

  // Initialise the processor-specific interface
  // Bootup of the other Application Processors and related tasks
  initialiseProcessor2();

  elf.load(&bootstrapInfo);

#if defined(DEBUGGER) && defined(DEBUGGER_RUN_AT_START)
  NOTICE("VBE info available? " << bootstrapInfo.hasVbeInfo());
  bar();
#endif

  #ifdef X86
    Processor::breakpoint();
  #endif

  Serial *s = machine.getSerial(0);
  s->write('p');
  s->write('o');
  s->write('o');
//   int a = 3/0;
#if defined(MIPS_COMMON) && defined(MIPS_MALTA_BONITO64)
//   InterruptState st;
//   Debugger::instance().breakpoint(st);
  return; // Go back to the YAMON prompt.
#endif

  for (;;)
  {
    #ifdef X86_COMMON
      Processor::setInterrupts(true);
    #endif
  }
  // Then get the BootstrapInfo object to convert its contents into
  // C++ classes.
//  g_pKernelMemoryMap = bsInf->getMemoryMap();  // Get the bios' memory map.
//  g_pProcessors      = bsInf->getProcessors(); // Parse the SMP table.
}
