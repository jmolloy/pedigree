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

#include <StaticString.h>

// initialiseConstructors()
#include "cppsupport.h"
// initialiseProcessor()
#include <processor/initialise.h>
// initialiseMachine1(), initialiseMachine2()
#include <machine/initialise.h>

Elf32 elf("kernel");

/// NOTE bluecode is doing some testing here
#include <processor/interrupt.h>
#include <processor/syscall.h>

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

/// NOTE JamesM is doing some testing here.
class Foo
{
public:
    Foo(){}
    ~Foo(){}
void mytestfunc(bool a, char b)
{
  //InterruptState state;
  //Debugger::instance().breakpoint(state);
  asm volatile("int $0x3");
}
};
/// Kernel entry point.
extern "C" void _main(BootstrapStruct_t *bsInf)
{
  // Firstly call the constructors of all global objects.
  initialiseConstructors();

  // Create a BootstrapInfo object to parse bsInf.
  BootstrapInfo bootstrapInfo(bsInf);

  // Initialise the processor-specific interface
  initialiseProcessor();

  /// NOTE there we go
  MyInterruptHandler myHandler;
  MySyscallHandler mySysHandler;
  InterruptManager &IntManager = InterruptManager::instance();
  SyscallManager &SysManager = SyscallManager::instance();
  if (IntManager.registerInterruptHandler(254, &myHandler) == false)
    NOTICE("Failed to register interrupt handler");
  if (SysManager.registerSyscallHandler(SyscallManager::kernelCore, &mySysHandler) == false)
    NOTICE("Failed to register syscall handler");

  // First stage of the machine-dependant initialisation.
  // After that only the memory-management related classes and
  // functions can be used.
  initialiseMachine1();

  // We need a heap for dynamic memory allocation.
//  initialiseMemory();

  // First stage of the machine-dependant initialisation.
  // After that every machine dependant class & function can be used.
  initialiseMachine2();

  elf.load(&bootstrapInfo);
  const char *addr = elf.lookupSymbol(0x100024);
  NOTICE("Addr: " << addr);

  /// NOTE: bluecode again
  asm volatile("int $0xFE"); // some interrupt
  asm volatile("int $0xFF" :: "a" ((SyscallManager::kernelCore << 16) | 0xFFFF)); // the syscall interrupt on x86

#if defined(DEBUGGER)
  Debugger::instance().initialise();
#endif
#if defined(DEBUGGER) && defined(DEBUGGER_RUN_AT_START)
  Foo foo;
  StaticString<32> str("RAR!");
  StaticString<32> str2("wee.");
  str += str2;
  NOTICE((const char *)str);
  //NOTICE("Foo: " << Hex << (int)&foo);
  foo.mytestfunc(false, 'g');
#endif

  // Then get the BootstrapInfo object to convert its contents into
  // C++ classes.
//  g_pKernelMemoryMap = bsInf->getMemoryMap();  // Get the bios' memory map.
//  g_pProcessors      = bsInf->getProcessors(); // Parse the SMP table.
}
