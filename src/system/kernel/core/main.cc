/*
 * Copyright (c) 2010 James Molloy, Jörg Pfähler, Matthew Iselin
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

#include "BootstrapInfo.h"

#ifdef DEBUGGER
    #include <Debugger.h>
    #include <LocksCommand.h>
#endif

#include <linker/KernelElf.h>             // KernelElf::initialise()
#include <Version.h>
#include <Log.h>
#include <panic.h>
#include <Archive.h>

#include "cppsupport.h"                   // initialiseConstructors()

#include <processor/Processor.h>          // Processor::initialise1(), Processor::initialise2()
#include <processor/PhysicalMemoryManager.h>
#include <processor/KernelCoreSyscallManager.h>

#include <machine/Machine.h>              // Machine::initialise()
#include <LocalIO.h>
#include <SerialIO.h>
#include <DebuggerIO.h>
#include "BootIO.h"

#include <process/initialiseMultitasking.h>
#include <process/Thread.h>
#include <process/Scheduler.h>
#include <process/SchedulingAlgorithm.h>

#include <machine/Device.h>

#ifdef OPENFIRMWARE
    #include <machine/openfirmware/Device.h>
#endif

#include <utilities/List.h>
#include <utilities/RadixTree.h>
#include <utilities/StaticString.h>

#include <machine/InputManager.h>

#ifdef THREADS
#include <utilities/ZombieQueue.h>
#endif

void apmm()
{
}
/** Output device for boot-time information. */
BootIO bootIO;

/** Global copy of the bootstrap information. */
BootstrapStruct_t *g_pBootstrapInfo;

/** Kernel entry point for application processors (after processor/machine has been initialised
    on the particular processor */
void apMain()
{
  NOTICE("Processor #" << Processor::id() << " started.");

  Processor::setInterrupts(true);
  for (;;)
  {
#ifdef THREADS
    Scheduler::instance().yield();
#endif
  }
}

/** A processor idle function. */
/*int idle(void *)
{
  NOTICE("Idle " << Processor::information().getCurrentThread()->getId());
  Processor::setInterrupts(true);
  for (;;)
  {
    Scheduler::instance().yield();
  }
  return 0;
}*/

/** Loads all kernel modules */
int loadModules(void *inf)
{
    BootstrapStruct_t bsInf = *static_cast<BootstrapStruct_t*>(inf);

    /// \note We have to do this before we call Processor::initialisationDone() otherwise the
    ///       BootstrapStruct_t might already be unmapped
    Archive initrd(bsInf.getInitrdAddress(), bsInf.getInitrdSize());

    size_t nFiles = initrd.getNumFiles();
    g_BootProgressTotal = nFiles*2; // Each file has to be preloaded and executed.
    for (size_t i = 0; i < nFiles; i++)
    {
        // Load file.
        KernelElf::instance().loadModule(reinterpret_cast<uint8_t*> (initrd.getFile(i)),
                                         initrd.getFileSize(i));
    }

    // The initialisation is done here, unmap/free the .init section and on x86/64 the identity
    // mapping of 0-4MB
    // NOTE: BootstrapStruct_t unusable after this point
    #ifdef X86_COMMON
        Processor::initialisationDone();
    #endif

    return 0;
}

/** Kernel entry point. */
extern "C" void _main(BootstrapStruct_t &bsInf)
{
  // Firstly call the constructors of all global objects.
  initialiseConstructors();

  g_pBootstrapInfo = &bsInf;

  // Initialise the kernel log.
  Log::instance().initialise();

  // Initialise the processor-specific interface
  Processor::initialise1(bsInf);

  // Initialise the machine-specific interface
  Machine &machine = Machine::instance();

#if defined(X86_COMMON) || defined(PPC_COMMON)
  Machine::instance().initialiseDeviceTree();
#endif

  machine.initialise();

  // Initialise the serial callback for the kernel log, now that the machine
  // abstraction is initialised.
  Log::instance().initialise2();

#if defined(DEBUGGER)
  Debugger::instance().initialise();
#endif

#ifndef ARM_BEAGLE
  // Initialise the Kernel Elf class
  if (KernelElf::instance().initialise(bsInf) == false)
    panic("KernelElf::initialise() failed");
#endif

#ifndef ARM_COMMON
  if (bsInf.isInitrdLoaded() == false)
    panic("Initrd module not loaded!");

  KernelCoreSyscallManager::instance().initialise();
#endif

  // Initialise the processor-specific interface
  // Bootup of the other Application Processors and related tasks
  Processor::initialise2(bsInf);

#ifdef THREADS
  ZombieQueue::instance().initialise();
#endif

  Processor::setInterrupts(true);

#ifndef ARM_COMMON // ARM isn't ready for InputManager
  // Initialise the input manager
  InputManager::instance().initialise();
#endif

  // Initialise the boot output.
  bootIO.initialise();

  // Spew out a starting string.
  HugeStaticString str;
  str += "Pedigree - revision ";
  str += g_pBuildRevision;
#ifndef DONT_LOG_TO_SERIAL
  str += "\r\n=======================\r\n";
#else
  str += "\n=======================\n";
#endif
  bootIO.write(str, BootIO::White, BootIO::Black);

  str.clear();
  str += "Built at ";
  str += g_pBuildTime;
  str += " by ";
  str += g_pBuildUser;
  str += " on ";
  str += g_pBuildMachine;
#ifndef DONT_LOG_TO_SERIAL
  str += "\r\n";
#else
  str += "\n";
#endif
  bootIO.write(str, BootIO::LightGrey, BootIO::Black);

  str.clear();
  str += "Build flags: ";
  str += g_pBuildFlags;
#ifndef DONT_LOG_TO_SERIAL
  str += "\r\n";
#else
  str += "\n";
#endif
  bootIO.write(str, BootIO::LightGrey, BootIO::Black);

  str.clear();
  str += "Processor information: ";
  Processor::identify(str);
#ifndef DONT_LOG_TO_SERIAL
  str += "\r\n";
#else
  str += "\n";
#endif
  bootIO.write(str, BootIO::LightGrey, BootIO::Black);

#ifdef ARM_COMMON
  asm volatile("swi $0x1");

#ifdef DEBUGGER_RUN_AT_START
  InterruptState state;
  LargeStaticString desc("Debugger running at startup");
  Debugger::instance().start(state, desc);
#else
  NOTICE_NOLOCK("ARM build now boots properly. Now hanging forever...");
  while(1)
  {
      asm volatile("wfi");
  }
#endif
#endif

#ifdef TRACK_LOCKS
  g_LocksCommand.setReady();
#endif

#ifdef THREADS
  new Thread(Processor::information().getCurrentThread()->getParent(), &loadModules, static_cast<void*>(&bsInf), 0);
#else
  loadModules(&bsInf);
#endif

#ifdef DEBUGGER_RUN_AT_START
  //Processor::breakpoint();
#endif

#ifdef THREADS
  Processor::information().getCurrentThread()->setPriority(MAX_PRIORITIES-1);
#endif

  // This will run when nothing else is available to run
  for (;;)
  {
    // Kernel idle thread.
    Processor::setInterrupts(true);
    
#if defined(X86_COMMON)
    asm volatile ("hlt");
#elif defined(ARM_COMMON)
    asm volatile ("wfi");
#else
    // No instruction to halt the processor until a given event.
#endif
    
#ifdef THREADS
    Scheduler::instance().yield();
#endif
  }
}

void system_reset()
{
    NOTICE("Resetting...");
    KernelElf::instance().unloadModules();
    Processor::reset();
}
