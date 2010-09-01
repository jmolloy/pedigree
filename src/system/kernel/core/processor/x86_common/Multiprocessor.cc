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

#ifdef MULTIPROCESSOR

#include <Log.h>
#include <utilities/Vector.h>
#include "Multiprocessor.h"
#include "../../../machine/x86_common/Pc.h"

#if defined(X86)
  #include "../x86/VirtualAddressSpace.h"
#elif defined(X64)
  #include "../x64/VirtualAddressSpace.h"
#endif

#if defined(ACPI)
  #include "../../../machine/x86_common/Acpi.h"
#endif
#if defined(SMP)
  #include "../../../machine/x86_common/Smp.h"
#endif

#if !defined(APIC)
  #error APIC not defined
#endif
#if !defined(ACPI) && !defined(SMP)
  #error Neither ACPI nor SMP defined
#endif

Spinlock Multiprocessor::m_ProcessorLock1;
Spinlock Multiprocessor::m_ProcessorLock2(true);

size_t Multiprocessor::initialise1()
{
  // Did we find a processor list?
  bool bMPInfoFound = false;
  // List of information about each usable processor
  const Vector<ProcessorInformation*> *Processors;

  #if defined(ACPI)
    // Search through the ACPI tables
    Acpi &acpi = Acpi::instance();
    if ((bMPInfoFound = acpi.validProcessorInfo()) == true)
      Processors = &acpi.getProcessorList();
  #endif

  #if defined(SMP)
    // Search through the SMP tables
    Smp &smp = Smp::instance();
    if (bMPInfoFound == false &&
        (bMPInfoFound = smp.valid()) == true)
      Processors = &smp.getProcessorList();
  #endif

  // No processor list found
  if (bMPInfoFound == false)
  {
    NOTICE("Multiprocessor: couldn't find any information about multiple processors");
    return 1;
  }

  NOTICE("Multiprocessor: Found " << Dec << Processors->count() << Hex << " processors");

  // Copy the trampoline code to 0x7000
  /// \note This is a slightly hacky way to have the code linked directly to the
  ///       kernel - we hard-code specific offsets. Avoids the "relocation
  ///       truncated to fit" error from ld.
  extern void *trampoline16, *trampoline32, *trampolinegdt, *trampolinegdtr;
  extern void *trampoline16_end, *trampoline32_end, *trampolinegdt_end, *trampolinegdtr_end;
  memcpy(reinterpret_cast<void*>(0x7000),
         &trampoline16,
         reinterpret_cast<uintptr_t>(&trampoline16_end) - reinterpret_cast<uintptr_t>(&trampoline16));
  memcpy(reinterpret_cast<void*>(0x7100),
         &trampoline32,
         reinterpret_cast<uintptr_t>(&trampoline32_end) - reinterpret_cast<uintptr_t>(&trampoline32));
  memcpy(reinterpret_cast<void*>(0x7200),
         &trampolinegdtr,
         reinterpret_cast<uintptr_t>(&trampolinegdtr_end) - reinterpret_cast<uintptr_t>(&trampolinegdtr));
  memcpy(reinterpret_cast<void*>(0x7210),
         &trampolinegdt,
         reinterpret_cast<uintptr_t>(&trampolinegdt_end) - reinterpret_cast<uintptr_t>(&trampolinegdt));

  // Parameters for the trampoline code
  #if defined(X86)
    volatile uint32_t *trampolineStack = reinterpret_cast<volatile uint32_t*>(0x7FF8);
    volatile uint32_t *trampolineKernelEntry = reinterpret_cast<volatile uint32_t*>(0x7FF4);

    // Set the virtual address space
    *reinterpret_cast<volatile uint32_t*>(0x7FFC) = static_cast<X86VirtualAddressSpace&>(VirtualAddressSpace::getKernelAddressSpace()).m_PhysicalPageDirectory;
  #elif defined(X64)
    volatile uint64_t *trampolineStack = reinterpret_cast<volatile uint64_t*>(0x7FF0);
    volatile uint64_t *trampolineKernelEntry = reinterpret_cast<volatile uint64_t*>(0x7FE8);

    // Set the virtual address space
    *reinterpret_cast<volatile uint64_t*>(0x7FF8) = static_cast<X64VirtualAddressSpace&>(VirtualAddressSpace::getKernelAddressSpace()).m_PhysicalPML4;
  #endif

  // Set the entry point
  *trampolineKernelEntry = reinterpret_cast<uintptr_t>(&applicationProcessorStartup);

  LocalApic &localApic = Pc::instance().getLocalApic();
  VirtualAddressSpace &kernelSpace = VirtualAddressSpace::getKernelAddressSpace();
  // Startup the application processors through startup interprocessor interrupt
  for (size_t i = 0; i < Processors->count(); i++)
  {
    // Add a ProcessorInformation object
    ::ProcessorInformation *pProcessorInfo = 0;

    // Startup the processor
    if (localApic.getId() != (*Processors)[i]->apicId)
    {
      // AP: set up a proper information structure
      pProcessorInfo = new ::ProcessorInformation((*Processors)[i]->processorId,
                                                  (*Processors)[i]->apicId);
      Processor::m_ProcessorInformation.pushBack(pProcessorInfo);
    
      // Allocate kernel stack
      void *pStack = kernelSpace.allocateStack();

      // Set trampoline stack
      *trampolineStack = reinterpret_cast<uintptr_t>(pStack);
        
      NOTICE(" Booting processor #" << Dec << (*Processors)[i]->processorId << ", stack at 0x" << Hex << reinterpret_cast<uintptr_t>(pStack));

      /// \todo 10 ms delay between INIT IPI and Startup IPI, and we may need to
      ///       send the Startup IPI twice on some hardware.

      // Acquire the lock
      m_ProcessorLock1.acquire();
      
      localApic.interProcessorInterrupt((*Processors)[i]->apicId,
                                        0x07,
                                        LocalApic::deliveryModeInit,
                                        true,
                                        true);
      for(int z = 0; z < 0x10000; z++);
      
      // Send the Startup IPI to the processor
      localApic.interProcessorInterrupt((*Processors)[i]->apicId,
                                        0x07,
                                        LocalApic::deliveryModeStartup,
                                        true,
                                        false);

      // Wait until the processor is started and has unlocked the lock
      m_ProcessorLock1.acquire();
      m_ProcessorLock1.release();
    }
    else
    {
      NOTICE("Currently running on CPU #" << Dec << localApic.getId() << Hex << ", skipping boot (not necessary)");
      
      Processor::m_ProcessorInformation.pushBack(&Processor::m_SafeBspProcessorInformation);
      Processor::m_SafeBspProcessorInformation.setIds((*Processors)[i]->processorId,
                                                      (*Processors)[i]->apicId);
    }
  }

  return Processors->count();
}

void Multiprocessor::initialise2()
{
  m_ProcessorLock2.release();
}

#endif
