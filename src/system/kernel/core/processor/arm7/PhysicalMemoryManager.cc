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

#include "PhysicalMemoryManager.h"
#include "VirtualAddressSpace.h"
#include <processor/Processor.h>
#include <LockGuard.h>
#include <Log.h>

Arm7PhysicalMemoryManager Arm7PhysicalMemoryManager::m_Instance;

PhysicalMemoryManager &PhysicalMemoryManager::instance()
{
    return Arm7PhysicalMemoryManager::instance();
}

physical_uintptr_t Arm7PhysicalMemoryManager::allocatePage()
{
    LockGuard<Spinlock> guard(m_Lock);

    /// \todo Cache compact if needed
    physical_uintptr_t ptr = m_PageStack.allocate(0);
    return ptr;
}
void Arm7PhysicalMemoryManager::freePage(physical_uintptr_t page)
{
    LockGuard<Spinlock> guard(m_Lock);

    m_PageStack.free(page);
}
void Arm7PhysicalMemoryManager::freePageUnlocked(physical_uintptr_t page)
{
    if(!m_Lock.acquired())
        FATAL("Arm7PhysicalMemoryManager::freePageUnlocked called without an acquired lock");

    m_PageStack.free(page);
}
bool Arm7PhysicalMemoryManager::allocateRegion(MemoryRegion &Region,
                                                    size_t cPages,
                                                    size_t pageConstraints,
                                                    size_t Flags,
                                                    physical_uintptr_t start)
{
    /// \todo Write.
    return false;
}
void Arm7PhysicalMemoryManager::unmapRegion(MemoryRegion *pRegion)
{
}

extern char __start, __end;
void Arm7PhysicalMemoryManager::initialise(const BootstrapStruct_t &info)
{
    // Define beginning and end ranges of usable RAM
#ifdef ARM_BEAGLE
    m_PhysicalRanges.free(0x80000000, 0x10000000);
    /// \todo virtual memory, so we can dynamically expand this stack
    for(physical_uintptr_t addr = 0x80000000; addr < /*0x90000000*/ 0x80001000; addr += 0x1000)
    {
        // Ignore any address within the kernel
        if(!(addr > reinterpret_cast<physical_uintptr_t>(&__start) &&
             addr < reinterpret_cast<physical_uintptr_t>(&__end)))
            m_PageStack.free(addr);
    }
#endif

    /// \todo MemoryRegion virtual address (m_VirtualMemoryRegions)
    /// \todo Virtual memory for the above.
}

Arm7PhysicalMemoryManager::Arm7PhysicalMemoryManager()
{
}
Arm7PhysicalMemoryManager::~Arm7PhysicalMemoryManager()
{
}

physical_uintptr_t Arm7PhysicalMemoryManager::PageStack::allocate(size_t constraints)
{
    // Ignore all constraints, none relevant
    physical_uintptr_t ret = 0;
    if((m_StackMax != m_StackSize) && m_StackSize)
    {
        m_StackSize -= sizeof(physical_uintptr_t);
        ret = m_Stack[m_StackSize / sizeof(physical_uintptr_t)];
    }
    return ret;
}

void Arm7PhysicalMemoryManager::PageStack::free(physical_uintptr_t physicalAddress)
{
    // Input verification (machine-specific)
#ifdef ARM_BEAGLE
    if(physicalAddress < 0x80000000)
        return;
    else if(physicalAddress >= 0x90000000)
        return;
    else if(physicalAddress >= 0x80001000) /// \todo temporary until we can map in stack expansion
        return;
#endif

    m_Stack[m_StackSize / sizeof(physical_uintptr_t)] = physicalAddress;
    m_StackSize += sizeof(physical_uintptr_t);
}

Arm7PhysicalMemoryManager::PageStack::PageStack()
{
    m_StackMax = 0x1000;
    m_StackSize = 0;
}


