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
#endif

#include <KernelElf.h>                    // KernelElf::initialise()
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

#include <machine/Device.h>

#ifdef OPENFIRMWARE
    #include <machine/openfirmware/Device.h>
#endif

#include <utilities/List.h>
#include <utilities/RadixTree.h>
#include <utilities/StaticString.h>

/** Output device for boot-time information. */
BootIO bootIO;

/** Kernel entry point for application processors (after processor/machine has been initialised
    on the particular processor */
void apMain()
{
  NOTICE("Processor #" << Processor::id() << " started.");

  Processor::setInterrupts(true);
  for (;;)
  {
    Scheduler::instance().yield();
  }
}

/** A processor idle function. */
int idle(void *)
{
  NOTICE("Idle " << Processor::information().getCurrentThread()->getId());
  Processor::setInterrupts(true);
  for (;;)
  {
    Scheduler::instance().yield();
  }
}

/** Kernel entry point. */
extern "C" void _main(BootstrapStruct_t &bsInf)
{
  // Firstly call the constructors of all global objects.
  initialiseConstructors();

  // Initialise the processor-specific interface
  Processor::initialise1(bsInf);

  // Initialise the machine-specific interface
  Machine &machine = Machine::instance();
  machine.initialise();

  // Initialise the Kernel Elf class
  if (KernelElf::instance().initialise(bsInf) == false)
    panic("KernelElf::initialise() failed");

#if defined(DEBUGGER)
  Debugger::instance().initialise();
#endif

  if (bsInf.isInitrdLoaded() == false)
    panic("Initrd module not loaded!");

  KernelCoreSyscallManager::instance().initialise();

  // Initialise the processor-specific interface
  // Bootup of the other Application Processors and related tasks
  Processor::initialise2();

  RadixTree<void*> tree;
  uintptr_t a = reinterpret_cast<uintptr_t>(tree.lookup(String("anus")));
  NOTICE("a: " << a);
  tree.insert(String("anus"), reinterpret_cast<void*>(67));
  uintptr_t b = reinterpret_cast<uintptr_t>(tree.lookup(String("anus")));
  NOTICE("b: " << b);
  tree.insert(String("anaii"), reinterpret_cast<void*>(34));
  uintptr_t c = reinterpret_cast<uintptr_t>(tree.lookup(String("anaii")));
  NOTICE("c: " << c);
  uintptr_t d = reinterpret_cast<uintptr_t>(tree.lookup(String("an")));
  NOTICE("d: " << d);
  uintptr_t e = reinterpret_cast<uintptr_t>(tree.lookup(String("anus")));
  NOTICE("e: " << e);


  for(;;);
  new Thread(Processor::information().getCurrentThread()->getParent(), &idle, 0, 0);
  Processor::setInterrupts(true);
  NOTICE("init bootio");
  // Initialise the boot output.
  bootIO.initialise();
  NOTICE("noninit bleh");
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

  /// \note We have to do this before we call Processor::initialisationDone() otherwise the
  ///       BootstrapStruct_t might already be unmapped
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
    NOTICE("Module load by thread " <<  Processor::information().getCurrentThread()->getId());
    KernelElf::instance().loadModule(reinterpret_cast<uint8_t*> (initrd.getFile(i)),
                                     initrd.getFileSize(i));
  }

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
  for (;;)
  {
    // Kernel idle thread.
    Processor::setInterrupts(true);
    Scheduler::instance().yield();
  }
}
