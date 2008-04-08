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
#include <processor/Processor.h>
#include <processor/IoPortManager.h>
#include "gdt.h"
#include "SyscallManager.h"
#include "InterruptManager.h"
#include "VirtualAddressSpace.h"
#include "../x86_common/PhysicalMemoryManager.h"

// Multiprocessor headers
#if defined(MULTIPROCESSOR)
  #include "../x86_common/Multiprocessor.h"
#endif

void Processor::switchAddressSpace(const VirtualAddressSpace &AddressSpace)
{
  const X64VirtualAddressSpace &x64AddressSpace = static_cast<const X64VirtualAddressSpace&>(AddressSpace);

  // Get the current page directory
  uint64_t cr3;
  asm volatile ("mov %%cr3, %0" : "=r" (cr3));

  // Do we need to set a new page directory?
  if (cr3 != x64AddressSpace.m_PhysicalPML4)
  {
    // Set the new page directory
    asm volatile ("mov %0, %%cr3" :: "r" (x64AddressSpace.m_PhysicalPML4));
  }
}

void Processor::initialise1(const BootstrapStruct_t &Info)
{
  // Initialise this processor's interrupt handling
  X64InterruptManager::initialiseProcessor();

  // Initialise this processor's syscall handling
  X64SyscallManager::initialiseProcessor();

  // Initialise the physical memory-management
  X86CommonPhysicalMemoryManager &physicalMemoryManager = X86CommonPhysicalMemoryManager::instance();
  physicalMemoryManager.initialise(Info);

  // Initialise the I/O Manager
  IoPortManager &ioPortManager = IoPortManager::instance();
  ioPortManager.initialise(0, 0x10000);

  m_Initialised = 1;
}

void Processor::initialise2()
{
  size_t nProcessors = 1;

  #if defined(MULTIPROCESSOR)
    nProcessors = initialiseMultiprocessor();
  #endif

  // Initialise the GDT
  X64GdtManager::instance().initialise(nProcessors);
  X64GdtManager::initialiseProcessor();

  // TODO: Unmap the identity mapping of the first MBs

  m_Initialised = 2;
}

void Processor::identify(HugeStaticString &str)
{
  str = "Rarcaken!!";
}

