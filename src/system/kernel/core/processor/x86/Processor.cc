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

#include <processor/Processor.h>
#include <processor/IoPortManager.h>
#include "gdt.h"
#include "InterruptManager.h"
#include "VirtualAddressSpace.h"
#include "../x86_common/PhysicalMemoryManager.h"
#include <processor/PageFaultHandler.h>
#include <processor/NMFaultHandler.h>
#include <process/initialiseMultitasking.h>
#include <SlabAllocator.h>

// Multiprocessor headers
#if defined(MULTIPROCESSOR)
  #include "../x86_common/Multiprocessor.h"
#endif

void Processor::switchAddressSpace(VirtualAddressSpace &AddressSpace)
{
  const X86VirtualAddressSpace &x86AddressSpace = static_cast<const X86VirtualAddressSpace&>(AddressSpace);

  // Do we need to set a new page directory?
  if (readCr3() != x86AddressSpace.m_PhysicalPageDirectory)
  {
    // Set the new page directory
    asm volatile ("mov %0, %%cr3" :: "r" (x86AddressSpace.m_PhysicalPageDirectory));

    // Update the information in the ProcessorInformation structure
    ProcessorInformation &processorInformation = Processor::information();
    processorInformation.setVirtualAddressSpace(AddressSpace);
  }
}

physical_uintptr_t Processor::readCr3()
{
  physical_uintptr_t cr3;
  asm volatile ("mov %%cr3, %0" : "=r" (cr3));
  return cr3;
}

void Processor::initialise1(const BootstrapStruct_t &Info)
{
  // Initialise this processor's interrupt handling
  X86InterruptManager::initialiseProcessor();

  PageFaultHandler::instance().initialise();

  // Initialise the physical memory-management
  X86CommonPhysicalMemoryManager &physicalMemoryManager = X86CommonPhysicalMemoryManager::instance();
  physicalMemoryManager.initialise(Info);

  // Initialise the I/O Manager
  IoPortManager &ioPortManager = IoPortManager::instance();
  ioPortManager.initialise(0, 0x10000);

  NMFaultHandler::instance().initialise();

  m_Initialised = 1;
}

void Processor::initialise2(const BootstrapStruct_t &Info)
{
  m_nProcessors = 1;
  #if defined(MULTIPROCESSOR)
    m_nProcessors = Multiprocessor::initialise1();
  #endif

  // Initialise the GDT
  X86GdtManager::instance().initialise(m_nProcessors);
  X86GdtManager::initialiseProcessor();

  initialiseMultitasking();
  
  Processor::setInterrupts(false);
  
  m_Initialised = 2;
  
  #if defined(MULTIPROCESSOR)
    if (m_nProcessors != 1)
      Multiprocessor::initialise2();
  #endif
}

void Processor::identify(HugeStaticString &str)
{
  uint32_t eax, ebx, ecx, edx;
  char ident[13];
  cpuid(0, 0, eax, ebx, ecx, edx);
  memcpy(ident, &ebx, 4);
  memcpy(&ident[4], &edx, 4);
  memcpy(&ident[8], &ecx, 4);
  ident[12] = 0;
  str = ident;
}

void Processor::setTlsBase(uintptr_t newBase)
{
    X86GdtManager::instance().setTlsBase(newBase);
    
    // Reload FS/GS
    uint16_t newseg = newBase ? Processor::information().getTlsSelector() | 3 : 0x23;
    asm volatile("mov %0, %%bx; mov %%bx, %%fs; mov %%bx, %%gs" :: "r" (newseg) : "ebx");
}

