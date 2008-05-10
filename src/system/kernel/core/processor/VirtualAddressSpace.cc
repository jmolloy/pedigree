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

#include <utilities/utility.h>
#include <processor/VirtualAddressSpace.h>
#include <processor/PhysicalMemoryManager.h>

VirtualAddressSpace *g_pCurrentAddressSpace = 0;

VirtualAddressSpace &VirtualAddressSpace::getCurrentAddressSpace()
{
  if (g_pCurrentAddressSpace == 0)
    return VirtualAddressSpace::getKernelAddressSpace();
  else
    return *g_pCurrentAddressSpace;
}

void VirtualAddressSpace::setCurrentAddressSpace(VirtualAddressSpace *p)
{
  g_pCurrentAddressSpace = p;
}

void *VirtualAddressSpace::expandHeap(size_t pageCount, size_t flags)
{
  void *Heap = m_HeapEnd;
  PhysicalMemoryManager &PMemoryManager = PhysicalMemoryManager::instance();
  for (size_t i = 0;i < pageCount;i++)
  {
    // Allocate a page
    physical_uintptr_t page = PMemoryManager.allocatePage();

    if (page == 0)
    {
      // Reset the heap pointer
      m_HeapEnd = adjust_pointer(m_HeapEnd, - i * PhysicalMemoryManager::getPageSize());

      // Free the pages that were already allocated
      rollbackHeapExpansion(m_HeapEnd, i);
      return 0;
    }

    // Map the page
    if (map(page, m_HeapEnd, flags) == false)
    {
      // Free the page
      PMemoryManager.freePage(page);

      // Reset the heap pointer
      m_HeapEnd = adjust_pointer(m_HeapEnd, - i * PhysicalMemoryManager::getPageSize());

      //  Free the pages that were already allocated
      rollbackHeapExpansion(m_HeapEnd, i);
      return 0;
    }

    // Go to the next address
    m_HeapEnd = adjust_pointer(m_HeapEnd, PhysicalMemoryManager::getPageSize());
  }
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
