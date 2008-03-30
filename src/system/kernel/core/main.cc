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

/// NOTE bluecode is doing some testing here
#include <processor/InterruptManager.h>
#include <processor/SyscallManager.h>
#include <processor/Processor.h>

class MyInterruptHandler : public InterruptHandler
{
  public:
    virtual void interrupt(size_t interruptNumber, InterruptState &state);
};
class MySyscallHandler : public SyscallHandler
{
  public:
    virtual void syscall(SyscallState &state);
};

void MyInterruptHandler::interrupt(size_t interruptNumber, InterruptState &state)
{
  NOTICE("myInterruptHandler::interrupt(" << interruptNumber << ")");
}
void MySyscallHandler::syscall(SyscallState &state)
{
  NOTICE("mySyscallHandler::syscall(" << state.getSyscallNumber() << ")");
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

#ifdef X86
  /// NOTE there we go
  MyInterruptHandler myHandler;
  MySyscallHandler mySysHandler;
  InterruptManager &IntManager = InterruptManager::instance();
  SyscallManager &SysManager = SyscallManager::instance();
  if (IntManager.registerInterruptHandler(254, &myHandler) == false)
    NOTICE("Failed to register interrupt handler");
  if (SysManager.registerSyscallHandler(SyscallManager::kernelCore, &mySysHandler) == false)
    NOTICE("Failed to register syscall handler");
#endif

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

  /// NOTE: bluecode again
  #ifdef X86
    asm volatile("int $0xFE"); // some interrupt
    asm volatile("int $0xFF" :: "a" ((SyscallManager::kernelCore << 16) | 0xFFFF)); // the syscall interrupt on x86
  #endif

#if defined(DEBUGGER) && defined(DEBUGGER_RUN_AT_START)
  NOTICE("VBE info available? " << bootstrapInfo.hasVbeInfo());
#endif

  #ifdef X86
    Processor::breakpoint();
  #endif

  Serial *s = machine.getSerial(0);
  s->write('p');
  s->write('o');
  s->write('o');
#ifdef MIPS_COMMON
  InterruptState st;
  Debugger::instance().breakpoint(st);
  return; // Go back to the YAMON prompt.
#endif

  #if defined(X64)
    // TODO FIXME HACK
    asm volatile("hlt");
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
