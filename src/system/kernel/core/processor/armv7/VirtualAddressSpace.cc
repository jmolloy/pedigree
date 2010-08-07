/*
 * Copyright (c) 2010 Matthew Iselin, Heights College
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
#include <Log.h>
#include <utilities/utility.h>
#include <processor/Processor.h>
#include <processor/types.h>
#include <processor/PhysicalMemoryManager.h>
#include <LockGuard.h>
#include "VirtualAddressSpace.h"
#include <machine/Machine.h>

/** Array of free pages, used during the mapping algorithms in case a new page
    table needs to be mapped, which must be done without relinquishing the lock
    (which means we can't call the PMM!)

    There is one page per processor. */
physical_uintptr_t g_EscrowPages[256]; /// \todo MAX_PROCESSORS

ArmV7KernelVirtualAddressSpace ArmV7KernelVirtualAddressSpace::m_Instance;

VirtualAddressSpace &VirtualAddressSpace::getKernelAddressSpace()
{
  return ArmV7KernelVirtualAddressSpace::m_Instance;
}

VirtualAddressSpace *VirtualAddressSpace::create()
{
  return new ArmV7VirtualAddressSpace();
}

ArmV7VirtualAddressSpace::ArmV7VirtualAddressSpace() :
  VirtualAddressSpace(reinterpret_cast<void*> (0))
{
}

ArmV7VirtualAddressSpace::~ArmV7VirtualAddressSpace()
{
}

bool ArmV7VirtualAddressSpace::memIsInHeap(void *pMem)
{
    if(pMem < KERNEL_VIRTUAL_HEAP)
        return false;
    else if(pMem >= getEndOfHeap())
        return false;
    else
        return true;
}
void *ArmV7VirtualAddressSpace::getEndOfHeap()
{
    return reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(KERNEL_VIRTUAL_HEAP) + KERNEL_VIRTUAL_HEAP_SIZE);
}

uint32_t ArmV7VirtualAddressSpace::toFlags(size_t flags)
{
    uint32_t ret = 0; // No access.
    if(flags & KernelMode)
    {
        ret = 1; // Read/write in privileged mode, not usable in user mode
    }
    else
    {
        if(flags & Write)
            ret = 3; // Read/write all
        else
            ret = 2; // Read/write privileged, read-only user
    }
    return ret;
}

size_t ArmV7VirtualAddressSpace::fromFlags(uint32_t flags)
{
    switch(flags)
    {
        case 0:
            return 0; // Zero permissions
        case 1:
            return (Write | KernelMode);
        case 2:
            return 0; // Read-only by user mode... how to represent that?
        case 3:
            return Write; // Read/write all
        default:
            return 0;
    }
}

bool ArmV7VirtualAddressSpace::initialise()
{
  return true;
}

bool ArmV7VirtualAddressSpace::isAddressValid(void *virtualAddress)
{
    // No address is "invalid" in the sense that we're looking for here.
    return true;
}

bool ArmV7VirtualAddressSpace::map(physical_uintptr_t physicalAddress,
                                    void *virtualAddress,
                                    size_t flags)
{
    return doMap(physicalAddress, virtualAddress, flags);
}

void ArmV7VirtualAddressSpace::unmap(void *virtualAddress)
{
    return doUnmap(virtualAddress);
}

bool ArmV7VirtualAddressSpace::isMapped(void *virtualAddress)
{
    return doIsMapped(virtualAddress);
}

void ArmV7VirtualAddressSpace::getMapping(void *virtualAddress,
                                           physical_uintptr_t &physicalAddress,
                                           size_t &flags)
{
    doGetMapping(virtualAddress, physicalAddress, flags);
}

void ArmV7VirtualAddressSpace::setFlags(void *virtualAddress, size_t newFlags)
{
    return doSetFlags(virtualAddress, newFlags);
}

void *ArmV7VirtualAddressSpace::allocateStack()
{
  return doAllocateStack(USERSPACE_VIRTUAL_STACK_SIZE);
}

void *ArmV7VirtualAddressSpace::allocateStack(size_t stackSz)
{
  return doAllocateStack(stackSz);
}

bool ArmV7VirtualAddressSpace::doIsMapped(void *virtualAddress)
{
    uintptr_t addr = reinterpret_cast<uintptr_t>(virtualAddress);
    addr &= ~0xFFF;
    uint32_t pdir_offset = addr >> 20;
    uint32_t ptab_offset = (addr >> 12) & 0xFF;

    // Grab the entry in the page directory
    FirstLevelDescriptor *pdir = reinterpret_cast<FirstLevelDescriptor *>(m_VirtualPageDirectory);
    if(pdir[pdir_offset].descriptor.entry)
    {
        if(pdir[pdir_offset].descriptor.fault.type == 2)
            return true; // No second-level table walk

        // Knowing if a page is mapped is a global thing
        SecondLevelDescriptor *ptbl = reinterpret_cast<SecondLevelDescriptor *>(reinterpret_cast<uintptr_t>(m_VirtualPageTables) + (pdir_offset * 0x400));
        if(ptbl[ptab_offset].descriptor.fault.type)
            return true;
    }

    return false;
}

extern "C" void writeHex(unsigned int n);
bool ArmV7VirtualAddressSpace::doMap(physical_uintptr_t physicalAddress,
                                    void *virtualAddress,
                                    size_t flags)
{
    // Determine which range of page tables to use
    void *pageTables = 0;
    if(reinterpret_cast<uintptr_t>(virtualAddress) < 0x40000000)
        pageTables = USERSPACE_PAGETABLES;
    else
        pageTables = KERNEL_PAGETABLES;

    // Check if we have an allocated escrow page - if we don't, allocate it.
    // The kernel address space already has all page tables pre-allocated.
    if ((g_EscrowPages[Processor::id()] == 0) && (pageTables == USERSPACE_PAGETABLES))
    {
        g_EscrowPages[Processor::id()] = PhysicalMemoryManager::instance().allocatePage();
        if (g_EscrowPages[Processor::id()] == 0)
        {
            // Still 0, we have problems.
            FATAL("Out of memory");
        }
    }

    LockGuard<Spinlock> guard(m_Lock);

    // Grab offsets for the virtual address
    uintptr_t addr = reinterpret_cast<uintptr_t>(virtualAddress);
    uint32_t pdir_offset = addr >> 20;
    uint32_t ptbl_offset = (addr >> 12) & 0xFF;

    // Is there a page table for this page yet?
    FirstLevelDescriptor *pdir = reinterpret_cast<FirstLevelDescriptor *>(m_VirtualPageDirectory);

    if(!pdir[pdir_offset].descriptor.entry)
    {
        // There isn't - allocate one.
        uintptr_t page = g_EscrowPages[Processor::id()];
        g_EscrowPages[Processor::id()] = 0;

        // Point to the page, which defines a page table, is not shareable,
        // and is in domain 0. Note that there's 4 page tables in a 4K page,
        // so we handle that here.
        for(int i = 0; i < 4; i++)
        {
            /// \note For later: when creating high-memory page table mappings, they should be in
            ///       DOMAIN ONE - which holds all paging structures. This way they can be locked down
            ///       appropriately.
            pdir[pdir_offset + i].descriptor.entry = page + (i * 1024);
            pdir[pdir_offset + i].descriptor.pageTable.type = 1;
            pdir[pdir_offset + i].descriptor.pageTable.sbz1 = pdir[pdir_offset + i].descriptor.pageTable.sbz2 = 0;
            pdir[pdir_offset + i].descriptor.pageTable.ns = 1;
            pdir[pdir_offset + i].descriptor.pageTable.domain = 0; // DOMAIN0: Main memory
            pdir[pdir_offset + i].descriptor.pageTable.imp = 0;

            // Map in the page table we've just created so we can zero it.
            // mapaddr is the virtual address of the page table we just allocated
            // physical space for.
            uintptr_t mapaddr = reinterpret_cast<uintptr_t>(m_VirtualPageTables);
            mapaddr += ((pdir_offset + i) * 0x400);
            uint32_t ptbl_offset2 = (mapaddr >> 12) & 0xFF;

            // Grab the right page table for this new page
            uintptr_t ptbl_addr = reinterpret_cast<uintptr_t>(m_VirtualPageTables) + (mapaddr >> 20);
            SecondLevelDescriptor *ptbl = reinterpret_cast<SecondLevelDescriptor*>(ptbl_addr);
            ptbl[ptbl_offset2].descriptor.entry = page + (i * 1024);
            ptbl[ptbl_offset2].descriptor.smallpage.type = 2;
            ptbl[ptbl_offset2].descriptor.smallpage.b = 0;
            ptbl[ptbl_offset2].descriptor.smallpage.c = 0;
            ptbl[ptbl_offset2].descriptor.smallpage.ap1 = 3; // Page table, give it READ/WRITE
            ptbl[ptbl_offset2].descriptor.smallpage.sbz = 0;
            ptbl[ptbl_offset2].descriptor.smallpage.ap2 = 0;
            ptbl[ptbl_offset2].descriptor.smallpage.s = 0;
            ptbl[ptbl_offset2].descriptor.smallpage.nG = 1;

            // Mapped, so clear the page now
            memset(reinterpret_cast<void*>(mapaddr), 0, 1024);
        }
    }

    // Grab the virtual address for the page table for this page
    uintptr_t mapaddr = reinterpret_cast<uintptr_t>(pageTables);
    mapaddr += pdir_offset * 0x400;

    // Perform the mapping, if necessary
    SecondLevelDescriptor *ptbl = reinterpret_cast<SecondLevelDescriptor*>(mapaddr);
    if(ptbl[ptbl_offset].descriptor.entry & 0x3)
    {
        return false; // Already mapped.
    }
    else
    {
        ptbl[ptbl_offset].descriptor.entry = physicalAddress;
        ptbl[ptbl_offset].descriptor.smallpage.type = 2;
        ptbl[ptbl_offset].descriptor.smallpage.b = 0;
        ptbl[ptbl_offset].descriptor.smallpage.c = 0;
        ptbl[ptbl_offset].descriptor.smallpage.ap1 = toFlags(flags);
        ptbl[ptbl_offset].descriptor.smallpage.sbz = 0;
        ptbl[ptbl_offset].descriptor.smallpage.ap2 = 0;
        ptbl[ptbl_offset].descriptor.smallpage.s = 0;
        ptbl[ptbl_offset].descriptor.smallpage.nG = 1;
    }

    return true;
}

void ArmV7VirtualAddressSpace::doGetMapping(void *virtualAddress,
                                           physical_uintptr_t &physicalAddress,
                                           size_t &flags)
{
    uintptr_t addr = reinterpret_cast<uintptr_t>(virtualAddress);
    addr &= ~0xFFF;
    uint32_t pdir_offset = addr >> 20;
    uint32_t ptab_offset = (addr >> 12) & 0xFF;

    // Grab the entry in the page directory
    FirstLevelDescriptor *pdir = reinterpret_cast<FirstLevelDescriptor *>(m_VirtualPageDirectory);
    if(pdir[pdir_offset].descriptor.entry)
    {
        // What type is the entry?
        switch(pdir[pdir_offset].descriptor.fault.type)
        {
            case 1:
            {
                // Page table walk.
                SecondLevelDescriptor *ptbl = reinterpret_cast<SecondLevelDescriptor *>(reinterpret_cast<uintptr_t>(m_VirtualPageTables) + (pdir_offset * 0x400));
                if(!ptbl[ptab_offset].descriptor.fault.type)
                    return;
                else
                {
                    physicalAddress = ptbl[ptab_offset].descriptor.smallpage.base << 12;
                    flags = fromFlags(ptbl[ptab_offset].descriptor.smallpage.ap1);
                }
                break;
            }
            case 2:
            {
                // Section or supersection
                if(pdir[pdir_offset].descriptor.section.sectiontype == 0)
                {
                    uintptr_t offset = addr % 0x100000;
                    physicalAddress = (pdir[pdir_offset].descriptor.section.base << 20) + offset;
                    flags = fromFlags(pdir[pdir_offset].descriptor.section.ap1);
                }
                else if(pdir[pdir_offset].descriptor.section.sectiontype == 1)
                {
                    uintptr_t offset = addr % 0x1000000;
                    physicalAddress = (pdir[pdir_offset].descriptor.section.base << 20) + offset;
                    flags = fromFlags(pdir[pdir_offset].descriptor.section.ap1);
                }
                else
                    ERROR("doGetMapping: who knows what the hell this paging structure is");
                break;
            }
            default:
                return;
        }
    }
}

void ArmV7VirtualAddressSpace::doSetFlags(void *virtualAddress, size_t newFlags)
{
    uintptr_t addr = reinterpret_cast<uintptr_t>(virtualAddress);
    uint32_t pdir_offset = addr >> 20;
    uint32_t ptab_offset = (addr >> 12) & 0xFF;

    // Grab the entry in the page directory
    FirstLevelDescriptor *pdir = reinterpret_cast<FirstLevelDescriptor *>(m_VirtualPageDirectory);
    if(pdir[pdir_offset].descriptor.entry)
    {
        // What type is the entry?
        switch(pdir[pdir_offset].descriptor.fault.type)
        {
            case 1:
            {
                // Page table walk.
                SecondLevelDescriptor *ptbl = reinterpret_cast<SecondLevelDescriptor *>(reinterpret_cast<uintptr_t>(m_VirtualPageTables) + (pdir_offset * 0x400));
                ptbl[ptab_offset].descriptor.smallpage.ap1 = toFlags(newFlags);
                break;
            }
            case 2:
            {
                // Section or supersection
                if(pdir[pdir_offset].descriptor.section.sectiontype == 0)
                    pdir[pdir_offset].descriptor.section.ap1 = toFlags(newFlags);
                else if(pdir[pdir_offset].descriptor.section.sectiontype == 1)
                {
                    WARNING("doSetFlags: supersections not handled yet");
                }
                else
                    ERROR("doSetFlags: who knows what the hell this paging structure is");
                break;
            }
            default:
                return;
        }
    }
}

void ArmV7VirtualAddressSpace::doUnmap(void *virtualAddress)
{
    uintptr_t addr = reinterpret_cast<uintptr_t>(virtualAddress);
    uint32_t pdir_offset = addr >> 20;
    uint32_t ptab_offset = (addr >> 12) & 0xFF;

    // Grab the entry in the page directory
    FirstLevelDescriptor *pdir = reinterpret_cast<FirstLevelDescriptor *>(m_VirtualPageDirectory);
    if(pdir[pdir_offset].descriptor.entry)
    {
        // What type is the entry?
        switch(pdir[pdir_offset].descriptor.fault.type)
        {
            case 1:
            {
                // Page table walk.
                SecondLevelDescriptor *ptbl = reinterpret_cast<SecondLevelDescriptor *>(reinterpret_cast<uintptr_t>(m_VirtualPageTables) + (pdir_offset * 0x400));
                ptbl[ptab_offset].descriptor.fault.type = 0; // Unmap.
                break;
            }
            case 2:
                // Section or supersection
                pdir[pdir_offset].descriptor.fault.type = 0;
                break;
            default:
                return;
        }
    }
}

void *ArmV7VirtualAddressSpace::doAllocateStack(size_t sSize)
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

void ArmV7VirtualAddressSpace::freeStack(void *pStack)
{
    // Add the stack to the list
    m_freeStacks.pushBack(pStack);
}

extern char __start, __end;

bool ArmV7KernelVirtualAddressSpace::initialiseKernelAddressSpace()
{
    // The kernel address space is initialised by this function. We don't have
    // the MMU on yet, so we can modify the page directory to our heart's content.
    // We'll enable the MMU after the page directory is set up - only ever use
    // this function to construct the kernel address space, and only ever once.
    
    // Map in the 4 MB we'll use for page tables - this region is pinned in
    // PhysicalMemoryManager
    FirstLevelDescriptor *pdir = reinterpret_cast<FirstLevelDescriptor*>(m_PhysicalPageDirectory);
    memset(pdir, 0, 0x4000);

    uint32_t pdir_offset = 0, ptbl_offset = 0;
    uintptr_t vaddr = 0, paddr = 0;
    
    // Page table for mapping in the page directory. This table will cover the
    // last MB of the address space.
    physical_uintptr_t ptbl_paddr = 0x8FB00000 + (0x400000 - 0x400);
    memset(reinterpret_cast<void*>(0x8FB00000), 0, 0x400000);
    
    // Map in the page directory
    SecondLevelDescriptor *ptbl = reinterpret_cast<SecondLevelDescriptor*>(ptbl_paddr);
    vaddr = reinterpret_cast<uintptr_t>(m_VirtualPageDirectory);
    pdir_offset = vaddr >> 20;

    pdir[pdir_offset].descriptor.entry = ptbl_paddr; // Last page table in the 4K block.
    pdir[pdir_offset].descriptor.pageTable.type = 1;
    pdir[pdir_offset].descriptor.pageTable.sbz1 = pdir[pdir_offset].descriptor.pageTable.sbz2 = 0;
    pdir[pdir_offset].descriptor.pageTable.ns = 0; // Shareable
    pdir[pdir_offset].descriptor.pageTable.domain = 1; // Paging structures = DOMAIN1
    pdir[pdir_offset].descriptor.pageTable.imp = 0;
    for(int i = 0; i < 4; i++) // 4 pages in the page directory
    {
        ptbl_offset = ((vaddr + (i * 0x1000)) >> 12) & 0xFF;
        ptbl[ptbl_offset].descriptor.entry = m_PhysicalPageDirectory + (i * 0x1000);
        ptbl[ptbl_offset].descriptor.smallpage.type = 2;
        ptbl[ptbl_offset].descriptor.smallpage.b = 0;
        ptbl[ptbl_offset].descriptor.smallpage.c = 0;
        ptbl[ptbl_offset].descriptor.smallpage.ap1 = 3; /// \todo Correct flags
        ptbl[ptbl_offset].descriptor.smallpage.ap2 = 0;
        ptbl[ptbl_offset].descriptor.smallpage.sbz = 0;
        ptbl[ptbl_offset].descriptor.smallpage.s = 1; // Shareable
        ptbl[ptbl_offset].descriptor.smallpage.nG = 0; // Global, hint to MMU
    }
    
    // Identity-map the kernel
    size_t kernelSize = reinterpret_cast<uintptr_t>(&__end) - 0x80000000;
    for(size_t offset = 0; offset < kernelSize; offset += 0x100000)
    {
        uintptr_t baseAddr = 0x80000000 + offset;
        pdir_offset = baseAddr >> 20;
        
        // Map this block
        pdir[pdir_offset].descriptor.entry = baseAddr;
        pdir[pdir_offset].descriptor.section.type = 2;
        pdir[pdir_offset].descriptor.section.b = 0;
        pdir[pdir_offset].descriptor.section.c = 0;
        pdir[pdir_offset].descriptor.section.xn = 0;
        pdir[pdir_offset].descriptor.section.domain = 2; // Kernel = DOMAIN2
        pdir[pdir_offset].descriptor.section.imp = 0;
        pdir[pdir_offset].descriptor.section.ap1 = 3;
        pdir[pdir_offset].descriptor.section.ap2 = 0;
        pdir[pdir_offset].descriptor.section.tex = 0;
        pdir[pdir_offset].descriptor.section.s = 1;
        pdir[pdir_offset].descriptor.section.nG = 0;
        pdir[pdir_offset].descriptor.section.sectiontype = 0;
        pdir[pdir_offset].descriptor.section.ns = 0;
    }

    // Pre-allocate and define all the remaining kernel page tables.
    vaddr = reinterpret_cast<uintptr_t>(KERNEL_PAGETABLES);
    paddr = 0x8FB00000;
    for(size_t offset = 0; offset < 0x400000; offset += 0x100000)
    {
        uintptr_t baseAddr = vaddr + offset;
        pdir_offset = baseAddr >> 20;
        
        // Map this block
        pdir[pdir_offset].descriptor.entry = paddr + offset;
        pdir[pdir_offset].descriptor.section.type = 2;
        pdir[pdir_offset].descriptor.section.b = 0;
        pdir[pdir_offset].descriptor.section.c = 0;
        pdir[pdir_offset].descriptor.section.xn = 0;
        pdir[pdir_offset].descriptor.section.domain = 1; // Paging structures = DOMAIN1
        pdir[pdir_offset].descriptor.section.imp = 0;
        pdir[pdir_offset].descriptor.section.ap1 = 3;
        pdir[pdir_offset].descriptor.section.ap2 = 0;
        pdir[pdir_offset].descriptor.section.tex = 0;
        pdir[pdir_offset].descriptor.section.s = 1;
        pdir[pdir_offset].descriptor.section.nG = 0;
        pdir[pdir_offset].descriptor.section.sectiontype = 0;
        pdir[pdir_offset].descriptor.section.ns = 0;

        // Virtual address base of the region this 1 MB of page tables maps
        uintptr_t blockVBase = offset << 10;

        // 1024 page tables in this 1 MB region
        for(int i = 0; i < 1024; i++)
        {
            // First virtual address mapped by this page table
            uintptr_t firstVaddr = blockVBase + (i * 0x100000);

            // Physical address of the page table (as mapped above)
            uintptr_t ptbl_paddr = paddr + offset + (i * 0x400);

            // Do NOT overwrite existing mappings. That'll negate the above.
            pdir_offset = firstVaddr >> 20;
            if(pdir[pdir_offset].descriptor.entry)
                continue;
            pdir[pdir_offset].descriptor.entry = ptbl_paddr;
            pdir[pdir_offset].descriptor.pageTable.type = 1;
            pdir[pdir_offset].descriptor.pageTable.sbz1 = pdir[pdir_offset].descriptor.pageTable.sbz2 = 0;
            pdir[pdir_offset].descriptor.pageTable.ns = 0; // Shareable
            pdir[pdir_offset].descriptor.pageTable.domain = 1; // Paging structures = DOMAIN1
            pdir[pdir_offset].descriptor.pageTable.imp = 0;
        }
    }

    // Set up the required control registers before turning on the MMU
    Processor::writeTTBR0(0);
    Processor::writeTTBR1(m_PhysicalPageDirectory);
    Processor::writeTTBCR(2); // 0b010 = 4 KB TTBR0 directory, 1/3 GB split for user/kernel
    asm volatile("MCR p15,0,%0,c3,c0,0" : : "r" (0xFFFFFFFF)); // Manager access to all domains for now

    // Switch on the MMU
    uint32_t sctlr = 0;
    asm volatile("MRC p15,0,%0,c1,c0,0" : "=r" (sctlr));
    if(!(sctlr & 1))
        sctlr |= 1;
    asm volatile("MCR p15,0,%0,c1,c0,0" : : "r" (sctlr));

    return true;
}

ArmV7VirtualAddressSpace::ArmV7VirtualAddressSpace(void *Heap,
                                               physical_uintptr_t PhysicalPageDirectory,
                                               void *VirtualPageDirectory,
                                               void *VirtualPageTables,
                                               void *VirtualStack)
  : VirtualAddressSpace(Heap), m_PhysicalPageDirectory(PhysicalPageDirectory),
    m_VirtualPageDirectory(VirtualPageDirectory), m_VirtualPageTables(VirtualPageTables),
    m_pStackTop(VirtualStack), m_freeStacks(), m_Lock(false, true)
{
}

ArmV7KernelVirtualAddressSpace::ArmV7KernelVirtualAddressSpace() :
    ArmV7VirtualAddressSpace(KERNEL_VIRTUAL_HEAP,
                             0x8FAFC000,
                             KERNEL_PAGEDIR,
                             KERNEL_PAGETABLES,
                             KERNEL_VIRTUAL_STACK)
{
    for(int i = 0; i < 256; i++)
        g_EscrowPages[i] = 0;

    initialiseKernelAddressSpace();
}

ArmV7KernelVirtualAddressSpace::~ArmV7KernelVirtualAddressSpace()
{
}

bool ArmV7KernelVirtualAddressSpace::isMapped(void *virtualAddress)
{
  return doIsMapped(virtualAddress);
}
bool ArmV7KernelVirtualAddressSpace::map(physical_uintptr_t physicalAddress,
                                       void *virtualAddress,
                                       size_t flags)
{
  return doMap(physicalAddress, virtualAddress, flags);
}
void ArmV7KernelVirtualAddressSpace::getMapping(void *virtualAddress,
                                              physical_uintptr_t &physicalAddress,
                                              size_t &flags)
{
  doGetMapping(virtualAddress, physicalAddress, flags);
  if(!(flags & KernelMode))
      flags |= KernelMode;
}
void ArmV7KernelVirtualAddressSpace::setFlags(void *virtualAddress, size_t newFlags)
{
  doSetFlags(virtualAddress, newFlags);
}
void ArmV7KernelVirtualAddressSpace::unmap(void *virtualAddress)
{
  doUnmap(virtualAddress);
}
void *ArmV7KernelVirtualAddressSpace::allocateStack()
{
  void *pStack = doAllocateStack(KERNEL_STACK_SIZE + 0x1000);

  return pStack;
}
