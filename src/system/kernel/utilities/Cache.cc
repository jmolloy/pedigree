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

#include <utilities/Cache.h>
#include <utilities/utility.h>
#include <Log.h>
#include <processor/VirtualAddressSpace.h>

MemoryAllocator Cache::m_Allocator;
Spinlock Cache::m_AllocatorLock;
static bool g_AllocatorInited = false;


Cache::Cache() :
    m_Pages(), m_Lock()
{
    if (!g_AllocatorInited)
    {
        m_Allocator.free(0xE0000000, 0x10000000);
        g_AllocatorInited = true;
    }
}

Cache::~Cache()
{
}

uintptr_t Cache::lookup (uintptr_t key)
{
    m_Lock.enter();

    uintptr_t off = key % 4096;

    CachePage *pPage = m_Pages.lookup(key&~0xFFFUL);
    if (!pPage)
    {
        m_Lock.leave();
        return 0;
    }

    uintptr_t ptr = pPage->location;
   
    m_Lock.leave();
    return ptr + off;
}

uintptr_t Cache::insert (uintptr_t key)
{
    m_Lock.acquire();

    uintptr_t off = key % 4096;
    CachePage *pPage = m_Pages.lookup(key&~0xFFFUL);

    if (pPage)
    {
        m_Lock.release();
        return pPage->location;
    }

    m_AllocatorLock.acquire();
    uintptr_t location;
    bool succeeded = m_Allocator.allocate(4096, location);
    m_AllocatorLock.release();

    if (!succeeded)
    {
        FATAL("Cache: out of address space.");
        return 0;
    }

    uintptr_t phys = PhysicalMemoryManager::instance().allocatePage();
    if (!Processor::information().getVirtualAddressSpace().map(phys, reinterpret_cast<void*>(location), VirtualAddressSpace::Write|VirtualAddressSpace::KernelMode))
    {
        FATAL("Map failed in Cache::insert())");
    }


    pPage = new CachePage;
    pPage->location = location;
    pPage->refcnt = 1;
    m_Pages.insert(key&~0xFFFUL, pPage);

    m_Lock.release();

    return location + off;
}
