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
#if 0
#include "SlabAllocator.h"

#include <processor/Processor.h>
#include <processor/VirtualAddressSpace.h>

#include "dlmalloc.h"

#include <panic.h>
#include <utilities/assert.h>

#include <LockGuard.h>
#include <Spinlock.h>

SlabAllocator SlabAllocator::m_Instance;

SASlab::SASlab() //  : m_VirtAddr(0), m_State(Empty)
{
    /*
    m_VirtAddr = reinterpret_cast<uintptr_t>(dlmallocSbrk(4096));

    if(!
        PhysicalMemoryManager::instance().allocateRegion(
            m_MemRegion,
            SLAB_SIZE,
            PhysicalMemoryManager::continuous,
            VirtualAddressSpace::KernelMode,
            -1
        )
    )
    {
        ERROR("Couldn't allocate a slab!");
        m_VirtAddr = 0;
    }
    else
    {
        m_VirtAddr = reinterpret_cast<uintptr_t>(m_MemRegion.virtualAddress());
    }
    */
}

SASlab::~SASlab()
{
//    m_MemRegion.free();
}

SACache::SACache(size_t blkSize) :
            m_FirstSlab(0), m_BlockSize(blkSize), m_NumBlocks(0), m_BitmapSize(0)
#if BIG_BLOCK_SUPPORT
            , m_BigBlockInfo(0)
#endif
{
    // Grab a slab...
    size_t slabSize = m_BlockSize > SLAB_MINIMUM_SIZE ? m_BlockSize : SLAB_MINIMUM_SIZE;
    m_FirstSlab = SASlab::get(slabSize);
    if(!m_FirstSlab)
    {
        ERROR("Couldn't get a slab!");
        return;
    }

    // Find the number of blocks per slab we can use
    m_NumBlocks = (slabSize) / m_BlockSize;
    m_NumBlocks -= (sizeof(SlabFooter) / m_BlockSize) + 1;
    m_NumBlocks -= (m_NumBlocks / 8 ) + 1;

    // Calculate the bitmap size.
    m_BitmapSize = (m_NumBlocks / 8) + ((m_NumBlocks % 8) ? 1 : 0);

    // Initialise this first slab
    initialise(m_FirstSlab);
}

SACache::~SACache()
{
    //
}

/// \note About the bitmap size here - the extra bytes are removed because without
///       them the last byte of the bitmap will fall on the zeroth byte of the *next*
///       slab (not good!)

SACache::SlabFooter *SACache::getFooter(uintptr_t slab)
{
    assert(slab);

    size_t slabSize = m_BlockSize > SLAB_MINIMUM_SIZE ? m_BlockSize : SLAB_MINIMUM_SIZE;
    size_t footerOffset = ((slabSize) - 1) - m_BitmapSize - sizeof(SlabFooter);
    return reinterpret_cast<SlabFooter *>(slab + footerOffset);
}

uint8_t *SACache::getBitmap(uintptr_t slab)
{
    assert(slab);

    if(m_BlockSize < BIG_BLOCK_THRESHOLD)
    {
        size_t slabSize = m_BlockSize > SLAB_MINIMUM_SIZE ? m_BlockSize : SLAB_MINIMUM_SIZE;
        size_t bitmapOffset = ((slabSize) - 1) - m_BitmapSize;
        return reinterpret_cast<uint8_t*>(slab + bitmapOffset);
    }
#if BIG_BLOCK_SUPPORT
    else
    {
        // Find the slab, return its bitmap
        SlabFooter *p = m_BigBlockInfo;
        while(p)
        {
            if(p->memStart <= slab && p->memEnd > slab)
                break;
            p = reinterpret_cast<SlabFooter *>(p->NextInfo);
        }
        if(p)
            return reinterpret_cast<uint8_t*>(&(p->BigBlockBitmap));
    }
#endif

    return 0;
}

SACache::SlabFooter *SACache::initialise(uintptr_t slab)
{
    size_t slabSize = m_BlockSize > SLAB_MINIMUM_SIZE ? m_BlockSize : SLAB_MINIMUM_SIZE;

    // Determine how we should approach the footer - 2K and over use a *new* slab for the footer
    if(m_BlockSize < BIG_BLOCK_THRESHOLD)
    {
        // Add the footer
        SlabFooter *footer = getFooter(slab);
        footer->memStart = slab;
        footer->memEnd = footer->memStart + (slabSize);
        footer->numAlloc = 0;
        footer->state = SASlab::Empty;
        footer->NextSlab = 0;
#if BIG_BLOCK_SUPPORT
        footer->NextInfo = 0;
#endif

        // Zero the bitmap
        memset(getBitmap(slab), 0, m_BitmapSize);

        return footer;
    }
    else
    {
#if BIG_BLOCK_SUPPORT
        // Create the new pointer (safe, because there's a static cache for these small blocks)
        SlabFooter *p = reinterpret_cast<SlabFooter *>(SlabAllocator::instance().getFirstCache()->allocate());

        // Information about the allocation
        p->memStart = slab;
        p->memEnd = p->memStart + (slabSize);
        p->numAlloc = 0;
        p->state = SASlab::Empty;
        p->NextSlab = 0;
        p->NextInfo = 0;

        p->BigBlockBitmap = 0;

        // Link it in
        if(!m_BigBlockInfo) m_BigBlockInfo = p; else link(p);

        // Different number of blocks now
        m_NumBlocks = (slabSize) / m_BlockSize;
        m_BitmapSize = 1;

        return p;
#else
        FATAL("Write me!");
#endif
    }

    return 0;
}

uintptr_t SACache::allocate()
{
    // Find the first slab with free blocks
    uintptr_t slab = m_FirstSlab;
#if BIG_BLOCK_SUPPORT
    SlabFooter *f = (m_BlockSize < BIG_BLOCK_THRESHOLD) ? getFooter(slab) : m_BigBlockInfo;
    while(f != 0 && f->state == SASlab::Full)
    {
        if(f->NextSlab && ((m_BlockSize < BIG_BLOCK_THRESHOLD) ? 1 : f->NextInfo))
        {
            // Safe, use it
            slab = f->NextSlab;
            if(m_BlockSize < BIG_BLOCK_THRESHOLD)
                f = getFooter(slab);
            else
            {
                f = reinterpret_cast<SlabFooter *>(f->NextInfo);
                slab = f->memStart;
            }
            assert(slab);
        }
        else
            f = 0;
    }
#else
    SlabFooter *f = getFooter(slab);
    while(f != 0 && f->state == SASlab::Full)
    {
        if(f->NextSlab)
        {
            // Safe, use it
            slab = f->NextSlab;
            f = getFooter(slab);
        }
        else
            f = 0;
    }
#endif

    if(!f)
    {
        // Grab a new slab, if possible
        size_t slabSize = m_BlockSize > SLAB_MINIMUM_SIZE ? m_BlockSize : SLAB_MINIMUM_SIZE;
        slab = SASlab::get(slabSize);
        if(!slab)
        {
            FATAL("Out of memory");
            return 0;
        }

        // Set it up properly
        f = initialise(slab);

        // Link it in (unless initialise has already done it for us)
        if(m_BlockSize < BIG_BLOCK_THRESHOLD)
            link(slab);
    }

    if(f)
    {
        // Grab the bitmap and find a free block
        uint8_t *bitmap = getBitmap(slab);
        assert(bitmap != 0);
        for(size_t i = 0; i < m_BitmapSize; i++)
        {
            // If no free bits, skip
            if(bitmap[i] == 0xFF)
                continue;

            // Find the free bit here
            size_t bit;
            for(bit = 0; bit < ((m_BitmapSize == 1) ? m_NumBlocks : 8); bit++)
            {
                if(bitmap[i] & (1 << bit))
                    continue;
                break;
            }

            // If the bitmap size isn't a multiple of 8, check.
            size_t bitNumber = (i * 8) + bit;
            if (bitNumber >= m_NumBlocks)
                continue;

            // It shouldn't be possible to not have found a bit
            assert(bit < 8);

            // Set this block as used, and return it
            bitmap[i] |= (1 << bit);

            // Now partially full (or perhaps even totally full!)
            f->numAlloc++;
            if(f->numAlloc >= m_NumBlocks)
                f->state = SASlab::Full;
            else
                f->state = SASlab::Partial;

            // Return the address
            size_t realOffset = bitNumber * m_BlockSize;
            uintptr_t finalAddr = slab + realOffset;
            return finalAddr;
        }
    }

    // No memory
    f->state = SASlab::Full;
    WARNING("SlabAllocator: Full slab, returning null [blksz=" << Dec << m_BlockSize << Hex << "]!");
    return 0;
}

void SACache::free(uintptr_t blk)
{
    // Find the cache that this block is within
    uintptr_t s = m_FirstSlab, slab = m_FirstSlab;
#if BIG_BLOCK_SUPPORT
    SlabFooter *f = (m_BlockSize < BIG_BLOCK_THRESHOLD) ? getFooter(s) : m_BigBlockInfo;
    while(f != 0)
    {
        // Is this block within the slab?
        if(f->memStart <= blk && f->memEnd > blk)
        {
            // Yes, free the block
            break;
        }

        if(f->NextSlab || f->NextInfo)
        {
            // Safe, use it
            slab = f->NextSlab;
            if(m_BlockSize < BIG_BLOCK_THRESHOLD)
                f = getFooter(slab);
            else
                f = reinterpret_cast<SlabFooter *>(f->NextInfo);
        }
        else
            f = 0;
    }
#else
    SlabFooter *f = getFooter(slab);
    while(f != 0)
    {
        // Is this block within the slab?
        if(f->memStart <= blk && f->memEnd > blk)
        {
            // Yes, free the block
            break;
        }

        if(f->NextSlab)
        {

            // Safe, use it
            slab = f->NextSlab;
            f = getFooter(slab);
        }
        else
            f = 0;
    }
#endif

    // Got a block...
    if(f)
    {
        // Offset into the slab
        uintptr_t blkOffset = blk - f->memStart;
        size_t blockNumber = blkOffset / m_BlockSize;

        // Set it as free...
        size_t bitmapIndex = blockNumber / 8;
        size_t bitIndex = blockNumber % 8;
        uint8_t *bmap = getBitmap(f->memStart);

#ifdef ADDITIONAL_CHECKS
        // Assert that the bit is actually set, so that we do not attempt to free an already-freed object.
        // This is considered an error condition - freeing a freed pointer is typically a mistake.
        assert((bmap[bitmapIndex] & (1 << bitIndex)) != 0); // Potential double free?
#endif
        /// \note Above assertion seems to fail all the time... But that's before the debugger's available, so
        ///       I haven't been able to trace it yet...
        /// \todo Uncomment the above assertion

        if(bmap[bitmapIndex] & (1 << bitIndex))
            bmap[bitmapIndex] &= ~(1 << bitIndex);
        else
            return;

        // One less allocation in the slab
#ifdef ADDITIONAL_CHECKS
        assert(f->numAlloc != 0); // Double delete in this cache?
#endif
        if(f->numAlloc)
        {
            f->numAlloc--;
            if(f->numAlloc)
                f->state = SASlab::Partial;
            else
                f->state = SASlab::Empty;
        }
    }
#if DEBUGGING_SLAB_ALLOCATOR
    else
        WARNING_NOLOCK("SlabAllocator: Cache was given block " << blk << ", but I have no slab for it?");
#endif
}

void SACache::link(uintptr_t slab)
{
    // Traverse the list of caches until we find the last slab
    uintptr_t s = m_FirstSlab;
    SlabFooter *f = getFooter(s);
    while(f != 0)
    {
        if((s = f->NextSlab))
            f = getFooter(s);
        else
            break;
    }

    // Link it in
    if(f)
        f->NextSlab = slab;
}

#if BIG_BLOCK_SUPPORT
void SACache::link(SlabFooter *p)
{
    // Traverse the list...
    assert(m_BlockSize >= BIG_BLOCK_THRESHOLD);
    SlabFooter *f = m_BigBlockInfo;
    while(f->NextInfo)
        f = reinterpret_cast<SlabFooter *>(f->NextInfo);

    assert(f);

    // Link it on
    if(f)
        f->NextInfo = reinterpret_cast<uintptr_t>(p);
    p->NextInfo = 0;
}
#endif

SlabAllocator::SlabAllocator() : m_FirstCache(0), m_NextCaches(), m_bInitialised(false), m_Lock(false)
{
}

SlabAllocator::~SlabAllocator()
{
}

void SlabAllocator::initialise()
{
    static SACache cache(FIRST_STATIC_CACHE_SIZE);
    m_FirstCache = &cache;

    m_bInitialised = true;
}

uintptr_t SlabAllocator::allocateDoer(SACache *cache)
{
    // Allocate from the cache
    assert(cache != 0);
    uintptr_t p = cache->allocate();
    if(p)
    {
        // Shove some data on the front that we'll use later
        AllocHeader *head = reinterpret_cast<AllocHeader *>(p);
        p += sizeof(AllocHeader);

        // Set up the header
        head->cache = cache;

        // Success
        return p;
    }

    return 0;
}

uintptr_t SlabAllocator::allocate(size_t nBytes)
{
#if DEBUGGING_SLAB_ALLOCATOR
    NOTICE_NOLOCK("SlabAllocator::allocate(" << Dec << nBytes << Hex << ")");
#endif

    // If we're not initialised, fix that
    if(!m_bInitialised)
        initialise();

    // Return address
    uintptr_t ret = 0;

    // Add in room for the allocation footer
    nBytes += sizeof(AllocHeader);

    // Big enough to fit into the static cache?
    if(nBytes < FIRST_STATIC_CACHE_SIZE)
    {
        // Allocate from the static cache
        m_Lock.acquire();
        ret = allocateDoer(m_FirstCache);
        m_Lock.release();
    }
    else
    {
        // Round the size up to the nearest power of two
        /// \todo 32-bit only, needs a fix to support 64-bit too
        nBytes--;
        nBytes |= nBytes >> 1;
        nBytes |= nBytes >> 2;
        nBytes |= nBytes >> 4;
        nBytes |= nBytes >> 8;
        nBytes |= nBytes >> 16;
        nBytes++;

        // Do we have a cache for this allocation size yet?
        SACache *cache = m_NextCaches.lookup(nBytes);

        // No, allocate it
        if(!cache)
        {
            cache = new SACache(nBytes);
            if(cache)
                m_NextCaches.insert(nBytes, cache);
        }

        if(cache)
        {
            // We have a cache (sanity checked), allocate!
            m_Lock.acquire();
            ret = allocateDoer(cache);
            m_Lock.release();
        }
    }

#if DEBUGGING_SLAB_ALLOCATOR
    if(!ret)
        ERROR_NOLOCK("SlabAllocator::allocate: Allocation failed (" << Dec << nBytes << Hex << " bytes)");
#else
    assert(ret != 0);
#endif

    return ret;
}

void SlabAllocator::free(uintptr_t mem)
{
#if DEBUGGING_SLAB_ALLOCATOR
    NOTICE_NOLOCK("SlabAllocator::free");
#endif

    // If we're not initialised, fix that
    if(!m_bInitialised)
        initialise();

    m_Lock.acquire();

    // Grab the header
    AllocHeader *head = reinterpret_cast<AllocHeader *>(mem - sizeof(AllocHeader));

    // If the cache is null, then the pointer is corrupted.
    assert(head->cache != 0);

    // Free the memory
    head->cache->free(mem);

    m_Lock.release();
}
#endif
