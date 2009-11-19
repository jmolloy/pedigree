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

#include <processor/PhysicalMemoryManager.h>

/// The age at which a cache page is considered "old" and can be evicted
/// This is expressed in seconds.
#define CACHE_AGE_THRESHOLD 10

/// In the case where no pages are old enough, this is the number of pages that
/// will be force-freed.
#define CACHE_NUM_THRESHOLD 2

// Forward declaration of Cache so CacheManager can be defined first
class Cache;

/** Provides a clean abstraction to a set of data caches. */
class CacheManager
{
    public:
        CacheManager();
        virtual ~CacheManager();

        static CacheManager &instance()
        {
            return m_Instance;
        }

        void registerCache(Cache *pCache);
        void unregisterCache(Cache *pCache);

        void compactAll();
    private:
        static CacheManager m_Instance;

        List<Cache*> m_Caches;
};

/** Provides an abstraction of a data cache. */
class Cache
{
public:

    Cache();
    virtual ~Cache();

    /** Looks for \p key , increasing \c refcnt by one if returned. */
    uintptr_t lookup (uintptr_t key);

    /** Creates a cache entry with the given key. */
    uintptr_t insert (uintptr_t key);
    
    /** Creates a bunch of cache entries to fill a specific size. Note that
     *  this is just a monster allocation of a virtual address - the physical
     *  pages are NOT CONTIGUOUS.
     */
    uintptr_t insert (uintptr_t key, size_t size);

    /** Decreases \p key 's \c refcnt by one. */
    void release(uintptr_t key);

    /** Attempts to "compact" the cache - (hopefully) reduces
     *  resource usage by throwing away items in a
     *  least-recently-used fashion. This is called in an
     *  emergency "physical memory getting full" situation by the
     *  PMM. */
    void compact ();

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
};

#endif
