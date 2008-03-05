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
// initialiseMachine()
#include <machine/initialise.h>

Elf32 elf("kernel");

/// NOTE bluecode is doing some testing here
#include <processor/interrupt.h>
#include <processor/syscall.h>
#include <utilities/List.h>

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
  //Debugger::instance().breakpoint();
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
  initialiseProcessor1();

  /// NOTE there we go
  MyInterruptHandler myHandler;
  MySyscallHandler mySysHandler;
  InterruptManager &IntManager = InterruptManager::instance();
  SyscallManager &SysManager = SyscallManager::instance();
  if (IntManager.registerInterruptHandler(254, &myHandler) == false)
    NOTICE("Failed to register interrupt handler");
  if (SysManager.registerSyscallHandler(SyscallManager::kernelCore, &mySysHandler) == false)
    NOTICE("Failed to register syscall handler");

  // Initialise the machine-specific interface
  initialiseMachine();

#if defined(DEBUGGER)
  Debugger::instance().initialise();
#endif

  // Initialise the processor-specific interface
  // Bootup of the other Application Processors and related tasks
  initialiseProcessor2();

  // We need a heap for dynamic memory allocation.
//  initialiseMemory();

  elf.load(&bootstrapInfo);

  /// NOTE: bluecode again
  asm volatile("int $0xFE"); // some interrupt
  asm volatile("int $0xFF" :: "a" ((SyscallManager::kernelCore << 16) | 0xFFFF)); // the syscall interrupt on x86

#if defined(DEBUGGER) && defined(DEBUGGER_RUN_AT_START)
  Foo foo;
  StaticString<32> str("RAR!");
  StaticString<32> str2("wee.");
  NOTICE("Str: " << Hex << reinterpret_cast<uintptr_t>(&str));
  NOTICE("Str2: " << Hex << reinterpret_cast<uintptr_t>(&str2));
  str += str2;
  str += 8;
  NOTICE((const char *)str);
  foo.mytestfunc(false, 'g');
#endif

  asm volatile ("int $3");

  List<int*> myList;
  List<int*>::ConstIterator cur = myList.begin();
  List<int*>::ConstIterator end = myList.end();
  for (;cur != end;++cur)
  {
    NOTICE("Hello, World");
  }

  for (;;){asm volatile("sti");}

  // Then get the BootstrapInfo object to convert its contents into
  // C++ classes.
//  g_pKernelMemoryMap = bsInf->getMemoryMap();  // Get the bios' memory map.
//  g_pProcessors      = bsInf->getProcessors(); // Parse the SMP table.
}
