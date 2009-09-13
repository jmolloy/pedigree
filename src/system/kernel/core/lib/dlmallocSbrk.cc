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
#include <Log.h>
#include <processor/PhysicalMemoryManager.h>

#if defined(TRACK_PAGE_ALLOCATIONS)
  #include <commands/AllocationCommand.h>
#endif

void *dlmallocSbrk(ssize_t incr)
{
  // NOTE: incr is already page aligned

  //Spinlock lock;
  //lock.acquire();

  // Get the current address space
#ifdef KERNEL_NEEDS_ADDRESS_SPACE_SWITCH
  VirtualAddressSpace &VAddressSpace = Processor::information().getVirtualAddressSpace();
  if (Processor::m_Initialised == 2)
    Processor::switchAddressSpace(VirtualAddressSpace::getKernelAddressSpace());
#endif
#if defined(TRACK_PAGE_ALLOCATIONS)
  g_AllocationCommand.setMallocing(true);
#endif

  // Expand the heap
  void *pHeap = VirtualAddressSpace::getKernelAddressSpace().expandHeap(incr,
                                                                        VirtualAddressSpace::KernelMode | VirtualAddressSpace::Write);

#if defined(TRACK_PAGE_ALLOCATIONS)
  g_AllocationCommand.setMallocing(false);
#endif
#ifdef KERNEL_NEEDS_ADDRESS_SPACE_SWITCH  
  if (Processor::m_Initialised == 2)
    Processor::switchAddressSpace(VAddressSpace);
#endif

  //lock.release();

  return pHeap;
}
