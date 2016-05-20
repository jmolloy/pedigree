/*
 * Copyright (c) 2008-2014, Pedigree Developers
 *
 * Please see the CONTRIB file in the root of the source tree for a full
 * list of contributors.
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

#define PAT_UC      0x00
#define PAT_WC      0x01
#define PAT_WT      0x04
#define PAT_WP      0x05
#define PAT_WB      0x06
#define PAT_UCMINUS 0x07

union pat {
    struct {
        uint32_t pa0 : 3;
        uint32_t rsvd0 : 5;
        uint32_t pa1 : 3;
        uint32_t rsvd1 : 5;
        uint32_t pa2 : 3;
        uint32_t rsvd2 : 5;
        uint32_t pa3 : 3;
        uint32_t rsvd3 : 5;
        uint32_t pa4 : 3;
        uint32_t rsvd4 : 5;
        uint32_t pa5 : 3;
        uint32_t rsvd5 : 5;
        uint32_t pa6 : 3;
        uint32_t rsvd6 : 5;
        uint32_t pa7 : 3;
        uint32_t rsvd7 : 5;
    } s;
    uint64_t x;
};

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

  /// todo move to a better place
  // Write PAT MSR.
  // MSR 0x277

/*
PAT Entry
Memory Type Following Power-up or Reset
PAT0 WB
PAT1 WT
PAT2 UC-
PAT3 UC
PAT4 WB
PAT5 WT
PAT6 UC-
PAT7 UC
*/
  //
  uint32_t pat_lo, pat_hi;
  asm volatile("rdmsr" : "=a" (pat_lo), "=d" (pat_hi) : "c" (0x277));

  union pat p;
  p.x = pat_lo | (static_cast<uint64_t>(pat_hi) << 32ULL);
  p.s.pa0 = PAT_WB;
  p.s.pa1 = PAT_WC; // Redefine PWT in all page entries to mean WC instead of WT.
  p.s.pa2 = PAT_UCMINUS;
  p.s.pa3 = PAT_UC;
  p.s.pa4 = PAT_WB;
  p.s.pa5 = PAT_WT; // PWT|PAT == WT.
  p.s.pa6 = PAT_UCMINUS;
  p.s.pa7 = PAT_UC;
  pat_lo = static_cast<uint32_t>(p.x);
  pat_hi = static_cast<uint32_t>(p.x >> 32ULL);

  asm volatile("wrmsr" :: "a" (pat_lo), "d" (pat_hi), "c" (0x277));

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
  MemoryCopy(ident, &ebx, 4);
  MemoryCopy(&ident[4], &edx, 4);
  MemoryCopy(&ident[8], &ecx, 4);
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

