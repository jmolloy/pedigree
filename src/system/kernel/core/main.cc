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

// If we're being called standalone, we don't get passed any BootstrapInfo.
#include "BootstrapInfo.h"

#ifdef DEBUGGER
  #include <Debugger.h>
#endif

#include <Log.h>
#include <panic.h>
#include <utilities/StaticString.h>
#include "cppsupport.h"                   // initialiseConstructors()
#include <processor/Processor.h>          // Processor::initialise1(), Processor::initialise2()
#include <machine/Machine.h>              // Machine::initialise()
#include <KernelElf.h>                    // KernelElf::initialise()
#include <Version.h>
#include <LocalIO.h>
#include <SerialIO.h>
#include <DebuggerIO.h>
#include "BootIO.h"
#include <processor/PhysicalMemoryManager.h>
#ifdef THREADS
#include <process/initialiseMultitasking.h>
#include <process/Thread.h>
#include <processor/PhysicalMemoryManager.h>
#include <process/Scheduler.h>
#endif
#include <Archive.h>

#include <machine/Device.h>

#include <machine/openfirmware/Device.h>

#include <processor/KernelCoreSyscallManager.h>
#include <utilities/List.h>
#include <linker/SymbolTable.h>

BootIO bootIO;
Semaphore sema(1);
/** Kernel entry point for application processors (after processor/machine has been initialised
    on the particular processor */
void apMain()
{
  NOTICE("Processor #" << Processor::id() << " started.");

  Processor::setInterrupts(true);
  for (;;)
  {
//    sema.acquire();
//    NOTICE("Got Sem: " << Processor::id() << ", apMain()");
//    sema.release();

    Scheduler::instance().yield();
//        for (int i = 0; i < 10000000; i++);
//    NOTICE("Processor " << Processor::id() << ", alive!");
  }
}
Spinlock bleh(false);
int idle(void *)
{
  Processor::setInterrupts(true);
  for (;;)
  {
    //bleh.acquire();
    //sema.acquire();
   //NOTICE("Got Sem: " << Processor::id() << ", idle()");
    //sema.release();
    //bleh.release();
    Scheduler::instance().yield();
//    for (int i = 0; i < 100000000; i++);
//    NOTICE("Processor " << Processor::id() << ", alive!");
  }
}

int foo(void *p)
{
  HugeStaticString str;
  int j=0;
  for (;;)
  {
    for(int i = 0; i < 10000000; i++) ;
    str.clear();
    str += "b";
    bootIO.write(str, BootIO::White, BootIO::Green);
    j++;
  }
  return 0;
}

int bar(void *p)
{
  HugeStaticString str;
  for (;;)
  {
    for(int i = 0; i < 10000000; i++) ;
    str.clear();
    str += "c";
    bootIO.write(str, BootIO::White, BootIO::Red);
  }
  return 0;
}

/// Kernel entry point.
extern "C" void _main(BootstrapStruct_t &bsInf)
{
  // Firstly call the constructors of all global objects.
  initialiseConstructors();

  // Initialise the processor-specific interface
  Processor::initialise1(bsInf);

  // Initialise the Kernel Elf class
  if (KernelElf::instance().initialise(bsInf) == false)
    panic("KernelElf::initialise() failed");

  // Initialise the machine-specific interface
  Machine &machine = Machine::instance();
  machine.initialise();

#if defined(DEBUGGER)
  Debugger::instance().initialise();
#endif

#if !defined(ARM_COMMON)
  if (bsInf.isInitrdLoaded() == false)
    panic("Initrd module not loaded!");
#endif

  KernelCoreSyscallManager::instance().initialise();

  // Initialise the processor-specific interface
  // Bootup of the other Application Processors and related tasks
  Processor::initialise2();

#ifdef THREADS
  new Thread(Processor::information().getCurrentThread()->getParent(), &idle, 0, 0);
new Thread(Processor::information().getCurrentThread()->getParent(), &idle, 0, 0);
new Thread(Processor::information().getCurrentThread()->getParent(), &idle, 0, 0);
new Thread(Processor::information().getCurrentThread()->getParent(), &idle, 0, 0);
new Thread(Processor::information().getCurrentThread()->getParent(), &idle, 0, 0);
new Thread(Processor::information().getCurrentThread()->getParent(), &idle, 0, 0);
new Thread(Processor::information().getCurrentThread()->getParent(), &idle, 0, 0);
new Thread(Processor::information().getCurrentThread()->getParent(), &idle, 0, 0);
  Processor::setInterrupts(true);
#endif

  // Initialise the boot output.
  bootIO.initialise();
#if defined(X86_COMMON) || defined(PPC_COMMON)
  Machine::instance().initialiseDeviceTree();
#endif

  // Spew out a starting string.
  HugeStaticString str;
  str += "Pedigree - revision ";
  str += g_pBuildRevision;
  str += "\n=======================\n";
  bootIO.write(str, BootIO::White, BootIO::Black);

  str.clear();
  str += "Built at ";
  str += g_pBuildTime;
  str += " by ";
  str += g_pBuildUser;
  str += " on ";
  str += g_pBuildMachine;
  str += "\n";
  bootIO.write(str, BootIO::LightGrey, BootIO::Black);

  str.clear();
  str += "Build flags: ";
  str += g_pBuildFlags;
  str += "\n";
  bootIO.write(str, BootIO::LightGrey, BootIO::Black);
//   for(;;);
  // NOTE We have to do this before we call Processor::initialisationDone() otherwise the
  //      BootstrapStruct_t might already be unmapped
#if defined(X86_COMMON) || defined(PPC_COMMON)
  Archive initrd(bsInf.getInitrdAddress(), bsInf.getInitrdSize());

  size_t nFiles = initrd.getNumFiles();
  for (size_t i = 0; i < nFiles; i++)
  {
    uint32_t percentage = ((i+1)*100) / nFiles;
    str.clear();
    str += "\r                                        \rLoading modules: ";
    str.append(percentage, 10, 3, ' ');
    str += "% (";
    str += initrd.getFileName(i);
    str += ")";
    bootIO.write(str, BootIO::LightGrey, BootIO::Black);

    // Load file.
    KernelElf::instance().loadModule(reinterpret_cast<uint8_t*> (initrd.getFile(i)),
                                     initrd.getFileSize(i));
  }

#endif

  // The initialisation is done here, unmap/free the .init section and on x86/64 the identity
  // mapping of 0-4MB
  // NOTE: BootstrapStruct_t unusable after this point
#ifdef X86_COMMON
  Processor::initialisationDone();
#endif


#ifdef DEBUGGER_RUN_AT_START
  //Processor::breakpoint();
#endif

  // Try and create a mapping.
#ifdef MIPS_COMMON
  physical_uintptr_t stackBase;
  stackBase = PhysicalMemoryManager::instance().allocatePage();
  NOTICE("StackBase = " << Hex << stackBase);
  bool br = VirtualAddressSpace::getKernelAddressSpace().map(stackBase, (void*)(0xC1505000), 0);
  NOTICE("b :" << br);

  uintptr_t *leh = reinterpret_cast<uintptr_t*> ((stackBase|0xa0000000) + 0x4);
  *leh = 0x1234567;

 Processor::breakpoint();

  volatile uintptr_t *a = (uintptr_t*)0xC1505004;
  uintptr_t b = *a;
  NOTICE("b : " << b);
  b++;
 Processor::breakpoint();
#endif

  for (;;)
  {
    // Kernel idle thread.
    Processor::setInterrupts(true);
    Scheduler::instance().yield();

//    for(int i = 0; i < 10000000; i++) ;
//    str.clear();
//    str += "a";
//    bootIO.write(str, BootIO::White, BootIO::Blue);
  }
}
