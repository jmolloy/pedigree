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

#include <Log.h>
#include <processor/VirtualAddressSpace.h>
#include <utilities/Cache.h>
#include <utilities/assert.h>
#include <utilities/utility.h>
#include <machine/Timer.h>
#include <machine/Machine.h>

MemoryAllocator Cache::m_Allocator;
Spinlock Cache::m_AllocatorLock;
static bool g_AllocatorInited = false;

CacheManager CacheManager::m_Instance;

CacheManager::CacheManager() : m_Caches()
{
}

CacheManager::~CacheManager()
{
}

void CacheManager::registerCache(Cache *pCache)
{
    m_Caches.pushBack(pCache);
}
void CacheManager::unregisterCache(Cache *pCache)
{
    for(List<Cache*>::Iterator it = m_Caches.begin();
        it != m_Caches.end();
        it++)
    {
        if((*it) == pCache)
        {
            m_Caches.erase(it);
            return;
        }
    }
}

void CacheManager::compactAll()
{
    for(List<Cache*>::Iterator it = m_Caches.begin();
        it != m_Caches.end();
        it++)
    {
        (*it)->compact();
    }
}

Cache::Cache() :
    m_Pages(), m_Lock()
{
    if (!g_AllocatorInited)
    {
#if defined(X86)
        m_Allocator.free(0xE0000000, 0x10000000);
#elif defined(X64)
        m_Allocator.free(0xFFFFFFFFD0000000, 0x10000000);
#elif defined(ARM_BEAGLE)
        m_Allocator.free(0xA0000000, 0x10000000);
#else
        #error Implement your architecture memory map area for caches into Cache::Cache
#endif
        g_AllocatorInited = true;
    }

    CacheManager::instance().registerCache(this);
}

Cache::~Cache()
{
    m_Lock.acquire();

    // Clean up existing cache pages
    for(Tree<uintptr_t, CachePage*>::Iterator it = m_Pages.begin();
        it != m_Pages.end();
        it++)
    {
        CachePage *page = reinterpret_cast<CachePage*>(it.value());
        VirtualAddressSpace &va = Processor::information().getVirtualAddressSpace();
        if(va.isMapped(reinterpret_cast<void*>(page->location)))
        {
            physical_uintptr_t phys;
            size_t flags;
            va.getMapping(reinterpret_cast<void*>(page->location), phys, flags);
            PhysicalMemoryManager::instance().freePage(phys);
        }

        m_Allocator.free(reinterpret_cast<uintptr_t>(page), 4096);
    }

    m_Lock.release();

    CacheManager::instance().unregisterCache(this);
}

uintptr_t Cache::lookup (uintptr_t key)
{
    m_Lock.enter();

    CachePage *pPage = m_Pages.lookup(key);
    if (!pPage)
    {
        m_Lock.leave();
        return 0;
    }

    uintptr_t ptr = pPage->location;
    pPage->refcnt ++;

    m_Lock.leave();
    return ptr;
}

uintptr_t Cache::insert (uintptr_t key)
{
    m_Lock.acquire();

    CachePage *pPage = m_Pages.lookup(key);

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
        m_Lock.release();
        FATAL("Cache: out of address space.");
        return 0;
    }

    uintptr_t phys = PhysicalMemoryManager::instance().allocatePage();
    if (!Processor::information().getVirtualAddressSpace().map(phys, reinterpret_cast<void*>(location), VirtualAddressSpace::Write|VirtualAddressSpace::KernelMode))
    {
        FATAL("Map failed in Cache::insert())");
    }

    Timer &timer = *Machine::instance().getTimer();

    pPage = new CachePage;
    pPage->location = location;
    pPage->refcnt = 1;
    pPage->timeAllocated = timer.getUnixTimestamp();
    m_Pages.insert(key, pPage);

    m_Lock.release();

    return location;
}

uintptr_t Cache::insert (uintptr_t key, size_t size)
{
    m_Lock.acquire();

    if(size % 4096)
    {
        WARNING("Cache::insert called with a size that isn't page-aligned");
        size &= ~0xFFF;
    }

    size_t nPages = size / 4096;

    // Already allocated buffer?
    CachePage *pPage = m_Pages.lookup(key);
    if (pPage)
    {
        m_Lock.release();
        return pPage->location;
    }

    // Nope, so let's allocate this block
    m_AllocatorLock.acquire();
    uintptr_t location;
    bool succeeded = m_Allocator.allocate(size, location);
    m_AllocatorLock.release();

    if (!succeeded)
    {
        m_Lock.release();
        ERROR("Cache: can't allocate " << Dec << size << Hex << " bytes.");
        return 0;
    }

    uintptr_t returnLocation = location;
    bool bOverlap = false;
    for(size_t page = 0; page < nPages; page++)
    {
        pPage = m_Pages.lookup(key + (page * 4096));
        if(pPage)
        {
            bOverlap = true;
            continue; // Don't overwrite existing buffers
        }

        uintptr_t phys = PhysicalMemoryManager::instance().allocatePage();
        if (!Processor::information().getVirtualAddressSpace().map(phys, reinterpret_cast<void*>(location), VirtualAddressSpace::Write|VirtualAddressSpace::KernelMode))
        {
            FATAL("Map failed in Cache::insert())");
        }

        Timer &timer = *Machine::instance().getTimer();

        pPage = new CachePage;
        pPage->location = location;
        pPage->refcnt = 1;
        pPage->timeAllocated = timer.getUnixTimestamp();
        m_Pages.insert(key + (page * 4096), pPage);

        location += 4096;
    }

    m_Lock.release();

    if(bOverlap)
        return false;

    return returnLocation;
}

void Cache::release (uintptr_t key)
{
    m_Lock.enter();

    CachePage *pPage = m_Pages.lookup(key);
    if (!pPage)
    {
        m_Lock.leave();
        return;
    }

    assert (pPage->refcnt);
    pPage->refcnt --;

    m_Lock.leave();
}

void Cache::compact()
{
    m_Lock.acquire();

    Timer &timer = *Machine::instance().getTimer();
    uint32_t now = timer.getUnixTimestamp();

    // For erasing later
    List<void*> m_PagesToErase;

    // Iterate through all pages, checking if any are over the time threshold
    bool bPagesFreed = false;
    for(Tree<uintptr_t, CachePage*>::Iterator it = m_Pages.begin();
        it != m_Pages.end();
        it++)
    {
        CachePage *page = reinterpret_cast<CachePage*>(it.value());
        if((page->timeAllocated + CACHE_AGE_THRESHOLD) <= now)
            m_PagesToErase.pushBack(reinterpret_cast<void*>(page->location));
    }

    // If no pages were added to the free list, we need to use a count threshold
    if(!m_PagesToErase.count())
    {
        int i = 0;
        for(Tree<uintptr_t, CachePage*>::Iterator it = m_Pages.begin();
            it != m_Pages.end();
            it++)
        {
            CachePage *page = reinterpret_cast<CachePage*>(it.value());
            m_PagesToErase.pushBack(reinterpret_cast<void*>(page->location));

            if(i++ >= CACHE_NUM_THRESHOLD)
                break;
        }
    }

    // We should have pages to erase now...
    while(m_PagesToErase.count())
    {
        void *page = m_PagesToErase.popFront();
        if(!page)
            break;

        VirtualAddressSpace &va = Processor::information().getVirtualAddressSpace();
        if(va.isMapped(page))
        {
            physical_uintptr_t phys;
            size_t flags;
            va.getMapping(page, phys, flags);
            PhysicalMemoryManager::instance().freePageUnlocked(phys);
        }

        m_Allocator.free(reinterpret_cast<uintptr_t>(page), 4096);
    }
    
    m_Lock.release();
}
