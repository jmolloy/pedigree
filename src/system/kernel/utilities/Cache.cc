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

// Don't allocate cache space in reverse, but DO re-use cache pages.
// This gives us wins because we don't need to reallocate page tables for
// evicted pages. Without reuse, we end up needing to clean up old page tables
// eventually.
MemoryAllocator Cache::m_Allocator(false, true);
Spinlock Cache::m_AllocatorLock;
static bool g_AllocatorInited = false;

CacheManager CacheManager::m_Instance;

static int trimTrampoline(void *p)
{
    CacheManager::instance().trimThread();
    return 0;
}

CacheManager::CacheManager() : m_Caches(), m_pTrimThread(0), m_bActive(false)
{
}

CacheManager::~CacheManager()
{
    m_bActive = false;
    m_pTrimThread->join();
}

void CacheManager::initialise()
{
    Timer *t = Machine::instance().getTimer();
    if(t)
    {
        t->registerHandler(this);
    }

    // Call out to the base class initialise() so the RequestQueue goes live.
    RequestQueue::initialise();

    // Create our main trim thread.
    Process *pParent = Processor::information().getCurrentThread()->getParent();
    m_bActive = true;
    m_pTrimThread = new Thread(pParent, trimTrampoline, 0);
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

bool CacheManager::trimAll(size_t count)
{
    size_t totalEvicted = 0;
    for(List<Cache*>::Iterator it = m_Caches.begin();
        (it != m_Caches.end()) && count;
        ++it)
    {
        size_t evicted = (*it)->trim(count);
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

void CacheManager::trimThread()
{
    while (m_bActive)
    {
        // Ask caches to trim if we're heading towards memory usage problems.
        size_t currFree = PhysicalMemoryManager::instance().freePageCount();
        size_t lowMark = MemoryPressureManager::getLowWatermark();
        size_t highMark = MemoryPressureManager::getHighWatermark();
        if (UNLIKELY(currFree <= lowMark))
        {
          // Start trimming. Trim more the closer to the high watermark we get.
          NOTICE_NOLOCK("trimThread: free page count nears high watermark, automatically trimming");
          // Increase as the amount of memory decreases beyond the low watermark.
          size_t trimCount = (lowMark - currFree) + 1;
          trimAll(trimCount);
        }
        else
            Scheduler::instance().yield();
    }
}

Cache::Cache() :
    m_Pages(), m_pLruHead(0), m_pLruTail(0), m_Lock(), m_Callback(0),
    m_Nanoseconds(0)
{
    if (!g_AllocatorInited)
    {
#if defined(X86)
        m_Allocator.free(0xE0000000, 0x10000000);
#elif defined(X64)
        // Pull in two regions: space directly after the heap, and space
        // directly after the MemoryRegion area.
        m_Allocator.free(0xFFFFFFFFD0000000, 0x10000000);
        m_Allocator.free(0xFFFFFFFF40000000, 0x3FC00000);
#elif defined(ARM_BEAGLE)
        m_Allocator.free(0xA0000000, 0x10000000);
#elif defined(HOSTED)
        // Some location in high-memory  (~32T mark).
        m_Allocator.free(0x200000000000, 0x10000000);
#elif defined(PPC_MAC)
        /// \todo real region addresses
        m_Allocator.free(0x1000, 0x1000);
#else
        #error Implement your architecture memory map area for caches into Cache::Cache
#endif
        g_AllocatorInited = true;
    }

    // Allocate any necessary iterators now, so that they're available
    // immediately and we consume their memory early.
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
   while(!m_Lock.enter()) Processor::pause();

    CachePage *pPage = m_Pages.lookup(key);
    if (!pPage)
    {
        m_Lock.leave();
        return 0;
    }

    uintptr_t ptr = pPage->location;
    pPage->refcnt ++;
    promotePage(pPage);

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

    // Do we have memory pressure - do we need to do an LRU eviction?
    lruEvict();

    uintptr_t phys = PhysicalMemoryManager::instance().allocatePage();
    if (!Processor::information().getVirtualAddressSpace().map(phys, reinterpret_cast<void*>(location), VirtualAddressSpace::Write|VirtualAddressSpace::KernelMode))
    {
        FATAL("Map failed in Cache::insert())");
    }

    Timer &timer = *Machine::instance().getTimer();

    pPage = new CachePage;
    pPage->key = key;
    pPage->location = location;
    // Enter into cache unpinned, but only if we can call an eviction callback.
    pPage->refcnt = m_Callback ? 0 : 1;
    pPage->checksum = 0;
    m_Pages.insert(key, pPage);
    linkPage(pPage);

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

        // Check for and evict pages if we're running low on memory.
        lruEvict();

        uintptr_t phys = PhysicalMemoryManager::instance().allocatePage();
        if (!Processor::information().getVirtualAddressSpace().map(phys, reinterpret_cast<void*>(location), VirtualAddressSpace::Write|VirtualAddressSpace::KernelMode))
        {
            FATAL("Map failed in Cache::insert())");
        }

        Timer &timer = *Machine::instance().getTimer();

        pPage = new CachePage;
        pPage->key = key + (page * 4096);
        pPage->location = location;

        // Enter into cache unpinned, but only if we can call an eviction callback.
        pPage->refcnt = m_Callback ? 0 : 1;
        pPage->checksum = 0;

        m_Pages.insert(key + (page * 4096), pPage);
        linkPage(pPage);

        location += 4096;
    }

    m_Lock.release();

    if(bOverlap)
        return false;

    return returnLocation;
}

bool Cache::exists(uintptr_t key, size_t length)
{
    while(!m_Lock.enter());

    bool result = true;
    for (size_t i = 0; i < length; i += 0x1000)
    {
        CachePage *pPage = m_Pages.lookup(key + (i * 0x1000));
        if (!pPage)
        {
            result = false;
            break;
        }
    }

    m_Lock.leave();

    return result;
}

bool Cache::evict(uintptr_t key)
{
    return evict(key, true, true, true);
}

void Cache::empty()
{
    while(!m_Lock.acquire());

    // Remove anything older than the given time threshold.
    for(Tree<uintptr_t, CachePage*>::Iterator it = m_Pages.begin();
        it != m_Pages.end();
        ++it)
    {
        CachePage *page = it.value();
        page->refcnt = 0;

        evict(it.key(), false, true, false);
    }

    m_Pages.clear();

    m_Lock.release();
}

bool Cache::evict(uintptr_t key, bool bLock, bool bPhysicalLock, bool bRemove)
{
    if(bLock)
        while(!m_Lock.acquire());

    CachePage *pPage = m_Pages.lookup(key);
    if (!pPage)
    {
        NOTICE("Cache::evict didn't evict " << key << " as it didn't actually exist");
        if(bLock)
            m_Lock.release();
        return false;
    }

    bool result = false;

    // Sanity check: don't evict pinned pages.
    // If we have a callback, we can evict refcount=1 pages as we can fire an
    // eviction event. Pinned pages with a configured callback have a base
    // refcount of one. Otherwise, we must be at a refcount of precisely zero
    // to permit the eviction.
    if((m_Callback && pPage->refcnt <= 1) || ((!m_Callback) && (!pPage->refcnt)))
    {
        VirtualAddressSpace &va = Processor::information().getVirtualAddressSpace();
        void *loc = reinterpret_cast<void *>(pPage->location);

        // Good to go. Trigger a writeback if we know this was a dirty page.
        if (!verifyChecksum(pPage))
        {
            m_Callback(WriteBack, key, pPage->location, m_CallbackMeta);
        }

        physical_uintptr_t phys;
        size_t flags;
        va.getMapping(loc, phys, flags);

        // Remove from our tracking.
        if(bRemove)
        {
            m_Pages.remove(key);
            unlinkPage(pPage);
        }

        // Eviction callback.
        if(m_Callback)
            m_Callback(Eviction, key, pPage->location, m_CallbackMeta);

        // Clean up resources now that all callbacks and removals are complete.
        va.unmap(loc);
        PhysicalMemoryManager::instance().freePage(phys);

        // Allow the space to be used again.
        m_Allocator.free(pPage->location, 4096);
        delete pPage;
        result = true;
    }

    if(bLock)
        m_Lock.release();

    return result;
}

bool Cache::pin (uintptr_t key)
{
    while(!m_Lock.acquire());

    CachePage *pPage = m_Pages.lookup(key);
    if (!pPage)
    {
        m_Lock.release();
        return false;
    }

    pPage->refcnt ++;
    promotePage(pPage);

    m_Lock.release();
    return true;
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
        // Trigger an eviction. The eviction will check refcnt, and won't do
        // anything if the refcnt is raised again.
        CacheManager::instance().addAsyncRequest(1, reinterpret_cast<uint64_t>(this), PleaseEvict, key);
    }

    m_Lock.release();
}

size_t Cache::trim(size_t count)
{
    LockGuard<UnlikelyLock> guard(m_Lock);

    if(!count)
        return 0;

    size_t nPages = 0;

    // Attempt an LRU compact.
    size_t n = 0;
    while ((nPages < count) && ((n = lruEvict(true)) > 0))
    {
        nPages += n;
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
    promotePage(pPage);

    m_Lock.release();

    if(async)
        CacheManager::instance().addAsyncRequest(1, reinterpret_cast<uint64_t>(this), WriteBack, key, location);
    else
        CacheManager::instance().addRequest(1, reinterpret_cast<uint64_t>(this), WriteBack, key, location);
}

void Cache::triggerChecksum(uintptr_t key)
{
    LockGuard<UnlikelyLock> guard(m_Lock);

    CachePage *pPage = m_Pages.lookup(key);
    if (!pPage)
    {
        return;
    }

    calculateChecksum(pPage);
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
        m_Nanoseconds = 0;
        return;
    }

    for(Tree<uintptr_t, CachePage*>::Iterator it = m_Pages.begin();
        it != m_Pages.end();
        ++it)
    {
        CachePage *page = it.value();
        if (verifyChecksum(page, true))
        {
            // Checksum didn't change from the last check. If we saw it
            // changing, perform an actual writeback now.
            if (!page->checksumChanging)
                continue;

            page->checksumChanging = false;
        }
        else
        {
            // Saw the checksum change. Don't write back immediately - wait for
            // any possible further changes to be seen.
            page->checksumChanging = true;
            continue;
        }

        // Promote - page is dirty since we last saw it.
        promotePage(page);

        // Queue a writeback for this dirty page to its backing store.
        CacheManager::instance().addAsyncRequest(1, reinterpret_cast<uint64_t>(this), WriteBack, it.key(), page->location);
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

    // Eviction request?
    if (static_cast<CallbackCause>(p2) == PleaseEvict)
    {
        evict(p3, false, true, true);
        return 0;
    }

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

size_t Cache::lruEvict(bool force)
{
    if (!(m_pLruHead && m_pLruTail))
        return 0;

    // Do we have memory pressure - do we need to do an LRU eviction?
    if (force || (PhysicalMemoryManager::instance().freePageCount() <
        MemoryPressureManager::getLowWatermark()))
    {
        // Yes, perform the LRU eviction.
        CachePage *toEvict = m_pLruTail;
        if (evict(toEvict->key, false, true, true))
            return 1;
        else
        {
            // Bump the page's priority up as eviction failed for some reason.
            promotePage(toEvict);
        }
    }

    return 0;
}

void Cache::linkPage(CachePage *pPage)
{
    pPage->pPrev = 0;
    pPage->pNext = m_pLruHead;
    if (m_pLruHead)
        m_pLruHead->pPrev = pPage;
    m_pLruHead = pPage;
    if (!m_pLruTail)
        m_pLruTail = m_pLruHead;
}

void Cache::promotePage(CachePage *pPage)
{
    unlinkPage(pPage);
    linkPage(pPage);
}

void Cache::unlinkPage(CachePage *pPage)
{
    if (pPage->pPrev)
        pPage->pPrev->pNext = pPage->pNext;
    if (pPage->pNext)
        pPage->pNext->pPrev = pPage->pPrev;
    if (pPage == m_pLruTail)
        m_pLruTail = pPage->pPrev;
    if (pPage == m_pLruHead)
        m_pLruHead = pPage->pNext;
}

void Cache::calculateChecksum(CachePage *pPage)
{
    void *buffer = reinterpret_cast<void *>(pPage->location);
    pPage->checksum = checksum(buffer, 4096);
}

bool Cache::verifyChecksum(CachePage *pPage, bool replace)
{
    void *buffer = reinterpret_cast<void *>(pPage->location);
    uint16_t current = checksum(buffer, 4096);
    bool result = (pPage->checksum == 0) || (pPage->checksum == current);
    if (replace)
        pPage->checksum = current;
    return result;
}

uint16_t Cache::checksum(const void *data, size_t len)
{
    // Fletcher 16-bit checksum.
    uint16_t sum1 = 0, sum2 = 0;
    const uint8_t *p = reinterpret_cast<const uint8_t *>(data);

    for (size_t i = 0; i < len; ++i)
    {
        sum1 = (sum1 + p[i]) % 255;
        sum2 = (sum2 + sum1) % 255;
    }

    return (sum2 << 8) | sum1;
}
