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

  // Initialise the processor-specific interface
  Processor::initialise1(bsInf);

  // Initialise the machine-specific interface
  Machine &machine = Machine::instance();

#if defined(X86_COMMON) || defined(PPC_COMMON)
  Machine::instance().initialiseDeviceTree();
#endif

  machine.initialise();

#if defined(DEBUGGER)
  Debugger::instance().initialise();
#endif

  // Initialise the Kernel Elf class
  if (KernelElf::instance().initialise(bsInf) == false)
    panic("KernelElf::initialise() failed");

#ifdef ARM_COMMON
  NOTICE("ARM build now boots properly. Now hanging forever...");
  while(1);
#endif

  if (bsInf.isInitrdLoaded() == false)
    panic("Initrd module not loaded!");

  KernelCoreSyscallManager::instance().initialise();

  // Initialise the processor-specific interface
  // Bootup of the other Application Processors and related tasks
  Processor::initialise2();

  Processor::setInterrupts(true);

  // Initialise the kernel log.
  Log::instance().initialise();

  // Initialise the input manager
  InputManager::instance().initialise();

  // Initialise the boot output.
  bootIO.initialise();

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
    
    asm volatile ("hlt");
    
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
