/// The kernel startup file.

// If we're being called standalone, we don't get passed any BootstrapInfo.
#ifndef KERNEL_STANDALONE
#include "BootstrapInfo.h"
#else // !KERNEL_STANDALONE
struct BootstrapInfo;
#endif // !KERNEL_STANDALONE

#ifdef DEBUGGER
#include <Debugger.h>
#endif

// Global variables, declared in main.h
#ifdef DEBUGGER
Debugger debugger;
#endif

// initialiseConstructors()
#include <cppsupport.h>
// initialiseArchitecture1(), initialiseArchitecture2()
#include <initialiseMachine.h>
#include <types.h>

/// Kernel entry point.
extern "C" void _main(BootstrapInfo *bsInf)
{
  asm volatile("mov $0x12345, %ecx");
  
  /// Firstly call the constructors of all global objects.
  initialiseConstructors();

#if defined(DEBUGGER) && defined(DEBUGGER_RUN_AT_START)
  debugger.breakpoint(DEBUGGER_RUN_AT_START);
#endif

  /// First stage of the machine-dependant initialisation.
  /// After that only the memory-management related classes and
  /// functions can be used.
  initialiseMachine1();

  // We need a heap for dynamic memory allocation.
//  initialiseMemory();

  /// First stage of the machine-dependant initialisation.
  /// After that every machine dependant class & function can be used.
  initialiseMachine2();

  // Then get the BootstrapInfo object to convert its contents into
  // C++ classes.
//  g_pKernelMemoryMap = bsInf->getMemoryMap();  // Get the bios' memory map.
//  g_pProcessors      = bsInf->getProcessors(); // Parse the SMP table.
  
  
}
