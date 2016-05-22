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

#if defined(MULTIPROCESSOR)

#include <processor/Processor.h>
#include "gdt.h"
#include "SyscallManager.h"
#include "InterruptManager.h"
#include <processor/NMFaultHandler.h>
#include <processor/x86_common/Multiprocessor.h>
#include <process/initialiseMultitasking.h>
#include <machine/mach_pc/Pc.h>

void Multiprocessor::applicationProcessorStartup()
{
  // Enable Write-Protect on the AP early so we don't have any problems with
  // the rest of the kernel.
  asm volatile("mov %%cr0, %%rax; or $0x10000, %%rax; mov %%rax, %%cr0" ::: "rax");

  // Initialise this processor's interrupt handling
  X64InterruptManager::initialiseProcessor();

  // Signal the Bootstrap processor that this processor is started and the BSP can continue
  // to boot up other processors
  m_ProcessorLock1.release();

  // Wait until the GDT is initialised and the first 4MB identity mapping removed
  m_ProcessorLock2.acquire(false);
  m_ProcessorLock2.release();

  // Initialise this processor's syscall handling
  X64SyscallManager::initialiseProcessor();

  // Load the GDT
  X64GdtManager::initialiseProcessor();

  // Configure floating point.
  NMFaultHandler::instance().initialiseProcessor();

  // Invalidate the first 4MB identity mapping
  Processor::invalidate(0);

  // Initialise the machine-specific interface
  Pc::instance().initialiseProcessor();

  // We need to synchronize the -init section invalidation
  Processor::invalidate(0);
  Processor::invalidate(reinterpret_cast<void*>(0x200000));

  // Start up the kernel process and idle thread for this processor.
  initialiseMultitaskingPerProcessor();

  // Call the per-processor code in main.cc
  apMain();
}

#endif
