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
#ifndef SLAM_ALLOCATOR_H
#define SLAM_ALLOCATOR_H

/** The SLAM allocator (SLAB à la James Molloy) is based on the Slab
    allocator (Bonwick94).

    \see http://www.pedigree-project.org/r/projects/pedigree/wiki/SlabDraft
**/

#include <processor/types.h>
#include <processor/PhysicalMemoryManager.h>
#include <Log.h>

// For dlmallocSbrk.
#include "dlmalloc.h"

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
#define MAGIC_TYPE                      uint32_t

/// Used only if USING_MAGIC. Magic value.
#define MAGIC_VALUE                     0xb00b1e55ULL

/// Minimum size of an object.
#define OBJECT_MINIMUM_SIZE             (sizeof(SlamCache::Node))

/// Outputs information during each function call
#define DEBUGGING_SLAB_ALLOCATOR        0

/// Adds magic numbers to the start of free blocks, to check for
/// buffer overruns.
#define OVERRUN_CHECK                   1

/// Adds magic numbers to the start and end of allocated chunks, increasing
/// object size. Also adds a small amount of backtrace information.
#define VIGILANT_OVERRUN_CHECK          1

#define VIGILANT_MAGIC                  0x1337cafe
#define VIGILANT_NUM_BT                 10

/// This will check EVERY object on EVERY alloc/free.
/// It will cripple your performance.
#define CRIPPLINGLY_VIGILANT            1

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
        Node *prev;
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

#if CRIPPLINGLY_VIGILANT
    void trackSlab(uintptr_t slab);
    void check();
#endif

private:
    SlamCache(const SlamCache &);
    const SlamCache& operator = (const SlamCache &);

#ifdef MULTIPROCESSOR
    ///\todo MAX_CPUS
    Node *m_PartialLists[255];
#else
    Node *m_PartialLists[1];
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
};

class SlamAllocator
{
    public:
        SlamAllocator();
        virtual ~SlamAllocator();

        void initialise();

        uintptr_t allocate(size_t nBytes);
        void free(uintptr_t mem);

        static SlamAllocator &instance()
        {
            return m_Instance;
        }

    private:
        SlamAllocator(const SlamAllocator&);
        const SlamAllocator& operator = (const SlamAllocator&);

        static SlamAllocator m_Instance;

        SlamCache m_Caches[32];
public:
        /// Prepended to all allocated data. Basically just information to make
        /// freeing slightly less performance-intensive...
        struct AllocHeader
        {
#if OVERRUN_CHECK
            size_t magic;
#if VIGILANT_OVERRUN_CHECK
            size_t backtrace[VIGILANT_NUM_BT];
#endif
#endif
            SlamCache *cache;
        };

        struct AllocFooter
        {
#if OVERRUN_CHECK
            uint8_t bytes[32];
            size_t magic;
#endif
        };
private:
        bool m_bInitialised;
};

#endif
