/// The kernel startup file.

#include "BootstrapInfo.h"

/// Calls the constructors for all global objects.
/// Call this before using any global objects.
void initialiseConstructors()
{
  // Constructor list is defined in the linker script.
  // The .ctors section is just an array of function pointers.
  // iterate through, calling each in turn.
  unsigned int *iterator = (unsigned int *)&start_ctors;
  while (iterator < (unsigned int*)&end_ctors)
  {
    void (*fp)(void) = (void (*)(void))*iterator;
    fp();
    iterator++;
  }
}

/// Required for G++ to compile code.
extern "C" void __cxa_atexit(void (*f)(void *), void *p, void *d)
{
}

/// Kernel entry point.
extern "C" void main(BootstrapInfo *bsInf)
{
  // Firstly call the constructors of all global objects.
  initialiseConstructors();
  
  // We need a heap for dynamic memory allocation.
  initialiseMemory();

  // Then get the BootstrapInfo object to convert its contents into
  // C++ classes.
  g_pKernelMemoryMap = bsInf->getMemoryMap();  // Get the bios' memory map.
  g_pProcessors      = bsInf->getProcessors(); // Parse the SMP table.
  
  
}
