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

void CacheManager::initialise()
{
    Timer *t = Machine::instance().getTimer();
    if(t)
    {
        t->registerHandler(this);
    }

    MemoryPressureManager::instance().registerHandler(MemoryPressureManager::MediumPriority, this);

    // Call out to the base class initialise() so the RequestQueue goes live.
    RequestQueue::initialise();
}

void CacheManager::registerCache(Cache *pCache)
{
    m_Caches.pushBack(pCache);
}

void CacheManager::unregisterCache(Cache *pCache)
{
    for(List<Cache*>::Iterator it = m_Caches.begin();
        it != m_Caches.end();
        ++it)
    {
        if((*it) == pCache)
        {
            m_Caches.erase(it);
            return;
        }
    }
}

bool CacheManager::compactAll(size_t count)
{
    size_t totalEvicted = 0;
    for(List<Cache*>::Iterator it = m_Caches.begin();
        (it != m_Caches.end()) && count;
        ++it)
    {
        size_t evicted = (*it)->compact(count);
        totalEvicted += evicted;
        count -= evicted;
    }

    return totalEvicted != 0;
}

void CacheManager::timer(uint64_t delta, InterruptState &state)
{
    for(List<Cache*>::Iterator it = m_Caches.begin();
        it != m_Caches.end();
        ++it)
    {
        (*it)->timer(delta, state);
    }
}

uint64_t CacheManager::executeRequest(uint64_t p1, uint64_t p2, uint64_t p3, uint64_t p4,
                                      uint64_t p5, uint64_t p6, uint64_t p7, uint64_t p8) {
    if(!p1)
        return 0;
    Cache *pCache = reinterpret_cast<Cache *>(p1);

    // Valid registered cache?
    bool bCacheFound = false;
    for(List<Cache*>::Iterator it = m_Caches.begin();
        it != m_Caches.end();
        ++it)
    {
        if((*it) == pCache)
        {
            bCacheFound = true;
            break;
        }
    }

    if(!bCacheFound)
    {
        ERROR("CacheManager::executeRequest for an unregistered cache!");
        return 0;
    }

    return pCache->executeRequest(p1, p2, p3, p4, p5, p6, p7, p8);
}

Cache::Cache() :
    m_Pages(), m_Lock(), m_Callback(0), m_Nanoseconds(0), m_bRegisteredHandler(false)
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

    // Allocate any necessary iterators before we hit a Cache::compact() call.
    // The heap must not be used during compacting, as a heap allocation could
    // have been the culprit of the compact.
    m_Pages.begin();
    m_Pages.end();

    CacheManager::instance().registerCache(this);
}

Cache::~Cache()
{
    // Clean up existing cache pages
    for(Tree<uintptr_t, CachePage*>::Iterator it = m_Pages.begin();
        it != m_Pages.end();
        it++)
    {
        evict(it.key());
    }

    CacheManager::instance().unregisterCache(this);
}

uintptr_t Cache::lookup (uintptr_t key)
{
   while(!m_Lock.enter());

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
    while(!m_Lock.acquire());

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
    while(!m_Lock.acquire());

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

        // Enter into cache unpinned, but only if we can call an eviction callback.
        pPage->refcnt = m_Callback ? 0 : 1;

        pPage->timeAllocated = timer.getUnixTimestamp();
        m_Pages.insert(key + (page * 4096), pPage);

        location += 4096;
    }

    m_Lock.release();

    if(bOverlap)
        return false;

    return returnLocation;
}

void Cache::evict(uintptr_t key)
{
    evict(key, true, true, true);
}

void Cache::empty()
{
    while(!m_Lock.acquire());

    // Remove anything older than the given time threshold.
    for(Tree<uintptr_t, CachePage*>::Iterator it = m_Pages.begin();
        it != m_Pages.end();
        ++it)
    {
        CachePage *page = reinterpret_cast<CachePage*>(it.value());
        page->refcnt = 0;

        evict(it.key(), false, true, false);
    }

    m_Pages.clear();

    m_Lock.release();
}

void Cache::evict(uintptr_t key, bool bLock, bool bPhysicalLock, bool bRemove)
{
    if(bLock)
        while(!m_Lock.acquire());

    CachePage *pPage = m_Pages.lookup(key);
    if (!pPage)
    {
        if(bLock)
            m_Lock.release();
        return;
    }

    // Sanity check: don't evict pinned pages.
    // If we have a callback, we can evict refcount=1 pages as we can fire an
    // eviction event. Pinned pages with a configured callback have a base
    // refcount of one. Otherwise, we must be at a refcount of precisely zero
    // to permit the eviction.
    if((m_Callback && pPage->refcnt <= 1) || ((!m_Callback) && (!pPage->refcnt)))
    {
        // Good to go.
        VirtualAddressSpace &va = Processor::information().getVirtualAddressSpace();
        void *loc = reinterpret_cast<void *>(pPage->location);
        if(va.isMapped(loc))
        {
            physical_uintptr_t phys;
            size_t flags;
            va.getMapping(loc, phys, flags);

            if(m_Callback && (flags & VirtualAddressSpace::Dirty))
            {
                // Dirty - request a write-back before we free the page.
                m_Callback(WriteBack, key, pPage->location, m_CallbackMeta);
            }

            va.unmap(loc);
            if(bPhysicalLock)
                PhysicalMemoryManager::instance().freePage(phys);
            else
                PhysicalMemoryManager::instance().freePageUnlocked(phys);
        }

        m_Allocator.free(pPage->location, 4096);

        if(bRemove)
            m_Pages.remove(key);

        // Eviction callback.
        if(m_Callback)
            m_Callback(Eviction, key, pPage->location, m_CallbackMeta);

        delete pPage;
    }

    if(bLock)
        m_Lock.release();
}

void Cache::pin (uintptr_t key)
{
    while(!m_Lock.acquire());

    CachePage *pPage = m_Pages.lookup(key);
    if (!pPage)
    {
        m_Lock.release();
        return;
    }

    pPage->refcnt ++;

    m_Lock.release();
}

void Cache::release (uintptr_t key)
{
    while(!m_Lock.acquire());

    CachePage *pPage = m_Pages.lookup(key);
    if (!pPage)
    {
        m_Lock.release();
        return;
    }

    assert (pPage->refcnt);
    pPage->refcnt --;

    if (!pPage->refcnt)
    {
        // Evict this page - refcnt dropped to zero.
        evict(key, false, true, true);
    }

    m_Lock.release();
}

size_t Cache::compact(size_t count)
{
    if(!count)
        return 0;

    size_t nPages = 0;

    Timer &timer = *Machine::instance().getTimer();
    uint32_t now = timer.getUnixTimestamp();

    VirtualAddressSpace &va = Processor::information().getVirtualAddressSpace();

    // Remove anything older than the given time threshold.
    for(Tree<uintptr_t, CachePage*>::Iterator it = m_Pages.begin();
        it != m_Pages.end();
        ++it)
    {
        CachePage *page = reinterpret_cast<CachePage*>(it.value());

        // If page has been pinned, it is completely unsafe to remove.
        if(!((m_Callback && (page->refcnt > 1)) || ((!m_Callback) && (page->refcnt > 0))))
        {
            if((page->timeAllocated + CACHE_AGE_THRESHOLD) <= now)
            {
                evict(it.key(), false, false, false);
                m_Pages.erase(it++);

                if(++nPages >= count)
                    break;
            }
            else
                ++it;
        }
        else
            ++it;
    }

    // If we still need to find pages, let's find pages that have not
    // been accessed since the last compact/writeback.
    if(nPages < count)
    {
        for(Tree<uintptr_t, CachePage*>::Iterator it = m_Pages.begin();
            it != m_Pages.end();
            ++it)
        {
            CachePage *page = reinterpret_cast<CachePage*>(it.value());

            // Only if page has not been pinned - dirty flag is not enough information.
            if(!((m_Callback && (page->refcnt > 1)) || ((!m_Callback) && (page->refcnt > 0))))
            {
                if(va.isMapped(reinterpret_cast<void *>(page->location)))
                {
                    physical_uintptr_t phys;
                    size_t flags;
                    va.getMapping(reinterpret_cast<void *>(page->location), phys, flags);

                    // Not accessed? Awesome!
                    if(!(flags & VirtualAddressSpace::Accessed))
                    {
                        // But watch out for dirty pages - they have not been written back yet.
                        if(flags & VirtualAddressSpace::Dirty)
                            continue;

                        evict(it.key(), false, false, false);
                        m_Pages.erase(it++);

                        if(++nPages >= count)
                            break;
                    }
                    else
                    {
                        // Clear the accessed flag, ready for the next compact that might come.
                        flags &= ~(VirtualAddressSpace::Accessed);
                        va.setFlags(reinterpret_cast<void *>(page->location), flags);

                        // Increment iterator as we did not erase.
                        ++it;
                    }
                }
            }
            else
                ++it;
        }
    }

    // If no pages were added to the free list, we need to just rip out pages.
    if(nPages < count)
    {
        int i = 0;
        for(Tree<uintptr_t, CachePage*>::Iterator it = m_Pages.begin();
            it != m_Pages.end();)
        {
            CachePage *page = reinterpret_cast<CachePage*>(it.value());

            // Only if page has not been pinned - otherwise completely unsafe to remove.
            if(!((m_Callback && (page->refcnt > 1)) || ((!m_Callback) && (page->refcnt > 0))))
            {
                evict(it.key(), false, false, false);
                m_Pages.erase(it++);
            }
            else
                ++it;

            if((i++ >= CACHE_NUM_THRESHOLD) || (++nPages >= count))
                break;
        }
    }

    return nPages;
}

void Cache::sync(uintptr_t key, bool async)
{
    if(!m_Callback)
        return;

    while(!m_Lock.acquire());

    CachePage *pPage = m_Pages.lookup(key);
    if (!pPage)
    {
        m_Lock.release();
        return;
    }

    uintptr_t location = pPage->location;

    m_Lock.release();

    if(async)
        CacheManager::instance().addAsyncRequest(1, reinterpret_cast<uint64_t>(this), WriteBack, key, location);
    else
        CacheManager::instance().addRequest(1, reinterpret_cast<uint64_t>(this), WriteBack, key, location);
}

void Cache::timer(uint64_t delta, InterruptState &state)
{
    m_Nanoseconds += delta;
    if(LIKELY(m_Nanoseconds < (CACHE_WRITEBACK_PERIOD * 1000000ULL)))
        return;
    else if(UNLIKELY(m_Callback == 0))
        return;
    else if(UNLIKELY(m_bInCritical == 1)) {
        // Missed - don't smash the system constantly doing this check.
        m_Nanoseconds = 0;
        return;
    }

    VirtualAddressSpace &va = Processor::information().getVirtualAddressSpace();

    if(!m_Lock.enter())
    {
        // IGNORE the timer firing for this particular callback.
        // We cannot block here as we are in the context of a timer IRQ.
        WARNING_NOLOCK("Cache: writeback timer fired, but couldn't get lock");
        return;
    }

    for(Tree<uintptr_t, CachePage*>::Iterator it = m_Pages.begin();
        it != m_Pages.end();
        ++it)
    {
        CachePage *page = reinterpret_cast<CachePage*>(it.value());
        if(va.isMapped(reinterpret_cast<void *>(page->location)))
        {
            physical_uintptr_t phys;
            size_t flags;
            va.getMapping(reinterpret_cast<void *>(page->location), phys, flags);

            // If dirty, write back to the backing store.
            if(flags & VirtualAddressSpace::Dirty)
            {
                // Queue the callback!
                CacheManager::instance().addAsyncRequest(1, reinterpret_cast<uint64_t>(this), WriteBack, it.key(), page->location);

                // Clear dirty flag - written back.
                flags &= ~(VirtualAddressSpace::Dirty | VirtualAddressSpace::Accessed);
                va.setFlags(reinterpret_cast<void *>(page->location), flags);
            }
        }
    }

    m_Lock.leave();

    m_Nanoseconds = 0;
}

void Cache::setCallback(Cache::writeback_t newCallback, void *meta)
{
    m_Callback = newCallback;
    m_CallbackMeta = meta;
}

uint64_t Cache::executeRequest(uint64_t p1, uint64_t p2, uint64_t p3, uint64_t p4,
                               uint64_t p5, uint64_t p6, uint64_t p7, uint64_t p8)
{
    if(!m_Callback)
        return 0;

    // Pin page while we do our writeback
    pin(p3);

#ifdef SUPERDEBUG
    NOTICE("Cache: writeback for off=" << p3 << " @" << p3 << "!");
#endif
    m_Callback(static_cast<CallbackCause>(p2), p3, p4, m_CallbackMeta);
#ifdef SUPERDEBUG
    NOTICE_NOLOCK("Cache: writeback for off=" << p3 << " @" << p3 << " complete!");
#endif

    // Unpin page, writeback complete
    release(p3);

    return 0;
}

