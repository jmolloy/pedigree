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

#include <panic.h>
#include <utilities/utility.h>
#include "utils.h"
#include "VirtualAddressSpace.h"
#include <processor/Processor.h>
#include <processor/PhysicalMemoryManager.h>
#include <process/Scheduler.h>
#include <process/Process.h>
#include <LockGuard.h>

//
// Page Table/Directory entry flags
//
#define PAGE_PRESENT                0x01
#define PAGE_WRITE                  0x02
#define PAGE_USER                   0x04
#define PAGE_WRITE_COMBINE          0x08
#define PAGE_CACHE_DISABLE          0x10
#define PAGE_ACCESSED               0x20
#define PAGE_DIRTY                  0x40
#define PAGE_2MB                    0x80
#define PAGE_PAT                    0x80
#define PAGE_GLOBAL                 0x100
#define PAGE_SWAPPED                0x200
#define PAGE_COPY_ON_WRITE          0x400
#define PAGE_SHARED                 0x800
#define PAGE_NX                     0x8000000000000000
#define PAGE_WRITE_THROUGH          (PAGE_PAT | PAGE_WRITE_COMBINE)

//
// Macros
//
#define PML4_INDEX(x) ((reinterpret_cast<uintptr_t>(x) >> 39) & 0x1FF)
#define PAGE_DIRECTORY_POINTER_INDEX(x) ((reinterpret_cast<uintptr_t>(x) >> 30) & 0x1FF)
#define PAGE_DIRECTORY_INDEX(x) ((reinterpret_cast<uintptr_t>(x) >> 21) & 0x1FF)
#define PAGE_TABLE_INDEX(x) ((reinterpret_cast<uintptr_t>(x) >> 12) & 0x1FF)

#define TABLE_ENTRY(table, index) (&physicalAddress(reinterpret_cast<uint64_t*>(table))[index])

#define PAGE_GET_FLAGS(x) (*x & 0x8000000000000FFFULL)
#define PAGE_SET_FLAGS(x, f) *x = (*x & ~0x8000000000000FFFULL) | f
#define PAGE_GET_PHYSICAL_ADDRESS(x) (*x & ~0x8000000000000FFFULL)

// Defined in boot-standalone.s
extern void *pml4;

X64VirtualAddressSpace X64VirtualAddressSpace::m_KernelSpace(KERNEL_VIRTUAL_HEAP,
                                                             reinterpret_cast<uintptr_t>(&pml4) - reinterpret_cast<uintptr_t>(KERNEL_VIRTUAL_ADDRESS),
                                                             KERNEL_VIRTUAL_STACK);

VirtualAddressSpace *g_pCurrentlyCloning = 0;

VirtualAddressSpace &VirtualAddressSpace::getKernelAddressSpace()
{
  return X64VirtualAddressSpace::m_KernelSpace;
}

VirtualAddressSpace *VirtualAddressSpace::create()
{
  return new X64VirtualAddressSpace();
}

bool X64VirtualAddressSpace::memIsInHeap(void *pMem)
{
    if(pMem < KERNEL_VIRTUAL_HEAP)
        return false;
    else if(pMem >= getEndOfHeap())
        return false;
    else
        return true;
}
void *X64VirtualAddressSpace::getEndOfHeap()
{
    return reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(KERNEL_VIRTUAL_HEAP) + KERNEL_VIRTUAL_HEAP_SIZE);
}


bool X64VirtualAddressSpace::isAddressValid(void *virtualAddress)
{
  if (reinterpret_cast<uint64_t>(virtualAddress) < 0x0008000000000000 ||
      reinterpret_cast<uint64_t>(virtualAddress) >= 0xFFF8000000000000)
    return true;
  return false;
}
bool X64VirtualAddressSpace::isMapped(void *virtualAddress)
{
  LockGuard<Spinlock> guard(m_Lock);
  
  size_t pml4Index = PML4_INDEX(virtualAddress);
  uint64_t *pml4Entry = TABLE_ENTRY(m_PhysicalPML4, pml4Index);

  // Is a page directory pointer table present?
  if ((*pml4Entry & PAGE_PRESENT) != PAGE_PRESENT)
    return false;

  size_t pageDirectoryPointerIndex = PAGE_DIRECTORY_POINTER_INDEX(virtualAddress);
  uint64_t *pageDirectoryPointerEntry = TABLE_ENTRY(PAGE_GET_PHYSICAL_ADDRESS(pml4Entry), pageDirectoryPointerIndex);

  // Is a page directory present?
  if ((*pageDirectoryPointerEntry & PAGE_PRESENT) != PAGE_PRESENT)
    return false;

  size_t pageDirectoryIndex = PAGE_DIRECTORY_INDEX(virtualAddress);
  uint64_t *pageDirectoryEntry = TABLE_ENTRY(PAGE_GET_PHYSICAL_ADDRESS(pageDirectoryPointerEntry), pageDirectoryIndex);

  // Is a page table or 2MB page present?
  if ((*pageDirectoryEntry & PAGE_PRESENT) != PAGE_PRESENT)
    return false;

  // Is it a 2MB page?
  if ((*pageDirectoryEntry & PAGE_2MB) == PAGE_2MB)
    return true;

  size_t pageTableIndex = PAGE_TABLE_INDEX(virtualAddress);
  uint64_t *pageTableEntry = TABLE_ENTRY(PAGE_GET_PHYSICAL_ADDRESS(pageDirectoryEntry), pageTableIndex);

  // Is a page present?
  return ((*pageTableEntry & PAGE_PRESENT) == PAGE_PRESENT);
}

bool X64VirtualAddressSpace::map(physical_uintptr_t physAddress,
                                 void *virtualAddress,
                                 size_t flags)
{
  LockGuard<Spinlock> guard(m_Lock);
  
  size_t Flags = toFlags(flags, true);
  size_t pml4Index = PML4_INDEX(virtualAddress);
  uint64_t *pml4Entry = TABLE_ENTRY(m_PhysicalPML4, pml4Index);

  // Check if a page directory pointer table was present *before* the conditional
  // allocation.
  bool pdWasPresent = (*pml4Entry & PAGE_PRESENT) != PAGE_PRESENT;

  // Is a page directory pointer table present?
  if (conditionalTableEntryAllocation(pml4Entry, flags) == false)
  {
    return false;
  }

  // If there wasn't a PDPT already present, and the address is in the kernel area
  // of memory, we need to propagate this change across all address spaces.
  if (!pdWasPresent &&
      Processor::m_Initialised == 2 &&
      virtualAddress >= KERNEL_VIRTUAL_HEAP)
  {
      uint64_t thisPml4Entry = *pml4Entry;
      for (size_t i = 0; i < Scheduler::instance().getNumProcesses(); i++)
      {
          Process *p = Scheduler::instance().getProcess(i);

          X64VirtualAddressSpace *x64VAS = reinterpret_cast<X64VirtualAddressSpace*> (p->getAddressSpace());
          uint64_t *pml4Entry = TABLE_ENTRY(x64VAS->m_PhysicalPML4, pml4Index);
          *pml4Entry = thisPml4Entry;
      }
  }

  size_t pageDirectoryPointerIndex = PAGE_DIRECTORY_POINTER_INDEX(virtualAddress);
  uint64_t *pageDirectoryPointerEntry = TABLE_ENTRY(PAGE_GET_PHYSICAL_ADDRESS(pml4Entry), pageDirectoryPointerIndex);

  // Is a page directory present?
  if (conditionalTableEntryAllocation(pageDirectoryPointerEntry, flags) == false)
  {
    return false;
  }

  size_t pageDirectoryIndex = PAGE_DIRECTORY_INDEX(virtualAddress);
  uint64_t *pageDirectoryEntry = TABLE_ENTRY(PAGE_GET_PHYSICAL_ADDRESS(pageDirectoryPointerEntry), pageDirectoryIndex);

  // Is a page table present?
  if (conditionalTableEntryAllocation(pageDirectoryEntry, flags) == false)
  {
    return false;
  }

  size_t pageTableIndex = PAGE_TABLE_INDEX(virtualAddress);
  uint64_t *pageTableEntry = TABLE_ENTRY(PAGE_GET_PHYSICAL_ADDRESS(pageDirectoryEntry), pageTableIndex);

  // Is a page already present?
  if ((*pageTableEntry & PAGE_PRESENT) == PAGE_PRESENT)
  {
    return false;
  }

  // Map the page
  *pageTableEntry = physAddress | Flags;

  // Flush the TLB
  Processor::invalidate(virtualAddress);

  return true;
}

void X64VirtualAddressSpace::getMapping(void *virtualAddress,
                                        physical_uintptr_t &physAddress,
                                        size_t &flags)
{
  // Get a pointer to the page-table entry (Also checks whether the page is actually present
  // or marked swapped out)
  uint64_t *pageTableEntry = 0;
  if (getPageTableEntry(virtualAddress, pageTableEntry) == false)
  {
    panic("VirtualAddressSpace::getMapping(): function misused");
    return;
  }

  // Extract the physical address and the flags
  physAddress = PAGE_GET_PHYSICAL_ADDRESS(pageTableEntry);
  flags = fromFlags(PAGE_GET_FLAGS(pageTableEntry), true);
}
void X64VirtualAddressSpace::setFlags(void *virtualAddress, size_t newFlags)
{
  LockGuard<Spinlock> guard(m_Lock);
  
  // Get a pointer to the page-table entry (Also checks whether the page is actually present
  // or marked swapped out)
  uint64_t *pageTableEntry = 0;
  if (getPageTableEntry(virtualAddress, pageTableEntry) == false)
  {
    panic("VirtualAddressSpace::setFlags(): function misused");
    return;
  }

  // Set the flags
  PAGE_SET_FLAGS(pageTableEntry, toFlags(newFlags, true));

  // Flush TLB - modified the mapping for this address.
  Processor::invalidate(virtualAddress);
}
void X64VirtualAddressSpace::unmap(void *virtualAddress)
{
  LockGuard<Spinlock> guard(m_Lock);
  
  // Get a pointer to the page-table entry (Also checks whether the page is actually present
  // or marked swapped out)
  uint64_t *pageTableEntry = 0;
  if (getPageTableEntry(virtualAddress, pageTableEntry) == false)
  {
    panic("VirtualAddressSpace::unmap(): function misused");
    return;
  }

  // Unmap the page
  *pageTableEntry = 0;

  // Invalidate the TLB entry
  Processor::invalidate(virtualAddress);

  // Possibly wipe out paging structures now that we've unmapped the page.
  // This can clear all the way up to, but not including, the PML4 - can be
  // extremely useful to conserve memory.
  maybeFreeTables(virtualAddress);
}

VirtualAddressSpace *X64VirtualAddressSpace::clone()
{
    LockGuard<Spinlock> guard(m_Lock);

    VirtualAddressSpace &thisAddressSpace = Processor::information().getVirtualAddressSpace();

    // Create a new virtual address space
    VirtualAddressSpace *pClone = VirtualAddressSpace::create();
    if (pClone == 0)
    {
        WARNING("X64VirtualAddressSpace: Clone() failed!");
        return 0;
    }

    // The userspace area is only the bottom half of the address space - the top 256 PML4 entries are for
    // the kernel, and these should be mapped anyway.
    for (uint64_t i = 0; i < 256; i++)
    {
        uint64_t *pml4Entry = TABLE_ENTRY(m_PhysicalPML4, i);
        if ((*pml4Entry & PAGE_PRESENT) != PAGE_PRESENT)
            continue;

        for (uint64_t j = 0; j < 512; j++)
        {
            uint64_t *pdptEntry = TABLE_ENTRY(PAGE_GET_PHYSICAL_ADDRESS(pml4Entry), j);
            if ((*pdptEntry & PAGE_PRESENT) != PAGE_PRESENT)
                continue;

            for (uint64_t k = 0; k < 512; k++)
            {
                uint64_t *pdEntry = TABLE_ENTRY(PAGE_GET_PHYSICAL_ADDRESS(pdptEntry), k);
                if ((*pdEntry & PAGE_PRESENT) != PAGE_PRESENT)
                    continue;

                /// \todo Deal with 2MB pages here.
                if ((*pdEntry & PAGE_2MB) == PAGE_2MB)
                    continue;

                for (uint64_t l = 0; l < 512; l++)
                {
                    uint64_t *ptEntry = TABLE_ENTRY(PAGE_GET_PHYSICAL_ADDRESS(pdEntry), l);
                    if ((*ptEntry & PAGE_PRESENT) != PAGE_PRESENT)
                        continue;
                  
                    uint64_t flags = PAGE_GET_FLAGS(ptEntry);
                    physical_uintptr_t physicalAddress = PAGE_GET_PHYSICAL_ADDRESS(ptEntry);

                    void *virtualAddress = reinterpret_cast<void*> ( ((i & 0x100)?(~0ULL << 48):0ULL) | /* Sign-extension. */
                                                                     (i << 39) |
                                                                     (j << 30) |
                                                                     (k << 21) |
                                                                     (l << 12) );

                    if(flags & PAGE_SHARED) {
                        // The physical address is now referenced (shared) in
                        // two address spaces, so make sure we hold another
                        // reference on it. Otherwise, if one of the two
                        // address spaces frees the page, the other may still
                        // refer to the bad page (and eventually double-free).
                        PhysicalMemoryManager::instance().pin(physicalAddress);

                        // Handle shared mappings - don't copy the original page.
                        pClone->map(physicalAddress, virtualAddress, fromFlags(flags, true));
                        continue;
                    }

                    // Map the new page in to the new address space for copy-on-write.
                    // This implies read-only (so we #PF for copy on write).
                    bool bWasCopyOnWrite = (flags & PAGE_COPY_ON_WRITE);
                    if(flags & PAGE_WRITE)
                      flags |= PAGE_COPY_ON_WRITE;
                    flags &= ~PAGE_WRITE;
                    pClone->map(physicalAddress, virtualAddress, fromFlags(flags, true));

                    // We need to modify the entry in *this* address space as well to
                    // also have the read-only and copy-on-write flag set, as otherwise
                    // writes in the parent process will cause the child process to see
                    // those changes immediately.
                    PAGE_SET_FLAGS(ptEntry, flags);
                    Processor::invalidate(virtualAddress);

                    // Pin the page twice - once for each side of the clone.
                    // But only pin for the parent if the parent page is not already
                    // copy on write. If we pin the CoW page, it'll be leaked when
                    // both parent and child terminate if the parent clone()s again.
                    if(!bWasCopyOnWrite)
                        PhysicalMemoryManager::instance().pin(physicalAddress);
                    PhysicalMemoryManager::instance().pin(physicalAddress);
                }
            }
        }
    }

    X64VirtualAddressSpace *pX64Clone = static_cast<X64VirtualAddressSpace *>(pClone);

    // Before returning the address space, bring across metadata.
    // Note though that if the parent of the clone (ie, this address space)
    // is the kernel address space, we mustn't copy metadata or else the
    // userspace defaults in the constructor get wiped out.

    if(m_pStackTop < KERNEL_SPACE_START)
    {
        pX64Clone->m_pStackTop = m_pStackTop;
        for(Vector<void*>::Iterator it = m_freeStacks.begin();
            it != m_freeStacks.end();
            ++it)
        {
            pX64Clone->m_freeStacks.pushBack(*it);
        }
    }

    if(m_Heap < KERNEL_SPACE_START)
    {
        pX64Clone->m_Heap = m_Heap;
        pX64Clone->m_HeapEnd = m_HeapEnd;
    }

    return pClone;
}

void X64VirtualAddressSpace::revertToKernelAddressSpace()
{
    LockGuard<Spinlock> guard(m_Lock);

    // The userspace area is only the bottom half of the address space - the top 256 PML4 entries are for
    // the kernel, and these should be mapped anyway.
    for (uint64_t i = 0; i < 256; i++)
    {
        uint64_t *pml4Entry = TABLE_ENTRY(m_PhysicalPML4, i);
        if ((*pml4Entry & PAGE_PRESENT) != PAGE_PRESENT)
            continue;

        for (uint64_t j = 0; j < 512; j++)
        {
            uint64_t *pdptEntry = TABLE_ENTRY(PAGE_GET_PHYSICAL_ADDRESS(pml4Entry), j);
            if ((*pdptEntry & PAGE_PRESENT) != PAGE_PRESENT)
                continue;

            for (uint64_t k = 0; k < 512; k++)
            {
                uint64_t *pdEntry = TABLE_ENTRY(PAGE_GET_PHYSICAL_ADDRESS(pdptEntry), k);
                if ((*pdEntry & PAGE_PRESENT) != PAGE_PRESENT)
                    continue;

                // Address this region begins at.
                void *regionVirtualAddress = reinterpret_cast<void*> ( ((i & 0x100)?(~0ULL << 48):0ULL) | /* Sign-extension. */
                                                                 (i << 39) |
                                                                 (j << 30) |
                                                                 (k << 21) );

                if(regionVirtualAddress < USERSPACE_VIRTUAL_START)
                    continue;
                if(regionVirtualAddress > KERNEL_SPACE_START)
                    break;

                /// \todo Deal with 2MB pages here.
                if ((*pdEntry & PAGE_2MB) == PAGE_2MB)
                    continue;

                for (uint64_t l = 0; l < 512; l++)
                {
                    uint64_t *ptEntry = TABLE_ENTRY(PAGE_GET_PHYSICAL_ADDRESS(pdEntry), l);
                    if ((*ptEntry & PAGE_PRESENT) != PAGE_PRESENT)
                        continue;

                    void *virtualAddress = reinterpret_cast<void *>(
                        reinterpret_cast<uintptr_t>(regionVirtualAddress) | (l << 12));

                    size_t flags = PAGE_GET_FLAGS(ptEntry);
                    physical_uintptr_t physicalAddress = PAGE_GET_PHYSICAL_ADDRESS(ptEntry);

                    // Release the physical memory if it is not shared with another
                    // process (eg, memory mapped file)
                    // Also avoid stumbling over a swapped out page.
                    /// \todo When swap system comes along, we want to remove this page
                    ///       from swap!
                    if((flags & (PAGE_SHARED | PAGE_SWAPPED)) == 0)
                    {
                        PhysicalMemoryManager::instance().freePage(physicalAddress);
                    }

                    // Free the page.
                    *ptEntry = 0;
                    Processor::invalidate(virtualAddress);
                }

                // Remove the table.
                PhysicalMemoryManager::instance().freePage(PAGE_GET_PHYSICAL_ADDRESS(pdEntry));
                *pdEntry = 0;
            }

            PhysicalMemoryManager::instance().freePage(PAGE_GET_PHYSICAL_ADDRESS(pdptEntry));
            *pdptEntry = 0;
        }

        PhysicalMemoryManager::instance().freePage(PAGE_GET_PHYSICAL_ADDRESS(pml4Entry));
        *pml4Entry = 0;
    }
}

bool X64VirtualAddressSpace::mapPageStructures(physical_uintptr_t physAddress,
                                               void *virtualAddress,
                                               size_t flags)
{
  LockGuard<Spinlock> guard(m_Lock);

  size_t Flags = toFlags(flags);
  size_t pml4Index = PML4_INDEX(virtualAddress);
  uint64_t *pml4Entry = TABLE_ENTRY(m_PhysicalPML4, pml4Index);

  // Is a page directory pointer table present?
  if (conditionalTableEntryMapping(pml4Entry, physAddress, Flags) == true)
    return true;

  size_t pageDirectoryPointerIndex = PAGE_DIRECTORY_POINTER_INDEX(virtualAddress);
  uint64_t *pageDirectoryPointerEntry = TABLE_ENTRY(PAGE_GET_PHYSICAL_ADDRESS(pml4Entry), pageDirectoryPointerIndex);

  // Is a page directory present?
  if (conditionalTableEntryMapping(pageDirectoryPointerEntry, physAddress, Flags) == true)
    return true;

  size_t pageDirectoryIndex = PAGE_DIRECTORY_INDEX(virtualAddress);
  uint64_t *pageDirectoryEntry = TABLE_ENTRY(PAGE_GET_PHYSICAL_ADDRESS(pageDirectoryPointerEntry), pageDirectoryIndex);

  // Is a page table present?
  if (conditionalTableEntryMapping(pageDirectoryEntry, physAddress, Flags) == true)
    return true;

  size_t pageTableIndex = PAGE_TABLE_INDEX(virtualAddress);
  uint64_t *pageTableEntry = TABLE_ENTRY(PAGE_GET_PHYSICAL_ADDRESS(pageDirectoryEntry), pageTableIndex);

  // Is a page already present?
  if ((*pageTableEntry & PAGE_PRESENT) != PAGE_PRESENT)
  {
    *pageTableEntry = physAddress | flags;
    return true;
  }
  return false;
}

bool X64VirtualAddressSpace::mapPageStructuresAbove4GB(physical_uintptr_t physAddress,
                                               void *virtualAddress,
                                               size_t flags)
{
  LockGuard<Spinlock> guard(m_Lock);
  
  size_t Flags = toFlags(flags);
  size_t pml4Index = PML4_INDEX(virtualAddress);
  uint64_t *pml4Entry = TABLE_ENTRY(m_PhysicalPML4, pml4Index);

  // Is a page directory pointer table present?
  if (conditionalTableEntryAllocation(pml4Entry, Flags) == true)
    return true;

  size_t pageDirectoryPointerIndex = PAGE_DIRECTORY_POINTER_INDEX(virtualAddress);
  uint64_t *pageDirectoryPointerEntry = TABLE_ENTRY(PAGE_GET_PHYSICAL_ADDRESS(pml4Entry), pageDirectoryPointerIndex);

  // Is a page directory present?
  if (conditionalTableEntryAllocation(pageDirectoryPointerEntry, Flags) == true)
    return true;

  size_t pageDirectoryIndex = PAGE_DIRECTORY_INDEX(virtualAddress);
  uint64_t *pageDirectoryEntry = TABLE_ENTRY(PAGE_GET_PHYSICAL_ADDRESS(pageDirectoryPointerEntry), pageDirectoryIndex);

  // Is a page table present?
  if (conditionalTableEntryAllocation(pageDirectoryEntry, Flags) == true)
    return true;

  size_t pageTableIndex = PAGE_TABLE_INDEX(virtualAddress);
  uint64_t *pageTableEntry = TABLE_ENTRY(PAGE_GET_PHYSICAL_ADDRESS(pageDirectoryEntry), pageTableIndex);

  // Is a page already present?
  if ((*pageTableEntry & PAGE_PRESENT) != PAGE_PRESENT)
  {
    *pageTableEntry = physAddress | flags;
    return true;
  }
  return false;
}

void *X64VirtualAddressSpace::allocateStack()
{
    size_t sz = USERSPACE_VIRTUAL_STACK_SIZE;
    if(this == &m_KernelSpace)
      sz = KERNEL_STACK_SIZE;
    return doAllocateStack(sz);
}

void *X64VirtualAddressSpace::allocateStack(size_t stackSz)
{
    if(stackSz == 0)
      return allocateStack();
    return doAllocateStack(stackSz);
}

void *X64VirtualAddressSpace::doAllocateStack(size_t sSize)
{
  size_t flags = 0;
  bool bMapAll = false;
  if(this == &m_KernelSpace)
  {
    // Don't demand map kernel mode stacks.
    flags = VirtualAddressSpace::KernelMode;
    bMapAll = true;
  }

  m_Lock.acquire();

  size_t pageSz = PhysicalMemoryManager::getPageSize();

  // Grab a new stack pointer. Use the list of freed stacks if we can, otherwise
  // adjust the internal stack pointer. Using the list of freed stacks helps
  // avoid having the virtual address creep downwards.
  void *pStack = 0;
  if (m_freeStacks.count() != 0)
  {
    pStack = m_freeStacks.popBack();
  }
  else
  {
    pStack = m_pStackTop;

    // Always leave one page unmapped between each stack to catch overflow.
    m_pStackTop = adjust_pointer(m_pStackTop, -(sSize + pageSz));
  }

  m_Lock.release();

  // Map the top of the stack in proper.
  uintptr_t firstPage = reinterpret_cast<uintptr_t>(pStack) - pageSz;
  physical_uintptr_t phys = PhysicalMemoryManager::instance().allocatePage();
  if(!bMapAll)
    PhysicalMemoryManager::instance().pin(phys);
  if(!map(phys, reinterpret_cast<void *>(firstPage), flags | VirtualAddressSpace::Write))
    WARNING("map() failed in doAllocateStack");

  // Bring in the rest of the stack as CoW.
  uintptr_t stackBottom = reinterpret_cast<uintptr_t>(pStack) - sSize;
  for (uintptr_t addr = stackBottom; addr < firstPage; addr += pageSz)
  {
    size_t map_flags = 0;

    if(!bMapAll)
    {
      // Copy first stack page on write.
      PhysicalMemoryManager::instance().pin(phys);
      map_flags = VirtualAddressSpace::CopyOnWrite;
    }
    else
    {
      phys = PhysicalMemoryManager::instance().allocatePage();
      map_flags = VirtualAddressSpace::Write;
    }

    if(!map(phys, reinterpret_cast<void *>(addr), flags | map_flags))
      WARNING("CoW map() failed in doAllocateStack");
  }

  return pStack;
}

void X64VirtualAddressSpace::freeStack(void *pStack)
{
  size_t pageSz = PhysicalMemoryManager::getPageSize();

  // Clean up the stack
  uintptr_t stackTop = reinterpret_cast<uintptr_t>(pStack);
  while(true)
  {
    stackTop -= pageSz;
    void *v = reinterpret_cast<void *>(stackTop);
    if(!isMapped(v))
      break; // Hit end of stack.

    size_t flags = 0;
    physical_uintptr_t phys = 0;
    getMapping(v, phys, flags);

    unmap(v);
    PhysicalMemoryManager::instance().freePage(phys);
  }

  // Add the stack to the list
  m_Lock.acquire();
  m_freeStacks.pushBack(pStack);
  m_Lock.release();
}

X64VirtualAddressSpace::~X64VirtualAddressSpace()
{
  PhysicalMemoryManager &physicalMemoryManager = PhysicalMemoryManager::instance();

  size_t freePages = physicalMemoryManager.freePageCount();

  // Drop back to the kernel address space. This will blow away the child's
  // mappings, but maintains shared pages as needed.
  revertToKernelAddressSpace();

  // Free the PageMapLevel4
  physicalMemoryManager.freePage(m_PhysicalPML4);

  size_t freePagesAfter = physicalMemoryManager.freePageCount();

  NOTICE("X64VirtualAddressSpace cleaned up " << (freePagesAfter - freePages) << " pages!");

  WARNING("X64VirtualAddressSpace::~X64VirtualAddressSpace doesn't clean up well.");
}

X64VirtualAddressSpace::X64VirtualAddressSpace()
  : VirtualAddressSpace(USERSPACE_VIRTUAL_HEAP), m_PhysicalPML4(0),
    m_pStackTop(USERSPACE_VIRTUAL_STACK), m_freeStacks(), m_bKernelSpace(false),
    m_Lock(false, true)
{

  // Allocate a new PageMapLevel4
  PhysicalMemoryManager &physicalMemoryManager = PhysicalMemoryManager::instance();
  m_PhysicalPML4 = physicalMemoryManager.allocatePage();

  // Initialise the page directory
  memset(reinterpret_cast<void*>(physicalAddress(m_PhysicalPML4)),
         0,
         0x800);

  // Copy the kernel PageMapLevel4
  memcpy(reinterpret_cast<void*>(physicalAddress(m_PhysicalPML4) + 0x800),
         reinterpret_cast<void*>(physicalAddress(m_KernelSpace.m_PhysicalPML4) + 0x800),
         0x800);
}

X64VirtualAddressSpace::X64VirtualAddressSpace(void *Heap, physical_uintptr_t PhysicalPML4, void *VirtualStack)
  : VirtualAddressSpace(Heap), m_PhysicalPML4(PhysicalPML4),
    m_pStackTop(VirtualStack), m_freeStacks(), m_bKernelSpace(true),
    m_Lock(false, true)
{
}

bool X64VirtualAddressSpace::getPageTableEntry(void *virtualAddress,
                                               uint64_t *&pageTableEntry)
{
  size_t pml4Index = PML4_INDEX(virtualAddress);
  uint64_t *pml4Entry = TABLE_ENTRY(m_PhysicalPML4, pml4Index);

  // Is a page directory pointer table present?
  if ((*pml4Entry & PAGE_PRESENT) != PAGE_PRESENT)
    return false;

  size_t pageDirectoryPointerIndex = PAGE_DIRECTORY_POINTER_INDEX(virtualAddress);
  uint64_t *pageDirectoryPointerEntry = TABLE_ENTRY(PAGE_GET_PHYSICAL_ADDRESS(pml4Entry), pageDirectoryPointerIndex);

  // Is a page directory present?
  if ((*pageDirectoryPointerEntry & PAGE_PRESENT) != PAGE_PRESENT)
    return false;

  size_t pageDirectoryIndex = PAGE_DIRECTORY_INDEX(virtualAddress);
  uint64_t *pageDirectoryEntry = TABLE_ENTRY(PAGE_GET_PHYSICAL_ADDRESS(pageDirectoryPointerEntry), pageDirectoryIndex);

  // Is a page table or 2MB page present?
  if ((*pageDirectoryEntry & PAGE_PRESENT) != PAGE_PRESENT)
    return false;
  if ((*pageDirectoryEntry & PAGE_2MB) == PAGE_2MB)
    return false;

  size_t pageTableIndex = PAGE_TABLE_INDEX(virtualAddress);
  pageTableEntry = TABLE_ENTRY(PAGE_GET_PHYSICAL_ADDRESS(pageDirectoryEntry), pageTableIndex);

  // Is a page present?
  if ((*pageTableEntry & PAGE_PRESENT) != PAGE_PRESENT &&
      (*pageTableEntry & PAGE_SWAPPED) != PAGE_SWAPPED)
    return false;

  return true;
}

void X64VirtualAddressSpace::maybeFreeTables(void *virtualAddress)
{
  bool bCanFreePageTable = true;

  uint64_t *pageDirectoryEntry = 0;

  size_t pml4Index = PML4_INDEX(virtualAddress);
  uint64_t *pml4Entry = TABLE_ENTRY(m_PhysicalPML4, pml4Index);

  // Is a page directory pointer table present?
  if ((*pml4Entry & PAGE_PRESENT) != PAGE_PRESENT)
    return;

  size_t pageDirectoryPointerIndex = PAGE_DIRECTORY_POINTER_INDEX(virtualAddress);
  uint64_t *pageDirectoryPointerEntry = TABLE_ENTRY(PAGE_GET_PHYSICAL_ADDRESS(pml4Entry), pageDirectoryPointerIndex);

  if ((*pageDirectoryPointerEntry & PAGE_PRESENT) == PAGE_PRESENT)
  {
    size_t pageDirectoryIndex = PAGE_DIRECTORY_INDEX(virtualAddress);
    pageDirectoryEntry = TABLE_ENTRY(PAGE_GET_PHYSICAL_ADDRESS(pageDirectoryPointerEntry), pageDirectoryIndex);

    if ((*pageDirectoryEntry & PAGE_PRESENT) == PAGE_PRESENT)
    {
      if ((*pageDirectoryEntry & PAGE_2MB) == PAGE_2MB)
      {
        bCanFreePageTable = false;
      }
      else
      {
        for (size_t i = 0; i < 0x200; ++i)
        {
          uint64_t *entry = TABLE_ENTRY(PAGE_GET_PHYSICAL_ADDRESS(pageDirectoryEntry), i);
          if ((*entry & PAGE_PRESENT) == PAGE_PRESENT ||
              (*entry & PAGE_SWAPPED) == PAGE_SWAPPED)
          {
            bCanFreePageTable = false;
            break;
          }
        }
      }
    }
  }

  if (bCanFreePageTable)
  {
    PhysicalMemoryManager::instance().freePage(PAGE_GET_PHYSICAL_ADDRESS(pageDirectoryEntry));
    *pageDirectoryEntry = 0;
  }
  else
    return;

  // Now that we've cleaned up the page table, we can scan the parent tables.

  bool bCanFreeDirectory = true;
  for (size_t i = 0; i < 0x200; ++i)
  {
    uint64_t *entry = TABLE_ENTRY(PAGE_GET_PHYSICAL_ADDRESS(pageDirectoryPointerEntry), i);
    if ((*entry & PAGE_PRESENT) == PAGE_PRESENT)
    {
      bCanFreeDirectory = false;
      break;
    }
  }

  if (bCanFreeDirectory)
  {
    PhysicalMemoryManager::instance().freePage(PAGE_GET_PHYSICAL_ADDRESS(pageDirectoryPointerEntry));
    *pageDirectoryPointerEntry = 0;
  }
  else
    return;

  bool bCanFreeDirectoryPointerTable = true;
  for (size_t i = 0; i < 0x200; ++i)
  {
    uint64_t *entry = TABLE_ENTRY(PAGE_GET_PHYSICAL_ADDRESS(pml4Entry), i);
    if ((*entry & PAGE_PRESENT) == PAGE_PRESENT)
    {
      bCanFreeDirectoryPointerTable = false;
      break;
    }
  }

  if (bCanFreeDirectoryPointerTable)
  {
    PhysicalMemoryManager::instance().freePage(PAGE_GET_PHYSICAL_ADDRESS(pml4Entry));
    *pml4Entry = 0;
  }
}

uint64_t X64VirtualAddressSpace::toFlags(size_t flags, bool bFinal)
{
  uint64_t Flags = 0;
  if ((flags & KernelMode) == KernelMode)
    Flags |= PAGE_GLOBAL;
  else
    Flags |= PAGE_USER;
  if ((flags & Write) == Write)
    Flags |= PAGE_WRITE;
  if ((flags & WriteCombine) == WriteCombine)
    Flags |= PAGE_WRITE_COMBINE;
  if ((flags & CacheDisable) == CacheDisable)
    Flags |= PAGE_CACHE_DISABLE;
  if ((flags & Execute) != Execute)
    Flags |= PAGE_NX;
  if ((flags & Swapped) == Swapped)
    Flags |= PAGE_SWAPPED;
  else
    Flags |= PAGE_PRESENT;
  if ((flags & CopyOnWrite) == CopyOnWrite)
    Flags |= PAGE_COPY_ON_WRITE;
  if ((flags & Shared) == Shared)
    Flags |= PAGE_SHARED;
  if (bFinal)
  {
    if ((flags & WriteThrough) == WriteThrough)
      Flags |= PAGE_WRITE_THROUGH;
    if ((flags & Accessed) == Accessed)
      Flags |= PAGE_ACCESSED;
    if ((flags & Dirty) == Dirty)
      Flags |= PAGE_DIRTY;
    if ((flags & ClearDirty) == ClearDirty)
      Flags &= ~PAGE_DIRTY;
  }
  return Flags;
}

size_t X64VirtualAddressSpace::fromFlags(uint64_t Flags, bool bFinal)
{
  size_t flags = 0;
  if ((Flags & PAGE_USER) != PAGE_USER)
    flags |= KernelMode;
  if ((Flags & PAGE_WRITE) == PAGE_WRITE)
    flags |= Write;
  if ((Flags & PAGE_WRITE_COMBINE) == PAGE_WRITE_COMBINE)
    flags |= WriteCombine;
  if ((Flags & PAGE_CACHE_DISABLE) == PAGE_CACHE_DISABLE)
    flags |= CacheDisable;
  if ((Flags & PAGE_NX) != PAGE_NX)
    flags |= Execute;
  if ((Flags & PAGE_SWAPPED) == PAGE_SWAPPED)
    flags |= Swapped;
  if ((Flags & PAGE_COPY_ON_WRITE) == PAGE_COPY_ON_WRITE)
    flags |= CopyOnWrite;
  if ((Flags & PAGE_SHARED) == PAGE_SHARED)
    flags |= Shared;
  if (bFinal)
  {
    if ((Flags & PAGE_WRITE_THROUGH) == PAGE_WRITE_THROUGH)
      flags |= WriteThrough;
    if ((Flags & PAGE_ACCESSED) == PAGE_ACCESSED)
      flags |= Accessed;
    if ((Flags & PAGE_DIRTY) == PAGE_DIRTY)
      flags |= Dirty;
  }
  return flags;
}

bool X64VirtualAddressSpace::conditionalTableEntryAllocation(uint64_t *tableEntry, uint64_t flags)
{
  // Convert VirtualAddressSpace::* flags to X64 flags.
  flags = toFlags(flags);

  if ((*tableEntry & PAGE_PRESENT) != PAGE_PRESENT)
  {
    // Allocate a page
    PhysicalMemoryManager &PMemoryManager = PhysicalMemoryManager::instance();
    uint64_t page = PMemoryManager.allocatePage();
    if (page == 0)
      return false;

    // Add the WRITE and USER flags so that these can be controlled
    // on a page-granularity level.
    flags &= ~(PAGE_GLOBAL | PAGE_NX | PAGE_SWAPPED | PAGE_COPY_ON_WRITE);
    flags |= PAGE_WRITE | PAGE_USER;

    // Map the page.
    *tableEntry = page | flags;

    // Zero the page directory pointer table
    memset(physicalAddress(reinterpret_cast<void*>(page)),
           0,
           PhysicalMemoryManager::getPageSize());
  }
  else if(((*tableEntry & PAGE_USER) != PAGE_USER) && (flags & PAGE_USER))
  {
    // Flags request user mapping, entry doesn't have that.
    *tableEntry |= PAGE_USER;
  }

  return true;
}

bool X64VirtualAddressSpace::conditionalTableEntryMapping(uint64_t *tableEntry,
                                                          uint64_t physAddress,
                                                          uint64_t flags)
{
  // Convert VirtualAddressSpace::* flags to X64 flags.
  flags = toFlags(flags, true);

  if ((*tableEntry & PAGE_PRESENT) != PAGE_PRESENT)
  {
    // Map the page. Add the WRITE and USER flags so that these can be controlled
    // on a page-granularity level.
    *tableEntry = physAddress | ((flags & ~(PAGE_GLOBAL | PAGE_NX | PAGE_SWAPPED | PAGE_COPY_ON_WRITE)) | PAGE_WRITE | PAGE_USER);

    // Zero the page directory pointer table
    memset(physicalAddress(reinterpret_cast<void*>(physAddress)),
           0,
           PhysicalMemoryManager::getPageSize());

    return true;
  }
  else if(((*tableEntry & PAGE_USER) != PAGE_USER) && (flags & PAGE_USER))
  {
    // Flags request user mapping, entry doesn't have that.
    *tableEntry |= PAGE_USER;
  }

  return false;
}
