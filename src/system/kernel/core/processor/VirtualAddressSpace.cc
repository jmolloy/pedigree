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

#include <utilities/utility.h>
#include <processor/VirtualAddressSpace.h>
#include <processor/PhysicalMemoryManager.h>
#include <processor/Processor.h>
#include <Log.h>

void *VirtualAddressSpace::expandHeap(ssize_t incr, size_t flags)
{
  void *Heap = m_HeapEnd;
  PhysicalMemoryManager &PMemoryManager = PhysicalMemoryManager::instance();

  void *newHeapEnd = adjust_pointer(m_HeapEnd, incr);
  
  m_HeapEnd = reinterpret_cast<void*> (reinterpret_cast<uintptr_t>(m_HeapEnd) & ~(PhysicalMemoryManager::getPageSize()-1));

  uintptr_t newEnd = reinterpret_cast<uintptr_t>(newHeapEnd);

  // Are we already at the end of the heap region?
  if(newEnd >= getKernelHeapStart())
  {
    // Kernel check - except SLAM doesn't use expandHeap.
    FATAL("expandHeap called for kernel heap!");
    return 0;
  }
  else if(getDynamicStart())
  {
    if(newEnd >= getDynamicStart())
    {
      // Heap is about to run over into the dynamic memory mapping region.
      // This is not allowed.
      ERROR("Heap expansion no longer allowed; about to run into dynamic memory area.");
      return 0;
    }
  }
  else if(newEnd >= getKernelStart())
  {
    // Nasty way of checking because by this point we'll have overrun the stack,
    // but the best we can do.
    ERROR("Heap expansion no longer allowed; have run over userspace stacks and about to run into kernel area.");
    return 0;
  }

  int i = 0;
  if(incr < 0)
  {
    while (reinterpret_cast<uintptr_t>(newHeapEnd) < reinterpret_cast<uintptr_t>(m_HeapEnd))
    {
        /// \note Should this be m_HeapEnd - getPageSize?
        void *unmapAddr = m_HeapEnd;
        if(isMapped(unmapAddr))
        {
            // Unmap the virtual address
            physical_uintptr_t phys = 0;
            size_t mappingFlags = 0;
            getMapping(unmapAddr, phys, mappingFlags);
            unmap(unmapAddr);

            // Free the physical page
            PMemoryManager.freePage(phys);
        }

        // Drop back a page
        m_HeapEnd = adjust_pointer(m_HeapEnd, -PhysicalMemoryManager::getPageSize());
        i++;
    }

    // Now that we've freed this section, the heap is actually at the end of the original memory...
    Heap = m_HeapEnd;
  }
  else
  {
      while (reinterpret_cast<uintptr_t>(newHeapEnd) > reinterpret_cast<uintptr_t>(m_HeapEnd))
      {
          // Allocate a page
          physical_uintptr_t page = PMemoryManager.allocatePage();

          if (page == 0)
          {
              ERROR("Out of physical memory!");
          
              // Reset the heap pointer
              m_HeapEnd = adjust_pointer(m_HeapEnd, - i * PhysicalMemoryManager::getPageSize());

              // Free the pages that were already allocated
              rollbackHeapExpansion(m_HeapEnd, i);
              return 0;
          }

          // Map the page
          if (map(page, m_HeapEnd, flags) == false)
          {
              // Map failed - probable double mapping. Go to the next page.
              // Free the page
              PMemoryManager.freePage(page);
          }
          else
          {
              // Empty the page.
              ByteSet(m_HeapEnd, 0, PhysicalMemoryManager::getPageSize());
          }

          // Go to the next address
          m_HeapEnd = adjust_pointer(m_HeapEnd, PhysicalMemoryManager::getPageSize());
          i++;
      }
  }

  m_HeapEnd = newHeapEnd;
  return Heap;
}

void VirtualAddressSpace::rollbackHeapExpansion(void *virtualAddress, size_t pageCount)
{
  for (size_t i = 0;i < pageCount;i++)
  {
    // Get the mapping for the current page
    size_t flags;
    physical_uintptr_t physicalAddress;
    getMapping(virtualAddress,
               physicalAddress,
               flags);

    // Free the physical page
    PhysicalMemoryManager::instance().freePage(physicalAddress);

    // Unmap the page from the virtual address space
    unmap(virtualAddress);

    // Go to the next virtual page
    virtualAddress = adjust_pointer(virtualAddress, PhysicalMemoryManager::getPageSize());
  }
}
