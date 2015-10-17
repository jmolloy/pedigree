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

#ifndef CACHE_H
#define CACHE_H

#include <processor/types.h>
#include <processor/Processor.h>
#include <utilities/MemoryAllocator.h>
#include <utilities/UnlikelyLock.h>
#include <utilities/Tree.h>
#include <Spinlock.h>

#include <machine/TimerHandler.h>

#include <processor/PhysicalMemoryManager.h>
#include <process/MemoryPressureManager.h>

/// The age at which a cache page is considered "old" and can be evicted
/// This is expressed in seconds.
#define CACHE_AGE_THRESHOLD 10

/// In the case where no pages are old enough, this is the number of pages that
/// will be force-freed.
#define CACHE_NUM_THRESHOLD 2

/// How regularly (in milliseconds) the writeback timer handler should fire.
#define CACHE_WRITEBACK_PERIOD 500

// Forward declaration of Cache so CacheManager can be defined first
class Cache;

/** Provides a clean abstraction to a set of data caches. */
class CacheManager : public TimerHandler, public RequestQueue, public MemoryPressureHandler
{
    public:
        CacheManager();
        virtual ~CacheManager();

        static CacheManager &instance()
        {
            return m_Instance;
        }

        virtual const String getMemoryPressureDescription()
        {
          return String("Global cache compact.");
        }

        void initialise();

        void registerCache(Cache *pCache);
        void unregisterCache(Cache *pCache);

        /**
         * Compact every cache we know about, until 'count' pages have been
         * evicted. Default value for count says 'all pages in all caches'.
         */
        bool compactAll(size_t count = ~0UL);

        virtual bool compact()
        {
            return compactAll(5);
        }

        virtual void timer(uint64_t delta, InterruptState &state);

    private:
        /**
         * RequestQueue doer - children give us new jobs, and we call out to
         * them when they hit the front of the queue.
         */
        virtual uint64_t executeRequest(uint64_t p1, uint64_t p2, uint64_t p3, uint64_t p4, uint64_t p5,
                uint64_t p6, uint64_t p7, uint64_t p8);

        /**
         * Used to ensure we only ever fire a WriteBack for the same page once -
         * that is, we don't constantly write back the same page over and over
         * while it's still queued.
         */
        virtual bool compareRequests(const Request &a, const Request &b)
        {
            // p2 = CallbackCause, p3 = key in m_Pages
            return (a.p2 == b.p2) && (a.p3 == b.p3);
        }

        static CacheManager m_Instance;

        List<Cache*> m_Caches;
};

/** Provides an abstraction of a data cache. */
class Cache
{
private:
    struct CachePage
    {
        /// Key for this page.
        uintptr_t key;

        /// The location of this page in memory
        uintptr_t location;

        /// Reference count to handle release() being called with multiple
        /// threads having access to the page.
        size_t refcnt;

        /// The time at which this page was allocated. This is used by
        /// compact() to determine the best pages to evict.
        uint32_t timeAllocated;

        /// Linked list components for LRU.
        CachePage *pNext;
        CachePage *pPrev;
    };

public:

    /**
     * Callback Cause enumeration.
     *
     * Callbacks can be called for a number of reasons. Instead of having
     * a callback for each reason, we just offer this enumeration. Callbacks
     * can then do a switch on this in order to select which behaviour to
     * invoke.
     */
    enum CallbackCause
    {
        WriteBack,
        Eviction,
    };

    /**
     * Callback type: for functions called by the write-back timer handler.
     *
     * The write-back handler checks all pages in the cache at a regular
     * interval. If it finds a dirty page, it calls the Cache callback,
     * which should write the modified data back to a backing store, if
     * any exists.
     *
     * Then, the write-back thread will mark the page as not-dirty.
     */
    typedef void (*writeback_t)(CallbackCause cause, uintptr_t loc, uintptr_t page, void *meta);

    Cache();
    virtual ~Cache();

    /** Set the write back callback to the given function. */
    void setCallback(writeback_t newCallback, void *meta);

    /** Looks for \p key , increasing \c refcnt by one if returned. */
    uintptr_t lookup (uintptr_t key);

    /** Creates a cache entry with the given key. */
    uintptr_t insert (uintptr_t key);

    /** Creates a bunch of cache entries to fill a specific size. Note that
     *  this is just a monster allocation of a virtual address - the physical
     *  pages are NOT CONTIGUOUS.
     */
    uintptr_t insert (uintptr_t key, size_t size);

    /**
     * Evicts the given key from the cache, also freeing the memory it holds.
     *
     * This will respect the refcount of the given key, so as to make pin()
     * exhibit more reliable behaviour.
     */
    void evict (uintptr_t key);

    /**
     * Empties the cache.
     *
     * Will not respect refcounts.
     */
    void empty();

    /** Decreases \p key 's \c refcnt by one. */
    void release(uintptr_t key);

    /**
     * Increases \p key 's \c refcnt by one.
     *
     * This is used for places that, for example, use the physical address of
     * a cache page and therefore will never set the dirty flag of a virtual
     * page. This use case will need to provide its own means for writing data
     * back to the backing store, if that is desirable.
     *
     * Pinned pages will not be freed during a compact().
     */
    void pin(uintptr_t key);

    /** Attempts to "compact" the cache - (hopefully) reduces
     *  resource usage by throwing away items in a
     *  least-recently-used fashion. This is called in an
     *  emergency "physical memory getting full" situation by the
     *  PMM.
     *
     * Pass a count to specify that only count pages should be cleared.
     * If a count is passed, the cache prefers to remove pages that have
     * not been accessed, rather than the least-recently-used page.
     */
    size_t compact (size_t count = ~0UL);

    /**
     * Synchronises the given cache key back to a backing store, if a
     * callback has been assigned to the Cache.
     */
    void sync(uintptr_t key, bool async);

    /**
     * Enters a critical section with respect to this cache. That is, do not
     * permit write back callbacks to be fired (aside from as a side effect
     * of eviction) until the section has been left.
     *
     * This is especially useful for an 'insert then read into buffer' operation,
     * which can cause a writeback in the middle of reading (when nothing has
     * actually changed at all).
     */
    void startAtomic()
    {
        m_bInCritical = 1;
    }

    /**
     * Leaves the critical section for this cache.
     */
    void endAtomic()
    {
        m_bInCritical = 0;
    }

private:

    /**
     * evict doer
     */
    void evict(uintptr_t key, bool bLock, bool bPhysicalLock, bool bRemove);

    /**
     * LRU evict do-er.
     */
    void lruEvict();

    /**
     * Link the given CachePage to the LRU list.
     */
    void linkPage(CachePage *pPage);

    /**
     * Promote the given CachePage within the LRU list.
     *
     * This marks the page as the most-recently-used page.
     */
    void promotePage(CachePage *pPage);

    /**
     * Unlink the given CachePage from the LRU list.
     */
    void unlinkPage(CachePage *pPage);

    struct callbackMeta
    {
        CallbackCause cause;
        writeback_t callback;
        uintptr_t loc;
        uintptr_t page;
        void *meta;
        UnlikelyLock *cacheLock;
    };

public:
    /**
     * Cache timer handler.
     *
     * Will call callbacks as needed to write dirty pages back to the backing
     * store. If no callback is set for the Cache instance, the timer will
     * not fire.
     */
    virtual void timer(uint64_t delta, InterruptState &state);

    /**
     * RequestQueue doer, called by the CacheManager instance.
     */
    virtual uint64_t executeRequest(uint64_t p1, uint64_t p2, uint64_t p3, uint64_t p4, uint64_t p5,
            uint64_t p6, uint64_t p7, uint64_t p8);

private:
    /** Key-item pairs. */
    Tree<uintptr_t, CachePage*> m_Pages;

    /**
     * List of known CachePages, kept up-to-date with m_Pages but in LRU order.
     */
    CachePage *m_pLruHead;
    CachePage *m_pLruTail;

    /** Static MemoryAllocator to allocate virtual address space for all caches. */
    static MemoryAllocator m_Allocator;

    /** Lock for using the allocator. */
    static Spinlock m_AllocatorLock;

    /** Lock for this cache. */
    UnlikelyLock m_Lock;

    /** Callback to be called in the write-back timer handler. */
    writeback_t m_Callback;

    /** Timer interface: number of nanoseconds counted so far in the timer handler. */
    uint64_t m_Nanoseconds;

    /** Metadata to pass to a callback. */
    void *m_CallbackMeta;

    /** Are we currently in a critical section? */
    Atomic<size_t> m_bInCritical;
};

#endif
