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

#include "PhysicalMemoryManager.h"
#include "VirtualAddressSpace.h"
#include <processor/Processor.h>
#include <processor/MemoryRegion.h>
#include <LockGuard.h>
#include <Log.h>

ArmV7PhysicalMemoryManager ArmV7PhysicalMemoryManager::m_Instance;

PhysicalMemoryManager &PhysicalMemoryManager::instance()
{
    return ArmV7PhysicalMemoryManager::instance();
}

physical_uintptr_t ArmV7PhysicalMemoryManager::allocatePage()
{
    LockGuard<Spinlock> guard(m_Lock);

    /// \todo Cache compact if needed
    physical_uintptr_t ptr = m_PageStack.allocate(0);
    return ptr;
}
void ArmV7PhysicalMemoryManager::freePage(physical_uintptr_t page)
{
    LockGuard<Spinlock> guard(m_Lock);

    m_PageStack.free(page);
}
void ArmV7PhysicalMemoryManager::freePageUnlocked(physical_uintptr_t page)
{
    if(!m_Lock.acquired())
        FATAL("ArmV7PhysicalMemoryManager::freePageUnlocked called without an acquired lock");

    m_PageStack.free(page);
}
bool ArmV7PhysicalMemoryManager::allocateRegion(MemoryRegion &Region,
                                                    size_t cPages,
                                                    size_t pageConstraints,
                                                    size_t Flags,
                                                    physical_uintptr_t start)
{
    LockGuard<Spinlock> guard(m_RegionLock);

    // Allocate a specific physical memory region (always physically continuous)
    if (start != static_cast<physical_uintptr_t>(-1))
    {
        if ((pageConstraints & continuous) != continuous)
            panic("PhysicalMemoryManager::allocateRegion(): function misused");

        // Remove the memory from the range-lists (if desired/possible)
#ifdef ARM_BEAGLE // Beagleboard RAM locations
        if((start < 0x80000000) || (start >= 0x90000000))
        {
            if(!m_NonRAMRanges.allocateSpecific(start, cPages * getPageSize()))
            {
                if(!(pageConstraints & PhysicalMemoryManager::force))
                    return false;
            }
        }
        else
#endif
        {
            if(!m_PhysicalRanges.allocateSpecific(start, cPages * getPageSize()))
                return false;
        }

        // Allocate the virtual address space
        uintptr_t vAddress;

        if (m_VirtualMemoryRegions.allocate(cPages * getPageSize(), vAddress)
            == false)
        {
            WARNING("AllocateRegion: MemoryRegion allocation failed.");
            return false;
        }

        // Map the physical memory into the allocated space
        VirtualAddressSpace *virtualAddressSpace;
        if(vAddress > 0x40000000) // > 1 GB = kernel address space
            virtualAddressSpace = &VirtualAddressSpace::getKernelAddressSpace();
        else
            virtualAddressSpace = &Processor::information().getVirtualAddressSpace();
        for (size_t i = 0;i < cPages;i++)
            if (virtualAddressSpace->map(start + i * PhysicalMemoryManager::getPageSize(),
                                        reinterpret_cast<void*>(vAddress + i * PhysicalMemoryManager::getPageSize()),
                                        Flags)
                == false)
           { 
                m_VirtualMemoryRegions.free(vAddress, cPages * PhysicalMemoryManager::getPageSize());
                WARNING("AllocateRegion: VirtualAddressSpace::map failed.");
                return false;
            }

        // Set the memory-region's members
        Region.m_VirtualAddress = reinterpret_cast<void*>(vAddress);
        Region.m_PhysicalAddress = start;
        Region.m_Size = cPages * PhysicalMemoryManager::getPageSize();

        // Add to the list of memory-regions
        PhysicalMemoryManager::m_MemoryRegions.pushBack(&Region);
        return true;
    }
    else
    {
        // Allocate continuous memory if we need to
        bool bContinuous = false;
        physical_uintptr_t physAddr = 0;
        if(pageConstraints & PhysicalMemoryManager::continuous)
        {
            bContinuous = true;
            if(!m_PhysicalRanges.allocate(cPages * getPageSize(), physAddr))
                return false;
        }

        // Allocate the virtual address space
        uintptr_t vAddress;
        if (m_VirtualMemoryRegions.allocate(cPages * PhysicalMemoryManager::getPageSize(),
                                            vAddress)
            == false)
        {
            WARNING("AllocateRegion: MemoryRegion allocation failed.");
            return false;
        }

        uint32_t start = 0;
        VirtualAddressSpace *virtualAddressSpace;
        if(vAddress > 0x40000000) // > 1 GB = kernel address space
            virtualAddressSpace = &VirtualAddressSpace::getKernelAddressSpace();
        else
            virtualAddressSpace = &Processor::information().getVirtualAddressSpace();

        {
            // Map the physical memory into the allocated space
            for (size_t i = 0;i < cPages;i++)
            {
                physical_uintptr_t page = 0;
                if(bContinuous)
                    page = physAddr + (i * PhysicalMemoryManager::getPageSize());
                else
                    page = m_PageStack.allocate(pageConstraints);
                if (virtualAddressSpace->map(page,
                                            reinterpret_cast<void*>(vAddress + i * PhysicalMemoryManager::getPageSize()),
                                            Flags)
                    == false)
                {
                    WARNING("AllocateRegion: VirtualAddressSpace::map failed.");
                    return false;
                }
            }
        }

        // Set the memory-region's members
        Region.m_VirtualAddress = reinterpret_cast<void*>(vAddress);
        Region.m_PhysicalAddress = bContinuous ? physAddr : 0; // If any mapping is done non-continuously, use getMapping
        Region.m_Size = cPages * PhysicalMemoryManager::getPageSize();

        // Add to the list of memory-regions
        PhysicalMemoryManager::m_MemoryRegions.pushBack(&Region);
        return true;
    }
    return false;
}
void ArmV7PhysicalMemoryManager::unmapRegion(MemoryRegion *pRegion)
{
}

extern char __start, __end;
void ArmV7PhysicalMemoryManager::initialise(const BootstrapStruct_t &info)
{
    // Define beginning and end ranges of usable RAM
#ifdef ARM_BEAGLE
    physical_uintptr_t addr = 0;
    for(addr = reinterpret_cast<physical_uintptr_t>(&__end);
        addr < 0x87000000;
        addr += 0x1000)
    {
        m_PageStack.free(addr);
    }
    for(addr = 0x88000000;
        addr < 0x8F000000;
        addr += 0x1000)
    {
        m_PageStack.free(addr);
    }

    size_t kernelSize = reinterpret_cast<physical_uintptr_t>(&__end) - reinterpret_cast<physical_uintptr_t>(&__start);
    if(kernelSize % 4096)
    {
        kernelSize += 0x1000;
        kernelSize &= ~0xFFF;
    }

    m_PhysicalRanges.free(0x80000000 + kernelSize, 0xF000000);
    m_PhysicalRanges.allocateSpecific(0x80000000,
                                      reinterpret_cast<physical_uintptr_t>(&__end) - 0x80000000);
    m_PhysicalRanges.allocateSpecific(0x87000000, 0x1000000);
    
    m_NonRAMRanges.free(0, 0x80000000);
    m_NonRAMRanges.free(0x90000000, 0x60000000);
#endif

    // Initialise the range of virtual space for MemoryRegions
    m_VirtualMemoryRegions.free(reinterpret_cast<uintptr_t>(KERNEL_VIRTUAL_MEMORYREGION_ADDRESS),
                         KERNEL_VIRTUAL_MEMORYREGION_SIZE);
}

ArmV7PhysicalMemoryManager::ArmV7PhysicalMemoryManager() :
    m_PageStack(), m_PhysicalRanges(), m_VirtualMemoryRegions(),
    m_Lock(false, true), m_RegionLock(false, true)
{
}
ArmV7PhysicalMemoryManager::~ArmV7PhysicalMemoryManager()
{
}

physical_uintptr_t ArmV7PhysicalMemoryManager::PageStack::allocate(size_t constraints)
{
    // Ignore all constraints, none relevant
    physical_uintptr_t ret = 0;
    if((m_StackMax != m_StackSize) && m_StackSize)
    {
        m_StackSize -= sizeof(physical_uintptr_t);
        ret = *(reinterpret_cast<uint32_t*>(m_Stack) + m_StackSize / sizeof(physical_uintptr_t));
    }
    return ret;
}

void ArmV7PhysicalMemoryManager::PageStack::free(physical_uintptr_t physicalAddress)
{
    // Input verification (machine-specific)
#ifdef ARM_BEAGLE
    if(physicalAddress < 0x80000000)
        return;
    else if(physicalAddress >= 0x90000000)
        return;
#endif

    // No stack, no free.
    if(!m_Stack)
        return;

    // Expand the stack if we need to
    if(m_StackMax == m_StackSize)
    {
        ArmV7VirtualAddressSpace &AddressSpace = static_cast<ArmV7VirtualAddressSpace&>(VirtualAddressSpace::getKernelAddressSpace());
        if(!AddressSpace.map(physicalAddress, adjust_pointer(m_Stack, m_StackMax), VirtualAddressSpace::KernelMode | VirtualAddressSpace::Write))
            return;

        m_StackMax += getPageSize();
    }

    *(reinterpret_cast<physical_uintptr_t*>(m_Stack) + (m_StackSize / sizeof(physical_uintptr_t))) = physicalAddress;
    m_StackSize += sizeof(physical_uintptr_t);
}

ArmV7PhysicalMemoryManager::PageStack::PageStack()
{
    m_StackMax = 0;
    m_StackSize = 0;
    m_Stack = KERNEL_VIRTUAL_PAGESTACK_4GB;
}


