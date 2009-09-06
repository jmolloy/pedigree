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

#include "SlamAllocator.h"

#include <utilities/assert.h>

SlamAllocator SlamAllocator::m_Instance;

SlamCache::SlamCache() :
    m_ObjectSize(0), m_SlabSize(0)
{
}

SlamCache::~SlamCache()
{
}

void SlamCache::initialise(size_t objectSize)
{
    if (objectSize < OBJECT_MINIMUM_SIZE)
        return;

    m_ObjectSize = objectSize;
    if (m_ObjectSize > SLAB_MINIMUM_SIZE)
        m_SlabSize = m_ObjectSize;
    else
        m_SlabSize = SLAB_MINIMUM_SIZE;

    size_t maxCpu = 1;
#ifdef MULTIPROCESSOR
    maxCpu = 255;
#endif
    for (size_t i = 0; i < maxCpu; i++)
        m_PartialLists[i] = 0;

    assert( (m_SlabSize % m_ObjectSize) == 0 );
}

uintptr_t SlamCache::allocate()
{
#ifdef MULTIPROCESSOR
    size_t thisCpu = Processor::information().id();
#else
    size_t thisCpu = 0;
#endif

    if (m_PartialLists[thisCpu] != 0)
    {
        Node *N =0, *pNext =0;
        do
        {
            N = m_PartialLists[thisCpu];
            pNext = N->next;
            if (N == 0)
                return reinterpret_cast<uintptr_t>(initialiseSlab(getSlab()));
        } while (!__sync_bool_compare_and_swap(&m_PartialLists[thisCpu], N, pNext));
#if USING_MAGIC
        N->magic = 0;
#endif
        return reinterpret_cast<uintptr_t>(N);
    }
    else
    {
        return reinterpret_cast<uintptr_t>(initialiseSlab(getSlab()));
    }
}

void SlamCache::free(uintptr_t object)
{
#ifdef MULTIPROCESSOR
    size_t thisCpu = Processor::information().id();
#else
    size_t thisCpu = 0;
#endif

    Node *N = reinterpret_cast<Node*> (object);
#if OVERRUN_CHECK
    // Grab the footer and check it.
    SlamAllocator::AllocFooter *pFoot = reinterpret_cast<SlamAllocator::AllocFooter*> (object+m_ObjectSize-sizeof(SlamAllocator::AllocFooter));
    assert(pFoot->magic == VIGILANT_MAGIC);
#endif

#if USING_MAGIC
    N->magic = MAGIC_VALUE;
    N->prev = 0;
#endif
    Node *pPartialPointer =0;
    do
    {
        pPartialPointer = m_PartialLists[thisCpu];
        N->next = pPartialPointer;
    } while (!__sync_bool_compare_and_swap(&m_PartialLists[thisCpu], pPartialPointer, N));
#if USING_MAGIC
    pPartialPointer->prev = N;
#endif
}

uintptr_t SlamCache::getSlab()
{
    return reinterpret_cast<uintptr_t>(dlmallocSbrk(m_SlabSize));
}

void SlamCache::freeSlab(uintptr_t slab)
{
    /// \todo Implement.
}

SlamCache::Node *SlamCache::initialiseSlab(uintptr_t slab)
{
#ifdef MULTIPROCESSOR
    size_t thisCpu = Processor::information().id();
#else
    size_t thisCpu = 0;
#endif

    // All object in slab are free, generate Node*'s for each (except the first) and
    // link them together.
    Node *pFirst=0, *pLast=0;

    size_t nObjects = m_SlabSize/m_ObjectSize;

    for (size_t i = 1; i < nObjects; i++)
    {
        Node *pNode = reinterpret_cast<Node*> (slab + i*m_ObjectSize);
        pNode->next = reinterpret_cast<Node*> (slab + (i+1) * m_ObjectSize);
#if USING_MAGIC
        pNode->prev = (i == 1) ? 0 : reinterpret_cast<Node*> (slab + (i-1) * m_ObjectSize);
        pNode->magic = MAGIC_VALUE;
#endif

        if (i == 1)
            pFirst = pNode;
        if (i+1 >= nObjects)
            pLast = pNode;
    }

    if (pFirst)
    {
        // We now need to do two atomic updates.
        Node *pPartialPointer =0;
        do
        {
            pPartialPointer = m_PartialLists[thisCpu];
            pLast->next = pPartialPointer;
        } while(!__sync_bool_compare_and_swap(&m_PartialLists[thisCpu], pPartialPointer, pFirst));

#if USING_MAGIC
        pPartialPointer->prev = pLast;
#endif
    }
    
    return reinterpret_cast<Node*> (slab);
}

SlamAllocator::SlamAllocator() :
    m_bInitialised(false)
{
}

SlamAllocator::~SlamAllocator()
{
}

void SlamAllocator::initialise()
{
    for (size_t i =0; i < 32; i++)
        m_Caches[i].initialise(1ULL << i);
    m_bInitialised = true;
}

uintptr_t SlamAllocator::allocate(size_t nBytes)
{
#if DEBUGGING_SLAB_ALLOCATOR
    NOTICE_NOLOCK("SlabAllocator::allocate(" << Dec << nBytes << Hex << ")");
#endif

    if (!m_bInitialised)
        initialise();

    // Return value.
    uintptr_t ret = 0;

    // Add in room for the allocation footer
    nBytes += sizeof(AllocHeader) + sizeof(AllocFooter);

    // Find nearest power of 2.
    size_t powerOf2 = 1;
    size_t lg2 = 0;
    while (powerOf2 < nBytes || powerOf2 < OBJECT_MINIMUM_SIZE)
    {
        powerOf2 <<= 1;
        lg2 ++;
    }

    // Allocate 4GB and I'll kick your teeth in.
    assert(lg2 < 32);

    nBytes = powerOf2;
    assert(nBytes >= OBJECT_MINIMUM_SIZE);

    ret = m_Caches[lg2].allocate();

    if (ret)
    {
        // Shove some data on the front that we'll use later
        AllocHeader *head = reinterpret_cast<AllocHeader *>(ret);
        AllocFooter *foot = reinterpret_cast<AllocFooter *>(ret+nBytes-sizeof(AllocFooter));
        ret += sizeof(AllocHeader);

        // Set up the header
        head->cache = &m_Caches[lg2];
#if OVERRUN_CHECK
        head->magic = VIGILANT_MAGIC;
        foot->magic = VIGILANT_MAGIC;
  #if VIGILANT_OVERRUN_CHECK
        /// \todo Fill in backtrace.
  #endif
#endif  
    }

#if DEBUGGING_SLAB_ALLOCATOR
    if(!ret)
        ERROR_NOLOCK("SlabAllocator::allocate: Allocation failed (" << Dec << nBytes << Hex << " bytes)");
#else
    assert(ret != 0);
#endif

    return ret;
}

void SlamAllocator::free(uintptr_t mem)
{
#if DEBUGGING_SLAB_ALLOCATOR
    NOTICE_NOLOCK("SlabAllocator::free");
#endif

    // If we're not initialised, fix that
    if(!m_bInitialised)
        initialise();

    // Grab the header
    AllocHeader *head = reinterpret_cast<AllocHeader *>(mem - sizeof(AllocHeader));

    // If the cache is null, then the pointer is corrupted.
    assert(head->cache != 0);
#if OVERRUN_CHECK
    assert(head->magic == VIGILANT_MAGIC);
    // Footer gets checked in SlamCache::free, as we don't know the object size.
#endif

    // Free the memory
    head->cache->free(mem - sizeof(AllocHeader));
}
