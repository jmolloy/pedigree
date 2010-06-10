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
#include <Log.h>
#include <utilities/utility.h>
#include <processor/Processor.h>
#include <processor/types.h>
#include <processor/PhysicalMemoryManager.h>
#include <LockGuard.h>
#include "VirtualAddressSpace.h"

ArmV7VirtualAddressSpace ArmV7VirtualAddressSpace::m_KernelSpace;

/** Array of free pages, used during the mapping algorithms in case a new page
    table needs to be mapped, which must be done without relinquishing the lock
    (which means we can't call the PMM!)

    There is one page per processor. */
physical_uintptr_t g_EscrowPages[256]; /// \todo MAX_PROCESSORS

VirtualAddressSpace &VirtualAddressSpace::getKernelAddressSpace()
{
  return ArmV7VirtualAddressSpace::m_KernelSpace;
}

VirtualAddressSpace *VirtualAddressSpace::create()
{
  // TODO
  //return new X86VirtualAddressSpace();
  return 0;
}

ArmV7VirtualAddressSpace::ArmV7VirtualAddressSpace() :
  VirtualAddressSpace(reinterpret_cast<void*> (0))
{
}

ArmV7VirtualAddressSpace::~ArmV7VirtualAddressSpace()
{
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

bool ArmV7VirtualAddressSpace::isMapped(void *virtualAddress)
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
                SecondLevelDescriptor *ptbl = reinterpret_cast<SecondLevelDescriptor *>(reinterpret_cast<uintptr_t>(USERSPACE_PAGETABLES) + pdir_offset);
                if(!ptbl[ptab_offset].descriptor.fault.type)
                    return false;
                break;
            }
            case 2:
                // Section or supersection
                WARNING("ArmV7VirtualAddressSpace::isAddressValid - sections and supersections not yet supported");
                break;
            default:
                return false;
        }
    }

    return true;
}

bool ArmV7VirtualAddressSpace::map(physical_uintptr_t physicalAddress,
                                    void *virtualAddress,
                                    size_t flags)
{
    // Check if we have an allocated escrow page - if we don't, allocate it.
    if (g_EscrowPages[Processor::id()] == 0)
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
            pdir[pdir_offset + i].descriptor.entry = page + (i * 1024);
            pdir[pdir_offset + i].descriptor.pageTable.type = 1;
            pdir[pdir_offset + i].descriptor.pageTable.sbz1 = pdir[pdir_offset + i].descriptor.pageTable.sbz2 = 0;
            pdir[pdir_offset + i].descriptor.pageTable.ns = 1;
            pdir[pdir_offset + i].descriptor.pageTable.domain = 0;
            pdir[pdir_offset + i].descriptor.pageTable.imp = 0;

            // Map in the page table we've just created so we can zero it.
            // mapaddr is the virtual address of the page table we just allocated
            // physical space for.
            uintptr_t mapaddr = reinterpret_cast<uintptr_t>(USERSPACE_PAGETABLES);
            mapaddr += ((pdir_offset + i) * 0x400);
            uint32_t ptbl_offset2 = (mapaddr >> 12) & 0xFF;

            // Grab the right page table for this new page
            uintptr_t ptbl_addr = reinterpret_cast<uintptr_t>(USERSPACE_PAGETABLES) + (mapaddr >> 20);
            SecondLevelDescriptor *ptbl = reinterpret_cast<SecondLevelDescriptor*>(ptbl_addr);
            ptbl[ptbl_offset2].descriptor.entry = page + (i * 1024);
            ptbl[ptbl_offset2].descriptor.smallpage.type = 2;
            ptbl[ptbl_offset2].descriptor.smallpage.b = 0;
            ptbl[ptbl_offset2].descriptor.smallpage.c = 0;
            ptbl[ptbl_offset2].descriptor.smallpage.ap1 = 3; /// \todo access flags!
            ptbl[ptbl_offset2].descriptor.smallpage.sbz = 0;
            ptbl[ptbl_offset2].descriptor.smallpage.ap2 = 0;
            ptbl[ptbl_offset2].descriptor.smallpage.s = 0;
            ptbl[ptbl_offset2].descriptor.smallpage.nG = 1;

            // Mapped, so clear the page now
            memset(reinterpret_cast<void*>(mapaddr), 0, 1024);
        }
    }
    
    // Grab the virtual address for the page table for this page
    uintptr_t mapaddr = reinterpret_cast<uintptr_t>(USERSPACE_PAGETABLES);
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
        ptbl[ptbl_offset].descriptor.smallpage.ap1 = 3; /// \todo access flags!
        ptbl[ptbl_offset].descriptor.smallpage.sbz = 0;
        ptbl[ptbl_offset].descriptor.smallpage.ap2 = 0;
        ptbl[ptbl_offset].descriptor.smallpage.s = 0;
        ptbl[ptbl_offset].descriptor.smallpage.nG = 1;
    }

    return true;
}

void ArmV7VirtualAddressSpace::getMapping(void *virtualAddress,
                                           physical_uintptr_t &physicalAddress,
                                           size_t &flags)
{
  physicalAddress = 0;
  flags = 0;
}

void ArmV7VirtualAddressSpace::setFlags(void *virtualAddress, size_t newFlags)
{
}

void ArmV7VirtualAddressSpace::unmap(void *virtualAddress)
{
}

void *ArmV7VirtualAddressSpace::allocateStack()
{
  // TODO
  return 0;
}
void ArmV7VirtualAddressSpace::freeStack(void *pStack)
{
  // TODO
}
