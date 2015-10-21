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

#ifndef SLAM_ALLOCATOR_H
#define SLAM_ALLOCATOR_H

/** The SLAM allocator (SLAB à la James Molloy) is based on the Slab
    allocator (Bonwick94).

    \see http://www.pedigree-project.org/r/projects/pedigree/wiki/SlabDraft
**/

#ifdef BENCHMARK
#include "compat.h"
#else
#include <processor/types.h>
#include <processor/PhysicalMemoryManager.h>
#include <utilities/MemoryAllocator.h>
#include <Log.h>

#include <Spinlock.h>
#endif

/// Size of each slab in 4096-byte pages
#define SLAB_SIZE                       1

/// Minimum slab size in bytes
#define SLAB_MINIMUM_SIZE               (4096 * SLAB_SIZE)

/// Define if using the magic number method of slab recovery.
/// This turns recovery into an O(n) instead of O(n^2) algorithm,
/// but relies on a magic number which introduces false positives
/// (depending on number length and value), and requires a
/// doubly linked list instead of a singly.
#define USING_MAGIC                     1

/// Used only if USING_MAGIC. Type of the magic number.
#define MAGIC_TYPE                      uintptr_t

/// Used only if USING_MAGIC. Magic value.
#define MAGIC_VALUE                     0xb00b1e55ULL

/// Minimum size of an object.
#define OBJECT_MINIMUM_SIZE             (sizeof(SlamCache::Node))

/// Outputs information during each function call
#define DEBUGGING_SLAB_ALLOCATOR        0

/// Temporary magic used during allocation.
#define TEMP_MAGIC 0x67845753

/// Adds magic numbers to the start of free blocks, to check for
/// buffer overruns.
#ifndef USE_DEBUG_ALLOCATOR
#define OVERRUN_CHECK                   1
#else
#define OVERRUN_CHECK                   0
#endif

/// Adds magic numbers to the start and end of allocated chunks, increasing
/// object size. Also adds a small amount of backtrace information.
#define VIGILANT_OVERRUN_CHECK          0

#define VIGILANT_MAGIC                  0x1337cafe

/// This will check EVERY object on EVERY alloc/free.
/// It will cripple your performance.
#define CRIPPLINGLY_VIGILANT            0

/// If you're using a modified version of Bochs which supports magic
/// watchpoints (xchg cx, cx), this will set and remove watchpoints
/// on all allocations. This means you find out exactly where each
/// overrun occurs (EIP and all) rather than guessing.
#define BOCHS_MAGIC_WATCHPOINTS         0

/** A cache allocates objects of a constant size. */
class SlamCache
{
// struct Node must be public so that sizeof(SlamCache::Node) is available.
public:
    /** The structure inside a free object (list node) */
    struct Node
    {
        Node *next;
#if USING_MAGIC
        MAGIC_TYPE magic;
#endif
    };

    /** Default constructor, does nothing. */
    SlamCache();
    /** Destructor is not designed to be called. There is no cleanup,
        this is a kernel heap! */
    virtual ~SlamCache();

    /** Main init function. */
    void initialise(size_t objectSize);

    /** Allocates an object. */
    uintptr_t allocate();

    /** Frees an object. */
    void free(uintptr_t object);

    /** Attempt to recover slabs from this cache. */
    size_t recovery(size_t maxSlabs);

    bool isPointerValid(uintptr_t object);

    inline size_t objectSize() const
    {
        return m_ObjectSize;
    }

    inline size_t slabSize() const
    {
        return m_SlabSize;
    }

#if CRIPPLINGLY_VIGILANT
    void trackSlab(uintptr_t slab);
    void check();
#endif

private:
    SlamCache(const SlamCache &);
    const SlamCache& operator = (const SlamCache &);

#ifdef MULTIPROCESSOR
    ///\todo MAX_CPUS
    typedef Node *partialListType;
    partialListType m_PartialLists[255];
#else
    typedef volatile Node *partialListType;
    partialListType m_PartialLists[1];
#endif

    uintptr_t getSlab();
    void freeSlab(uintptr_t slab);

    Node *initialiseSlab(uintptr_t slab);

    size_t m_ObjectSize;
    size_t m_SlabSize;

    // This version of the allocator doesn't have a free list, instead
    // the reap() function returns memory directly to the VMM. This
    // avoids needing to lock the free list on MP systems.

#if CRIPPLINGLY_VIGILANT
    uintptr_t m_FirstSlab;
#endif

    /**
     * Recovery cannot be done trivially.
     * Spinlock disables interrupts as part of its operation, so we can
     * use it to ensure recovery isn't interrupted. Note recovery is a
     * per-CPU thing.
     */
    Spinlock m_RecoveryLock;
};

class SlamAllocator
{
    public:
        SlamAllocator();
        virtual ~SlamAllocator();

        void initialise();

        uintptr_t allocate(size_t nBytes);
        void free(uintptr_t mem);

        size_t recovery(size_t maxSlabs = 1);

        bool isPointerValid(uintptr_t mem);

        size_t allocSize(uintptr_t mem);

        static SlamAllocator &instance()
        {
#ifdef BENCHMARK
            static SlamAllocator instance;
            return instance;
#else
            return m_Instance;
#endif
        }

        size_t heapPageCount() const
        {
            return m_HeapPageCount;
        }

        uintptr_t getSlab(size_t fullSize);
        void freeSlab(uintptr_t address, size_t length);

#ifdef USE_DEBUG_ALLOCATOR
    inline size_t headerSize()
    {
        return sizeof(AllocHeader);
    }
    inline size_t footerSize()
    {
        return sizeof(AllocFooter);
    }
#endif

#if CRIPPLINGLY_VIGILANT
    void setVigilance(bool b)
    {m_bVigilant = b;}
    bool getVigilance()
    {return m_bVigilant;}
#endif

    private:
        SlamAllocator(const SlamAllocator&);
        const SlamAllocator& operator = (const SlamAllocator&);

#ifndef BENCHMARK
        static SlamAllocator m_Instance;
#endif

        SlamCache m_Caches[32];
public:
        /// Prepended to all allocated data. Basically just information to make
        /// freeing slightly less performance-intensive...
        struct AllocHeader
        {
#if OVERRUN_CHECK
#if BOCHS_MAGIC_WATCHPOINTS
            uint32_t catcher;
#endif
            size_t magic;
#if VIGILANT_OVERRUN_CHECK
            uintptr_t backtrace[NUM_SLAM_BT_FRAMES];
            size_t requested;
#endif
#endif
            SlamCache *cache;
        } __attribute__((aligned(16)));

        struct AllocFooter
        {
#if OVERRUN_CHECK
#if BOCHS_MAGIC_WATCHPOINTS
            uint32_t catcher;
#endif
            size_t magic;
#endif
        } __attribute__((aligned(16)));
private:
        bool m_bInitialised;

#if CRIPPLINGLY_VIGILANT
    bool m_bVigilant;
#endif

    Spinlock m_SlabRegionLock;

    size_t m_HeapPageCount;

    uint64_t *m_SlabRegionBitmap;
    size_t m_SlabRegionBitmapEntries;

    uintptr_t m_Base;
};

#endif
