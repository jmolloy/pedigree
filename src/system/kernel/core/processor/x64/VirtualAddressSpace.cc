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
#define PAGE_WRITE_THROUGH          0x08
#define PAGE_CACHE_DISABLE          0x10
#define PAGE_2MB                    0x80
#define PAGE_GLOBAL                 0x100
#define PAGE_SWAPPED                0x200
#define PAGE_COPY_ON_WRITE          0x400
#define PAGE_NX                     0x8000000000000000

//
// Macros
//
#define PML4_INDEX(x) ((reinterpret_cast<uintptr_t>(x) >> 39) & 0x1FF)
#define PAGE_DIRECTORY_POINTER_INDEX(x) ((reinterpret_cast<uintptr_t>(x) >> 30) & 0x1FF)
#define PAGE_DIRECTORY_INDEX(x) ((reinterpret_cast<uintptr_t>(x) >> 21) & 0x1FF)
#define PAGE_TABLE_INDEX(x) ((reinterpret_cast<uintptr_t>(x) >> 12) & 0x1FF)

#define TABLE_ENTRY(table, index) (&physicalAddress(reinterpret_cast<uint64_t*>(table))[index])

#define PAGE_GET_FLAGS(x) (*x & 0x8000000000000FFF)
#define PAGE_SET_FLAGS(x, f) *x = (*x & ~0x8000000000000FFF) | f
#define PAGE_GET_PHYSICAL_ADDRESS(x) (*x & ~0x8000000000000FFF)

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
  
  size_t Flags = toFlags(flags);
  size_t pml4Index = PML4_INDEX(virtualAddress);
  uint64_t *pml4Entry = TABLE_ENTRY(m_PhysicalPML4, pml4Index);

  // Check if a page directory pointer table was present *before* the conditional
  // allocation.
  bool pdWasPresent = (*pml4Entry & PAGE_PRESENT) != PAGE_PRESENT;

  // Is a page directory pointer table present?
  if (conditionalTableEntryAllocation(pml4Entry, Flags) == false)
    return false;

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
  if (conditionalTableEntryAllocation(pageDirectoryPointerEntry, Flags) == false)
    return false;

  size_t pageDirectoryIndex = PAGE_DIRECTORY_INDEX(virtualAddress);
  uint64_t *pageDirectoryEntry = TABLE_ENTRY(PAGE_GET_PHYSICAL_ADDRESS(pageDirectoryPointerEntry), pageDirectoryIndex);

  // Is a page table present?
  if (conditionalTableEntryAllocation(pageDirectoryEntry, Flags) == false)
    return false;

  size_t pageTableIndex = PAGE_TABLE_INDEX(virtualAddress);
  uint64_t *pageTableEntry = TABLE_ENTRY(PAGE_GET_PHYSICAL_ADDRESS(pageDirectoryEntry), pageTableIndex);

  // Is a page already present?
  if ((*pageTableEntry & PAGE_PRESENT) == PAGE_PRESENT)
    return false;

  // Flush the TLB (if we are marking a page as swapped-out)
  if ((Flags & PAGE_SWAPPED) == PAGE_SWAPPED)
    Processor::invalidate(virtualAddress);

  // Map the page
  *pageTableEntry = physAddress | Flags;

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
  flags = fromFlags(PAGE_GET_FLAGS(pageTableEntry));
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
  PAGE_SET_FLAGS(pageTableEntry, toFlags(newFlags));

  // TODO: Might need a TLB flush
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

  // Invalidate the TLB entry
  Processor::invalidate(virtualAddress);

  // Unmap the page
  *pageTableEntry = 0;
}

VirtualAddressSpace *X64VirtualAddressSpace::clone()
{
    // No lock guard in here - we assume that if we're cloning, nothing will be trying
    // to map/unmap memory.
    // Also, we need a way of solving this as we use the map/unmap functions, which
    // themselves try and grab the lock.
    /// \bug This assumption is false!

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

                for (uint64_t l = 0; l < 512; l++)
                {
                    uint64_t *ptEntry = TABLE_ENTRY(PAGE_GET_PHYSICAL_ADDRESS(pdEntry), l);
                    if ((*ptEntry & PAGE_PRESENT) != PAGE_PRESENT)
                        continue;
                  
                    uint64_t flags = PAGE_GET_FLAGS(ptEntry);

                    void *virtualAddress = reinterpret_cast<void*> ( ((i & 0x100)?(~0ULL << 48):0ULL) | /* Sign-extension. */
                                                                     (i << 39) |
                                                                     (j << 30) |
                                                                     (k << 21) |
                                                                     (l << 12) );
                  
                    if (getKernelAddressSpace().isMapped(virtualAddress))
                        continue;

                    // Page mapped in source address space, but not in kernel.
                    /// \todo Copy on write.
                    physical_uintptr_t newFrame = PhysicalMemoryManager::instance().allocatePage();

                    // Copy.
                    memcpy(reinterpret_cast<void*>(physicalAddress(newFrame)), reinterpret_cast<void*>(physicalAddress(PAGE_GET_PHYSICAL_ADDRESS(ptEntry))), 0x1000);
                  
                    // Map in.
                    pClone->map(newFrame, virtualAddress, fromFlags(flags));
                }
            }
        }
    }

    return pClone;
}

void X64VirtualAddressSpace::revertToKernelAddressSpace()
{
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

            bool canFreePdptEntry = false;
            for (uint64_t k = 0; k < 512; k++)
            {
                uint64_t *pdEntry = TABLE_ENTRY(PAGE_GET_PHYSICAL_ADDRESS(pdptEntry), k);
                if ((*pdEntry & PAGE_PRESENT) != PAGE_PRESENT)
                    continue;

                /// \todo Deal with 2MB pages here.

                bool bDidSkipPD = false;
                for (uint64_t l = 0; l < 512; l++)
                {
                    uint64_t *ptEntry = TABLE_ENTRY(PAGE_GET_PHYSICAL_ADDRESS(pdEntry), l);
                    if ((*ptEntry & PAGE_PRESENT) != PAGE_PRESENT)
                        continue;

                    void *virtualAddress = reinterpret_cast<void*> ( ((i & 0x100)?(~0ULL << 48):0ULL) | /* Sign-extension. */
                                                                     (i << 39) |
                                                                     (j << 30) |
                                                                     (k << 21) |
                                                                     (l << 12) );

                    // A little inefficient?
                    if((virtualAddress >= KERNEL_SPACE_START) || (getKernelAddressSpace().isMapped(virtualAddress)))
                    {
                        bDidSkipPD = true;
                        continue;
                    }
                    
                    NOTICE_NOLOCK("Blowing away " << reinterpret_cast<uintptr_t>(virtualAddress));

                    // Free the page.
                    /// \todo There's going to be a caveat with CoW here...
                    unmap(virtualAddress);
                    PhysicalMemoryManager::instance().freePage(PAGE_GET_PHYSICAL_ADDRESS(ptEntry));

                    *ptEntry = 0;
                }

                // If we skipped at least one page in the table, don't free
/*
                if(!bDidSkipPD)
                {
                    NOTICE_NOLOCK("freeing table");
                    
                    // We didn't skip any, so free the table
                    physical_uintptr_t phys = PAGE_GET_PHYSICAL_ADDRESS(pdEntry);
                    *pdEntry = 0;
                    PhysicalMemoryManager::instance().freePage(phys);
                    Processor::invalidate(pdEntry);
                }
*/
            }
        }
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
    return doAllocateStack(USERSPACE_VIRTUAL_STACK_SIZE);
}
void *X64VirtualAddressSpace::allocateStack(size_t stackSz)
{
    if(stackSz == 0)
        stackSz = USERSPACE_VIRTUAL_STACK_SIZE;
    return doAllocateStack(stackSz);
}
void *X64VirtualAddressSpace::doAllocateStack(size_t sSize)
{
  m_Lock.acquire();
  
  // Get a virtual address for the stack
  void *pStack = 0;
  if (m_freeStacks.count() != 0)
  {
    pStack = m_freeStacks.popBack();
    m_Lock.release();
  }
  else
  {
    pStack = m_pStackTop;
    m_pStackTop = adjust_pointer(m_pStackTop, -sSize);
    
    m_Lock.release();

    // Map it in
    uintptr_t stackBottom = reinterpret_cast<uintptr_t>(pStack) - sSize;
    for (size_t j = 0; j < sSize; j += PhysicalMemoryManager::getPageSize())
    {
        physical_uintptr_t phys = PhysicalMemoryManager::instance().allocatePage();
        bool b = map(phys,
                 reinterpret_cast<void*> (j + stackBottom),
                 VirtualAddressSpace::Write);
        if (!b)
            WARNING("map() failed in doAllocateStack");
    }
  }

  return pStack;
}
void X64VirtualAddressSpace::freeStack(void *pStack)
{
  // Add the stack to the list
  m_freeStacks.pushBack(pStack);
}

X64VirtualAddressSpace::~X64VirtualAddressSpace()
{
  PhysicalMemoryManager &physicalMemoryManager = PhysicalMemoryManager::instance();

  // Switch to the kernel's virtual address space
  Processor::switchAddressSpace(VirtualAddressSpace::getKernelAddressSpace());

  // TODO: Free other things, perhaps in VirtualAddressSpace
  //       We can't do this in VirtualAddressSpace destructor though!

  // Free the PageMapLevel4
  physicalMemoryManager.freePage(m_PhysicalPML4);
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
uint64_t X64VirtualAddressSpace::toFlags(size_t flags)
{
  uint64_t Flags = PAGE_PRESENT;
  if ((flags & KernelMode) == KernelMode)
    Flags |= PAGE_GLOBAL;
  else
    Flags |= PAGE_USER;
  if ((flags & Write) == Write)
    Flags |= PAGE_WRITE;
  if ((flags & WriteThrough) == WriteThrough)
    Flags |= PAGE_WRITE_THROUGH;
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
  return Flags;
}
size_t X64VirtualAddressSpace::fromFlags(uint64_t Flags)
{
  size_t flags = 0;
  if ((Flags & PAGE_USER) != PAGE_USER)
    flags |= KernelMode;
  if ((Flags & PAGE_WRITE) == PAGE_WRITE)
    flags |= Write;
  if ((Flags & PAGE_WRITE_THROUGH) == PAGE_WRITE_THROUGH)
    flags |= WriteThrough;
  if ((Flags & PAGE_CACHE_DISABLE) == PAGE_CACHE_DISABLE)
    flags |= CacheDisable;
  if ((Flags & PAGE_NX) != PAGE_NX)
    flags |= Execute;
  if ((Flags & PAGE_SWAPPED) == PAGE_SWAPPED)
    flags |= Swapped;
  if ((Flags & PAGE_COPY_ON_WRITE) == PAGE_COPY_ON_WRITE)
    flags |= CopyOnWrite;
  return flags;
}

bool X64VirtualAddressSpace::conditionalTableEntryAllocation(uint64_t *tableEntry, uint64_t flags)
{
  if ((*tableEntry & PAGE_PRESENT) != PAGE_PRESENT)
  {
    // Allocate a page
    PhysicalMemoryManager &PMemoryManager = PhysicalMemoryManager::instance();
    uint64_t page = PMemoryManager.allocatePage();
    if (page == 0)
      return false;

    // Map the page. Add the WRITE and USER flags so that these can be controlled
    // on a page-granularity level.
    *tableEntry = page | ((flags & ~(PAGE_GLOBAL | PAGE_NX | PAGE_SWAPPED | PAGE_COPY_ON_WRITE)) | PAGE_WRITE | PAGE_USER);

    // Zero the page directory pointer table
    memset(physicalAddress(reinterpret_cast<void*>(page)),
           0,
           PhysicalMemoryManager::getPageSize());
  }

  return true;
}

bool X64VirtualAddressSpace::conditionalTableEntryMapping(uint64_t *tableEntry,
                                                          uint64_t physAddress,
                                                          uint64_t flags)
{
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

  return false;
}
