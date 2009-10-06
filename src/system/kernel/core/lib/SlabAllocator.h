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
#ifndef SLAB_ALLOCATOR_H
#if 0
#define SLAB_ALLOCATOR_H

/** An implementation of the Slab Allocator (Bonwick94)
  * \todo Lots of room for improvement!
  *        - 64-bit isn't handled in the "round up to power of two" code
  *        - Locking is optimistic at best, needs to be fixed up
  *        - Still depends on dlmallocSbrk - no need for contiguous RAM anymore!
  *        - Needs benchmarking
  */

#include <processor/types.h>
#include <processor/MemoryRegion.h>
#include <processor/PhysicalMemoryManager.h>
#include <Log.h>

#include <utilities/Tree.h>

#include "dlmalloc.h"

/// Size of each slab in 4096-byte pages
#define SLAB_SIZE                       1

/// Minimum slab size in bytes
#define SLAB_MINIMUM_SIZE               (4096 * SLAB_SIZE)

/// Block size of the first, static, cache
#define FIRST_STATIC_CACHE_SIZE         32

/// Threshold for a "big block"
#define BIG_BLOCK_THRESHOLD             1024

/// Outputs information during each function call
#define DEBUGGING_SLAB_ALLOCATOR        0

/// Whether or not to support blocks which are bigger than BIG_BLOCK_THRESHOLD
#define BIG_BLOCK_SUPPORT               1

class SASlab
{
    public:
        /// Slab state
        enum SlabState
        {
            /** Nothing allocated in this slab yet */
            Empty = 0,

            /** One or more items allocated from this slab */
            Partial = 1,

            /** Slab is full - no more items free */
            Full = 2
        };

        SASlab();
        virtual ~SASlab();

        static uintptr_t get(size_t slabSize)
        {
            return reinterpret_cast<uintptr_t>(dlmallocSbrk(slabSize));
        }
    private:

        /// The actual memory region we're using here
        /// \note If we could allocate regions in a specific virtual address range, this'd be fine.
        // MemoryRegion m_MemRegion;
};

class SACache
{
    private:
        /// What follows this in RAM is a bitmap that stores information
        /// about the state of each block in the slab
        struct SlabFooter
        {
            // Footer to add to each slab/region/HELP :D...

            /// Start of our slab, for freeing
            uintptr_t memStart;

            /// End of our slab, for freeing
            uintptr_t memEnd;

            /// Hint for bitmap search
            uint16_t numAlloc;

            /// Next slab in the chain
            uintptr_t NextSlab;

#if BIG_BLOCK_SUPPORT
            // Next information structure in the chain (big blocks only)
            uintptr_t NextInfo;

            // Big block bitmap
            uint8_t BigBlockBitmap;
#endif

            /// Slab state
            SASlab::SlabState state;
        } __attribute__((packed));

    public:
        SACache(size_t blkSize);
        virtual ~SACache();

        // Allocates a block from this cache
        uintptr_t allocate();

        // Frees a block from this cache
        void free(uintptr_t blk);

        // Initialises a slab for use in the cache
        SlabFooter *initialise(uintptr_t slab);

        // Links a new cache slab
        void link(uintptr_t slab);

#if BIG_BLOCK_SUPPORT
        // Links a new big block information structure
        void link(SlabFooter *p);
#endif

        // Gets the footer location given a slab
        SlabFooter *getFooter(uintptr_t slab);

        // Gets the bitmap location given a slab
        uint8_t *getBitmap(uintptr_t slab);

        // Gets the cache block size
        size_t getBlockSize()
        {
            return m_BlockSize;
        }
    private:
        SACache(const SACache &);
        const SACache& operator = (const SACache &);

        /// First slab
        uintptr_t m_FirstSlab;

        /// Allocated block sizes
        size_t m_BlockSize;

        /// Number of blocks we have per slab
        size_t m_NumBlocks;

        /// Size of the allocation bitmap
        size_t m_BitmapSize;

#if BIG_BLOCK_SUPPORT
        /// Big allocation block first information structure
        SlabFooter *m_BigBlockInfo;
#endif
};

class SlabAllocator
{
    public:
        SlabAllocator();
        virtual ~SlabAllocator();

        void initialise();

        uintptr_t allocate(size_t nBytes);
        void free(uintptr_t mem);

        static SlabAllocator &instance()
        {
            return m_Instance;
        }

        uintptr_t allocateDoer(SACache *cache);

        inline SACache *getFirstCache()
        {
            return m_FirstCache;
        }
    private:
        SlabAllocator(const SlabAllocator&);
        const SlabAllocator& operator = (const SlabAllocator&);

        static SlabAllocator m_Instance;

        SACache *m_FirstCache;
        Tree<size_t, SACache*> m_NextCaches;

        /// Prepended to all allocated data. Basically just information to make
        /// freeing slightly less performance-intensive...
        struct AllocHeader
        {
            SACache *cache;
        } __attribute__((packed));

        bool m_bInitialised;

        Spinlock m_Lock;
};

#endif
#endif
