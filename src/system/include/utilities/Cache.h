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
class CacheManager : public TimerHandler
{
    public:
        CacheManager();
        virtual ~CacheManager();

        static CacheManager &instance()
        {
            return m_Instance;
        }

        void initialise();

        void registerCache(Cache *pCache);
        void unregisterCache(Cache *pCache);

        /**
         * Compact every cache we know about, until 'count' pages have been
         * evicted. Default value for count says 'all pages in all caches'.
         */
        void compactAll(size_t count = ~0UL);

        virtual void timer(uint64_t delta, InterruptState &state);
    private:
        static CacheManager m_Instance;

        List<Cache*> m_Caches;
};

/** Provides an abstraction of a data cache. */
class Cache
{
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

    /// \todo possibly need a callback type for eviction too...

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

private:

    /**
     * evict doer
     */
    void evict(uintptr_t key, bool bLock, bool bRemove);

    struct callbackMeta
    {
        CallbackCause cause;
        writeback_t callback;
        uintptr_t loc;
        uintptr_t page;
        void *meta;
    };

    /**
     * Callback thread entry point.
     *
     * We can't call callbacks from the timer handler directly, as we cannot
     * allow the timer handler to block when many other things use and require
     * it to be firing more regularly. So we create a thread for each callback.
     */
    static int callbackThread(void *meta);

public:
    /**
     * Cache timer handler.
     *
     * Will call callbacks as needed to write dirty pages back to the backing
     * store. If no callback is set for the Cache instance, the timer will
     * not fire.
     */
    virtual void timer(uint64_t delta, InterruptState &state);

private:

    struct CachePage
    {
        /// The location of this page in memory
        uintptr_t location;

        /// Reference count to handle release() being called with multiple
        /// threads having access to the page.
        size_t refcnt;

        /// The time at which this page was allocated. This is used by
        /// compact() to determine the best pages to evict.
        uint32_t timeAllocated;
    };

    /** Key-item pairs. */
    Tree<uintptr_t, CachePage*> m_Pages;

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

    /** Have we registered ourselves as a timer handler yet? */
    bool m_bRegisteredHandler;
};

#endif
