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
#include <utilities/StaticString.h>
#include "cppsupport.h"                   // initialiseConstructors()
#include <processor/Processor.h>          // Processor::initialise1(), Processor::initialise2()
#include <machine/Machine.h>              // Machine::initialise()
#include <KernelElf.h>                    // KernelElf::initialise()
#include <Version.h>
#include <LocalIO.h>
#include <SerialIO.h>
#include <DebuggerIO.h>

LocalIO *g_pLocalIO;
SerialIO *g_pSerialIO1;
SerialIO *g_pSerialIO2;

/// Initialises the boot output.
void initialiseBootOutput()
{
  g_pLocalIO->enableCli();
  g_pSerialIO1->enableCli();
#ifndef ARM_COMMON
  g_pSerialIO2->enableCli();
#endif
}

/// Allows output to any DebuggerIO terminal.
void bootOutput(HugeStaticString &str,
                DebuggerIO::Colour front=DebuggerIO::LightGrey,
                DebuggerIO::Colour back=DebuggerIO::Black)
{
  g_pLocalIO->writeCli(str, front, back);
  g_pSerialIO1->writeCli(str, front, back);
#ifndef ARM_COMMON
  g_pSerialIO2->writeCli(str, front, back);
#endif
}

/// Kernel entry point.
extern "C" void _main(BootstrapStruct_t *bsInf)
{
  // Firstly call the constructors of all global objects.
  initialiseConstructors();

  // Initialise the processor-specific interface
  Processor::initialise1(*bsInf);

  // Initialise the Kernel Elf class
  KernelElf::instance().initialise(*bsInf);

  // Initialise the machine-specific interface
  Machine &machine = Machine::instance();
  machine.initialise();

#if defined(DEBUGGER)
  Debugger::instance().initialise();
#endif

  // Initialise the processor-specific interface
  // Bootup of the other Application Processors and related tasks
  Processor::initialise2();

#if defined(ARM_COMMON)
   InterruptState st;
   LargeStaticString str2("I r cool");
   Debugger::instance().start(st, str2);
  return; // Go back to the YAMON prompt.
#endif

  // Initialise the boot output.
  LocalIO localIO(Machine::instance().getVga(0), Machine::instance().getKeyboard());
  g_pLocalIO = &localIO;
  SerialIO serialIO1(Machine::instance().getSerial(0));
  g_pSerialIO1 = &serialIO1;
#ifndef ARM_COMMON
  SerialIO serialIO2(Machine::instance().getSerial(1));
  g_pSerialIO2 = &serialIO2;
#endif
  initialiseBootOutput();

  // The initialisation is done here, unmap/free the .init section
  Processor::initialisationDone();

  // Spew out a starting string.
  HugeStaticString str;
  str += "Pedigree - revision ";
  str += g_pBuildRevision;
  str += "\n=======================\n";
  bootOutput(str, DebuggerIO::White, DebuggerIO::Black);

  str.clear();
  str += "Build at ";
  str += g_pBuildTime;
  str += " by ";
  str += g_pBuildUser;
  str += " on ";
  str += g_pBuildMachine;
  str += "\n";
  bootOutput(str);

  str.clear();
  str += "Build flags: ";
  str += g_pBuildFlags;
  str += "\n";
  bootOutput(str);

#ifdef DEBUGGER_RUN_AT_START
  Processor::breakpoint();
#endif
  
  for (;;)
  {
    #ifdef X86_COMMON
      Processor::setInterrupts(true);
    #endif
  }
}
