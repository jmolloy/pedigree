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

#include <machine/Machine.h>
#include <processor/Processor.h>
#include <panic.h>

#define TEMP_MAGIC 0x67845753

SlamAllocator SlamAllocator::m_Instance;

SlamCache::SlamCache() :
    m_ObjectSize(0), m_SlabSize(0)
#if CRIPPLINGLY_VIGILANT
    ,m_FirstSlab()
#endif
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
            N = const_cast<Node*>(m_PartialLists[thisCpu]);
            pNext = N->next;

            if (N == 0)
            {
                uintptr_t slab = reinterpret_cast<uintptr_t>(initialiseSlab(getSlab()));
#if CRIPPLINGLY_VIGILANT
                if (SlamAllocator::instance().getVigilance())
                    trackSlab(slab);
#endif
                return slab;
            }
        } while (!__sync_bool_compare_and_swap(&m_PartialLists[thisCpu], N, pNext));
#if USING_MAGIC
        N->magic = TEMP_MAGIC;
#endif

        return reinterpret_cast<uintptr_t>(N);
    }
    else
    {
        uintptr_t slab = reinterpret_cast<uintptr_t>(initialiseSlab(getSlab()));
#if CRIPPLINGLY_VIGILANT
        if (SlamAllocator::instance().getVigilance())
            trackSlab(slab);
#endif
        return slab;
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

#if BOCHS_MAGIC_WATCHPOINTS
    asm volatile("xchg %%dx,%%dx" :: "a" (&pFoot->catcher));
#endif
#endif

#if USING_MAGIC
    // Possible double free?
    assert(N->magic != MAGIC_VALUE);
    N->magic = MAGIC_VALUE;
    N->prev = 0;
#endif
    Node *pPartialPointer =0;
    do
    {
        pPartialPointer = const_cast<Node*>(m_PartialLists[thisCpu]);
        N->next = pPartialPointer;
    } while (!__sync_bool_compare_and_swap(&m_PartialLists[thisCpu], pPartialPointer, N));

#if USING_MAGIC
    if (pPartialPointer)
        pPartialPointer->prev = N;
#endif
}

Spinlock s;
uintptr_t SlamCache::getSlab()
{
    s.acquire();
    uintptr_t ret = reinterpret_cast<uintptr_t>(dlmallocSbrk(m_SlabSize));
    s.release();
    return ret;
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

    Node *N = reinterpret_cast<Node*> (slab);
    N->magic = TEMP_MAGIC;

    if (pFirst)
    {
        // We now need to do two atomic updates.
        Node *pPartialPointer =0;
        do
        {
            pPartialPointer = const_cast<Node*>(m_PartialLists[thisCpu]);
            pLast->next = pPartialPointer;
        } while(!__sync_bool_compare_and_swap(&m_PartialLists[thisCpu], pPartialPointer, pFirst));

#if USING_MAGIC
        if (pPartialPointer)
            pPartialPointer->prev = pLast;
#endif
    }

    return N;
}

#if CRIPPLINGLY_VIGILANT
Spinlock rarp;
void SlamCache::check(int x)
{
    if (!Machine::instance().isInitialised() || Processor::m_Initialised != 2)
        return;
    if (m_ObjectSize == 0)
        return;
    rarp.acquire();

    size_t nObjects = m_SlabSize/m_ObjectSize;

    size_t maxPerSlab = (m_SlabSize / sizeof(uintptr_t))-2;

    uintptr_t curSlab = m_FirstSlab;
    while (true)
    {
        if (!curSlab)
        {
            rarp.release();
            return;
        }
        uintptr_t numAlloced = *reinterpret_cast<uintptr_t*> (curSlab);
        uintptr_t next = *reinterpret_cast<uintptr_t*> (curSlab+sizeof(uintptr_t));

        for (size_t i = 0; i < numAlloced; i++)
        {
            uintptr_t slab = *reinterpret_cast<uintptr_t*> (curSlab+sizeof(uintptr_t)*(i+2));
            for (size_t i = 0; i < nObjects; i++)
            {
                uintptr_t addr = slab + i*m_ObjectSize;
                Node *pNode = reinterpret_cast<Node*>(addr);
                if (pNode->magic == MAGIC_VALUE || pNode->magic == TEMP_MAGIC)
                    // Free, continue.
                    continue;
                SlamAllocator::AllocHeader *pHead = reinterpret_cast
                    <SlamAllocator::AllocHeader*> (addr);
                SlamAllocator::AllocFooter *pFoot = reinterpret_cast
                    <SlamAllocator::AllocFooter*> (addr+m_ObjectSize-
                                                   sizeof(SlamAllocator::AllocFooter));
                if (pHead->magic != VIGILANT_MAGIC)
                {
                    ERROR("Possible heap underrun: object starts at " << addr << ", size: " << m_ObjectSize << ", block: " << (addr+sizeof(SlamAllocator::AllocHeader)));
                }
                if (pFoot->magic != VIGILANT_MAGIC)
                {
                    ERROR("Possible heap overrun: object starts at " << addr);
                    assert(false);
                }
            }
        }
        if (numAlloced == maxPerSlab)
            curSlab = next;
        else
            break;
    }
    rarp.release();
}

void SlamCache::trackSlab(uintptr_t slab)
{
    if (!Machine::instance().isInitialised() || Processor::m_Initialised != 2)
        return;
    if (m_ObjectSize == 0)
        return;

    if (!m_FirstSlab)
    {
        m_FirstSlab = getSlab();
        uintptr_t *numAlloced = reinterpret_cast<uintptr_t*> (m_FirstSlab);
        uintptr_t *next = reinterpret_cast<uintptr_t*> (m_FirstSlab+sizeof(uintptr_t));
        *numAlloced = 0;
        *next = 0;
    }

    size_t maxPerSlab = (m_SlabSize / sizeof(uintptr_t))-2;

    uintptr_t curSlab = m_FirstSlab;
    while (true)
    {
        uintptr_t *numAlloced = reinterpret_cast<uintptr_t*> (curSlab);
        uintptr_t *next = reinterpret_cast<uintptr_t*> (curSlab+sizeof(uintptr_t));

        if (*numAlloced < maxPerSlab)
        {
            uintptr_t *p = reinterpret_cast<uintptr_t*> (curSlab + (*numAlloced + 2) * sizeof(uintptr_t));
            *p = slab;
            *numAlloced = *numAlloced+1;
            return;
        }

        if (*next)
            curSlab = *next;
        else
        {
            uintptr_t newSlab = getSlab();
            *next = newSlab;
            curSlab = newSlab;

            uintptr_t *numAlloced = reinterpret_cast<uintptr_t*> (curSlab);
            uintptr_t *next = reinterpret_cast<uintptr_t*> (curSlab+sizeof(uintptr_t));
            *numAlloced = 0;
            *next = 0;
        }
    }
}
#endif

SlamAllocator::SlamAllocator() :
    m_bInitialised(false)
#if CRIPPLINGLY_VIGILANT
    , m_bVigilant(false)
#endif
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

#if CRIPPLINGLY_VIGILANT
    if (m_bVigilant)
        for (int i = 0; i < 32; i++)
            m_Caches[i].check(0);
#endif

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
    assert(lg2 < 24);

    nBytes = powerOf2;
    assert(nBytes >= OBJECT_MINIMUM_SIZE);

    ret = m_Caches[lg2].allocate();

    //   l.release();
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


#if BOCHS_MAGIC_WATCHPOINTS
        /// \todo head->catcher should be used for underrun checking
        // asm volatile("xchg %%cx,%%cx" :: "a" (&head->catcher));
        asm volatile("xchg %%cx,%%cx" :: "a" (&foot->catcher));
#endif
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

#if CRIPPLINGLY_VIGILANT
    if (m_bVigilant)
        for (int i = 0; i < 32; i++)
            m_Caches[i].check(1);
#endif

    // Grab the header
    AllocHeader *head = reinterpret_cast<AllocHeader *>(mem - sizeof(AllocHeader));

    // If the cache is null, then the pointer is corrupted.
    assert(head->cache != 0);
#if OVERRUN_CHECK
    assert(head->magic == VIGILANT_MAGIC);
    // Footer gets checked in SlamCache::free, as we don't know the object size.

#if BOCHS_MAGIC_WATCHPOINTS
        /// \todo head->catcher should be used for underrun checking
        // asm volatile("xchg %%dx,%%dx" :: "a" (&head->catcher));
#endif
#endif

    // Free the memory
    head->cache->free(mem - sizeof(AllocHeader));
}
