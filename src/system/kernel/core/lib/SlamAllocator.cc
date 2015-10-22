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

#include "SlamAllocator.h"

#ifndef BENCHMARK
#include <utilities/assert.h>
#include <utilities/MemoryTracing.h>
#include <LockGuard.h>

#include <machine/Machine.h>
#include <processor/Processor.h>
#include <processor/VirtualAddressSpace.h>
#include <processor/PhysicalMemoryManager.h>
#include <panic.h>

#include <Backtrace.h>
#include <SlamCommand.h>
#endif

#ifndef BENCHMARK
SlamAllocator SlamAllocator::m_Instance;
#endif

template<typename T>
T *untagged(T *p)
{
    /// \todo this now requires 64-bit pointers everywhere.
    // All heap pointers begin with 32 bits of ones. So we shove a tag there.
    uintptr_t ptr = reinterpret_cast<uintptr_t>(p);
#ifdef BENCHMARK
    // Tags in the top 32-bits, we only operate in 32-bit address space.
    ptr &= 0xFFFFFFFFULL;
#else
    ptr |= 0xFFFFFFFF00000000ULL;
#endif
    return reinterpret_cast<T *>(ptr);
}

template<typename T>
T *tagged(T *p)
{
    uintptr_t ptr = reinterpret_cast<uintptr_t>(p);
    ptr &= 0xFFFFFFFFULL;
    return reinterpret_cast<T *>(ptr);
}

template<typename T>
T *touch_tag(T *p)
{
    // Add one to the tag.
    uintptr_t ptr = reinterpret_cast<uintptr_t>(p);
    ptr += 0x100000000ULL;
    return reinterpret_cast<T *>(ptr);
}

#ifdef BENCHMARK
#define TEST_MEMORY_AREA_SIZE 0x40000000
#endif

inline uintptr_t getHeapBase()
{
#ifdef BENCHMARK
    return 0x10000000ULL;
#else
    return VirtualAddressSpace::getKernelAddressSpace().getKernelHeapStart();
#endif
}

inline uintptr_t getHeapEnd()
{
#ifdef BENCHMARK
    return getHeapBase() + TEST_MEMORY_AREA_SIZE;
#else
    return VirtualAddressSpace::getKernelAddressSpace().getKernelHeapEnd();
#endif
}

inline size_t getPageSize()
{
#ifdef BENCHMARK
    return 0x1000;
#else
    return PhysicalMemoryManager::getPageSize();
#endif
}

inline void allocateAndMapAt(void *addr)
{
#ifdef BENCHMARK
    void *r = mmap(addr, getPageSize(), PROT_READ | PROT_WRITE, MAP_PRIVATE |
        MAP_FIXED | MAP_ANONYMOUS, -1, 0);
    if (r == MAP_FAILED)
    {
        fprintf(stderr, "map failed: %s\n", strerror(errno));
        exit(1);
    }
#else
    VirtualAddressSpace &va = VirtualAddressSpace::getKernelAddressSpace();
    physical_uintptr_t phys = PhysicalMemoryManager::instance().allocatePage();
    va.map(phys, addr, VirtualAddressSpace::KernelMode | VirtualAddressSpace::Write);
#endif
}

inline void unmap(void *addr)
{
#ifdef BENCHMARK
    munmap(addr, getPageSize());
#else
    VirtualAddressSpace &va = VirtualAddressSpace::getKernelAddressSpace();
    if (!va.isMapped(addr))
        return;

    physical_uintptr_t phys; size_t flags;
    va.getMapping(addr, phys, flags);
    va.unmap(addr);

    PhysicalMemoryManager::instance().freePage(phys);
#endif
}

SlamCache::SlamCache() :
    m_ObjectSize(0), m_SlabSize(0)
#if CRIPPLINGLY_VIGILANT
    ,m_FirstSlab()
#endif
    , m_RecoveryLock(false)
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
        m_PartialLists[i] = tagged(&m_EmptyNode);

    // Make the empty node loop always, so it can be easily linked into place.
    memset(&m_EmptyNode, 0xAB, sizeof(m_EmptyNode));
    m_EmptyNode.next = tagged(&m_EmptyNode);

    assert( (m_SlabSize % m_ObjectSize) == 0 );
}

uintptr_t SlamCache::allocate()
{
#ifdef MULTIPROCESSOR
    size_t thisCpu = Processor::id();
#else
    size_t thisCpu = 0;
#endif

    Node *N = 0, *pNext = 0;
    partialListType currentHead = __atomic_load_n(&m_PartialLists[thisCpu], __ATOMIC_ACQUIRE);
    do
    {
        pNext = 0;

        // Grab the next pointer after the head.
        N = untagged(currentHead);
        if (N == m_pHazard)
            continue;

        pNext = __atomic_load_n(&N->next, __ATOMIC_RELAXED);
        if (UNLIKELY(pNext != untagged(m_PartialLists[thisCpu])->next))
            continue;

        // A thread that gets a specific N currently continues on, using it and
        // trashing it. This is OK when one thread is accessing the allocator,
        // as there's no way for the same thread to be in here twice.
        // Once we have two or more threads, it's possible that one of them will
        // see N change underneath it, because it managed to read the head just
        // before the other thread replaced it.

        // So... what to do, what to do?

        // FWIW, the ABA problem is actually solved now, because the use of
        // tagged pointers has made it impossible. So we're just left with the
        // segfault problem, which is particularly nasty.
    } while(!__atomic_compare_exchange_n(&m_PartialLists[thisCpu], &currentHead,
        touch_tag(pNext), false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED));

    // Something else got there first if N == 0. Just allocate a new slab.
    if (UNLIKELY(N == &m_EmptyNode))
    {
        Node *pNode = initialiseSlab(getSlab());
        uintptr_t slab = reinterpret_cast<uintptr_t>(pNode);
#if CRIPPLINGLY_VIGILANT
        if (SlamAllocator::instance().getVigilance())
            trackSlab(slab);
#endif
        return slab;
    }

    // Check that the block was indeed free.
    assert(N->next != reinterpret_cast<Node *>(VIGILANT_MAGIC));
#if USING_MAGIC
    assert(N->magic == TEMP_MAGIC || N->magic == MAGIC_VALUE);
    N->magic = TEMP_MAGIC;
#endif

    return reinterpret_cast<uintptr_t>(N);
}

void SlamCache::free(uintptr_t object)
{
#ifdef MULTIPROCESSOR
    size_t thisCpu = Processor::id();
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
#endif

    Node *pPartialPointer = 0;
    partialListType currentHead = 0;
    N->next = m_PartialLists[thisCpu];
    while(!__atomic_compare_exchange_n(&m_PartialLists[thisCpu], &N->next, touch_tag(N), false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED));
}

bool SlamCache::isPointerValid(uintptr_t object)
{
    Node *N = reinterpret_cast<Node*> (object);
#if OVERRUN_CHECK
    // Grab the footer and check it.
    SlamAllocator::AllocFooter *pFoot = reinterpret_cast<SlamAllocator::AllocFooter*> (object+m_ObjectSize-sizeof(SlamAllocator::AllocFooter));
    if (pFoot->magic != VIGILANT_MAGIC)
    {
        return false;
    }
#endif

#if USING_MAGIC
    // Possible double free?
    if (N->magic == MAGIC_VALUE)
    {
        return false;
    }
#endif

    return true;
}

uintptr_t SlamCache::getSlab()
{
    return SlamAllocator::instance().getSlab(m_SlabSize);
}

void SlamCache::freeSlab(uintptr_t slab)
{
    SlamAllocator::instance().freeSlab(slab, m_SlabSize);
}

size_t SlamCache::recovery(size_t maxSlabs)
{
#ifdef MULTIPROCESSOR
    size_t thisCpu = Processor::id();
#else
    size_t thisCpu = 0;
#endif

    /// \todo implement tagging here

    if(!m_PartialLists[thisCpu])
        return 0;

    size_t freedSlabs = 0;
    if(m_ObjectSize < getPageSize())
    {
        Node *reinsertHead = 0;
        Node *reinsertTail = 0;
        while(maxSlabs--)
        {
            // Grab the head node of the free list.
            Node *N =0, *pNext =0;
            do
            {
                pNext = 0;
                N = m_PartialLists[thisCpu];
                if(N)
                    pNext = N->next;
            } while(!__sync_bool_compare_and_swap(&m_PartialLists[thisCpu], N, pNext));

            // If no head node, we're done with this free list.
            if(N == 0)
            {
                if (!reinsertTail)
                {
                    // No reinsert tail. Bail.
                    break;
                }

                // Okay, we've emptied the partial lists.
                // Let's link in our reinsert nodes.
                Node *pHead = 0;
                do
                {
                    // Link our tail to the current head, replace head.
                    pHead = m_PartialLists[thisCpu];
                    reinsertTail->next = pHead;
                } while(!__sync_bool_compare_and_swap(&m_PartialLists[thisCpu], pHead, reinsertHead));

                break;
            }

            uintptr_t slab = reinterpret_cast<uintptr_t>(N) & ~(getPageSize() - 1);

            // A possible node found! Any luck?
            bool bSlabNotFree = false;
            for (size_t i = 0; i < (m_SlabSize / m_ObjectSize); ++i)
            {
                Node *pNode = reinterpret_cast<Node*> (slab + (i * m_ObjectSize));
                SlamAllocator::AllocHeader *pHeader = reinterpret_cast<SlamAllocator::AllocHeader *>(pNode);
                if (pHeader->cache == this)
                {
                    // Oops, an active allocation was found.
                    bSlabNotFree = true;
                }
#if USING_MAGIC
                if(pNode->magic != MAGIC_VALUE)
                {
                    // Not free.
                    bSlabNotFree = true;
                    break;
                }
#endif
            }

            if(bSlabNotFree)
            {
                // Link the node into our reinsert lists, as the slab cannot
                // be freed quite so easily.
                if(!reinsertHead)
                {
                    reinsertHead = reinsertTail = N;
                    N->next = 0;
                }
                else
                {
                    N->next = reinsertHead;
                    reinsertHead = N;
                }

                continue;
            }

            // Slab free. Unlink all items and remove.
            for (size_t i = 0; i < (m_SlabSize / m_ObjectSize); ++i)
            {
                Node *pNode = reinterpret_cast<Node*> (slab + (i * m_ObjectSize));
                Node *pNodeNext = pNode->next;

                // If this node became the partial list head, make sure it is
                // no longer the head.
                __sync_val_compare_and_swap(&m_PartialLists[thisCpu], pNode, pNodeNext);
            }

            // Kill off the slab!
            freeSlab(slab);
            ++freedSlabs;
        }
    }
    else
    {
        while(maxSlabs--)
        {
            if(!m_PartialLists[thisCpu])
                break;

            Node *N =0, *pNext =0;
            do
            {
                pNext = 0;
                N = m_PartialLists[thisCpu];
                if(N)
                    pNext = N->next;
            } while(!__sync_bool_compare_and_swap(&m_PartialLists[thisCpu], N, pNext));

            if(N == 0)
            {
                // Emptied the partial list!
                break;
            }

#if USING_MAGIC
            assert(N->magic == MAGIC_VALUE);
#endif

            uintptr_t slab = reinterpret_cast<uintptr_t>(N);

            freeSlab(slab);
            ++freedSlabs;
        }
    }

    return freedSlabs;
}

SlamCache::Node *SlamCache::initialiseSlab(uintptr_t slab)
{
#ifdef MULTIPROCESSOR
    size_t thisCpu = Processor::id();
#else
    size_t thisCpu = 0;
#endif

    size_t nObjects = m_SlabSize / m_ObjectSize;

    Node *N = reinterpret_cast<Node*> (slab);
#if USING_MAGIC
    N->magic = TEMP_MAGIC;
#endif

    // Early exit if there's no other free objects in this slab.
    if (nObjects <= 1)
        return N;

    // All objects in slab are free, generate Node*'s for each (except the
    // first) and link them together.
    Node *pFirst=0, *pLast=0;
    for (size_t i = 1; i < nObjects; i++)
    {
        Node *pNode = reinterpret_cast<Node*> (slab + (i * m_ObjectSize));
        pNode->next = ((i + 1) >= nObjects) ? 0 : reinterpret_cast<Node*> (slab + ((i + 1) * m_ObjectSize));
        pNode->next = tagged(pNode->next);
#if USING_MAGIC
        pNode->magic = MAGIC_VALUE;
#endif

        if (i == 1)
            pFirst = tagged(pNode);
        if ((i + 1) >= nObjects)
            pLast = tagged(pNode);
    }

    // Link this slab in as the first in the partial list
    if (pFirst)
    {
        // We now need to do two atomic updates.
        Node *pPartialPointer =0;
        partialListType currentHead = 0;
        pLast->next = m_PartialLists[thisCpu];
        while(!__atomic_compare_exchange_n(&m_PartialLists[thisCpu], &pLast->next, touch_tag(pFirst), false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED));
    }

    return N;
}

#if CRIPPLINGLY_VIGILANT
Spinlock rarp;
void SlamCache::check()
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
    , m_SlabRegionLock(false), m_HeapPageCount(0)
{
}

SlamAllocator::~SlamAllocator()
{
}

void SlamAllocator::initialise()
{
    LockGuard<Spinlock> guard(m_SlabRegionLock);

    // We need to allocate our bitmap for this purpose.
    uintptr_t bitmapBase = getHeapBase();
    uintptr_t heapEnd = getHeapEnd();
    size_t heapSize = heapEnd - bitmapBase;
    size_t bitmapBytes = (heapSize / getPageSize()) / 8;

    m_SlabRegionBitmap = reinterpret_cast<uint64_t *>(bitmapBase);
    m_SlabRegionBitmapEntries = bitmapBytes / sizeof(uint64_t);

    // Ensure the bitmap size is now page-aligned before we allocate it.
    if(bitmapBytes & (getPageSize() - 1))
    {
        bitmapBytes &= ~(getPageSize() - 1);
        bitmapBytes += getPageSize();
    }

    m_Base = bitmapBase + bitmapBytes;

#ifdef KERNEL_NEEDS_ADDRESS_SPACE_SWITCH
    VirtualAddressSpace &va = VirtualAddressSpace::getKernelAddressSpace();
    VirtualAddressSpace &currva = Processor::information().getVirtualAddressSpace();
    if (Processor::m_Initialised == 2)
        Processor::switchAddressSpace(va);
#endif

    // Allocate bitmap.
    for(uintptr_t addr = bitmapBase; addr < m_Base; addr += getPageSize())
    {
        allocateAndMapAt(reinterpret_cast<void *>(addr));
    }

#ifdef KERNEL_NEEDS_ADDRESS_SPACE_SWITCH
    if (Processor::m_Initialised == 2)
        Processor::switchAddressSpace(currva);
#endif

    // Good to go - wipe the bitmap and we are now configured.
    memset(m_SlabRegionBitmap, 0, bitmapBytes);

    NOTICE("Kernel heap range prepared from " << Hex << m_Base << " to " << heapEnd << ", size: " << (heapEnd - m_Base));
    DEBUG_LOG("  -> kernel heap bitmap is " << Dec << (bitmapBytes / 1024) << Hex << "K");

    for (size_t i =0; i < 32; i++)
    {
        m_Caches[i].initialise(1ULL << i);
    }

    m_bInitialised = true;
}

uintptr_t SlamAllocator::getSlab(size_t fullSize)
{

    ssize_t nPages = fullSize / getPageSize();
    if(!nPages)
    {
        panic("Attempted to get a slab smaller than the native page size.");
    }

    LockGuard<Spinlock> guard(m_SlabRegionLock);

    // Try to find space for this allocation.
    size_t entry = 0;
    size_t bit = ~0UL;
    if(nPages == 1)
    {
        // Fantastic - easy search.
        for(entry = 0; entry < m_SlabRegionBitmapEntries; ++entry)
        {
            if(!m_SlabRegionBitmap[entry])
            {
                bit = 0;
                break;
            }
            else if(m_SlabRegionBitmap[entry] != 0xFFFFFFFFFFFFFFFFULL)
            {
                // First set of the INVERTED entry will be the first zero bit.
                // Note - the check for this block ensures we always get a
                // result from ffsll here.
                bit =  __builtin_ffsll(~m_SlabRegionBitmap[entry]) - 1;
                break;
            }
        }
    }
    else if(nPages > 64)
    {
        // This allocation does not fit within a single bitmap entry.
        for(entry = 0; entry < m_SlabRegionBitmapEntries; ++entry)
        {
            // If there are any bits set in this entry, we must disregard it.
            if(m_SlabRegionBitmap[entry])
                continue;

            // This entry has 64 free pages. Now we need to see if we can get
            // contiguously free bitmap entries.
            size_t needed = nPages - 64;
            size_t checkEntry = entry + 1;
            while(needed >= 64)
            {
                // If the entry has any set bits whatsoever, it's no good.
                if(m_SlabRegionBitmap[checkEntry])
                    break;

                // Success.
                ++checkEntry;
                needed -= 64;
            }

            // Check for the ideal case.
            if(needed == 0)
            {
                bit = 0;
                break;
            }
            else if(needed < 64)
            {
                // Possible! Can we get enough trailing zeroes in the next entry to
                // make this work?
                size_t leading = __builtin_ctzll(m_SlabRegionBitmap[checkEntry]);
                if(leading >= needed)
                {
                    bit = 0;
                    break;
                }
            }

            // Skip already-checked entries.
            entry = checkEntry;
        }
    }
    else
    {
        // Have to search within entries.
        for(entry = 0; entry < m_SlabRegionBitmapEntries; ++entry)
        {
            if(!m_SlabRegionBitmap[entry])
            {
                bit = 0;
                break;
            }
            else if(__builtin_popcountll(~m_SlabRegionBitmap[entry]) >= nPages)
            {
                // Need to find a sequence of bits.
                // Try for the beginning or end of the entry, as these are builtins.
                ssize_t trailing = __builtin_ctzll(m_SlabRegionBitmap[entry]);
                if(trailing < nPages)
                {
                    ssize_t leading = __builtin_clzll(m_SlabRegionBitmap[entry]);
                    if(leading < nPages)
                    {
                        // No or not enough leading/trailing zeroes. Have to
                        // fall back to a more linear search.

                        ssize_t c = 0;
                        size_t b = ~0UL;
                        size_t len = 64;
                        uint64_t v = m_SlabRegionBitmap[entry];

                        // Try and reduce the number of bits we need to scan.
                        if((v & 0xFFFFFFFFULL) == 0xFFFFFFFFULL)
                        {
                            len -= 32;
                            v >>= 32;
                        }
                        if((v & 0xFFFFULL) == 0xFFFFULL)
                        {
                            len -= 16;
                            v >>= 16;
                        }
                        if((v & 0xFFULL) == 0xFFULL)
                        {
                            len -= 8;
                            v >>= 8;
                        }
                        if((v & 0xFULL) == 0xFULL)
                        {
                            len -= 4;
                            v >>= 4;
                        }

                        while(len--)
                        {
                            if(v & 1)
                            {
                                c = 0;
                                b = ~0;
                            }
                            else
                            {
                                // Bits count from zero.
                                if(b == ~0UL)
                                    b = 63 - len;

                                if(++c >= nPages)
                                    break;
                            }

                            v >>= 1;
                        }

                        if(LIKELY(b != ~0UL))
                        {
                            bit = b;
                            break;
                        }
                    }
                    else
                    {
                        bit = 64 - leading;
                        break;
                    }
                }
                else
                {
                    bit = 0;
                    break;
                }
            }
        }
    }

    if(bit == ~0UL)
    {
        FATAL("SlamAllocator::getSlab cannot find a place to allocate this slab (" << Dec << fullSize << Hex << " bytes)!");
    }

    uintptr_t slab = m_Base + (((entry * 64) + bit) * getPageSize());

#ifdef KERNEL_NEEDS_ADDRESS_SPACE_SWITCH
    VirtualAddressSpace &va = VirtualAddressSpace::getKernelAddressSpace();
    VirtualAddressSpace &currva = Processor::information().getVirtualAddressSpace();
    if (Processor::m_Initialised == 2)
        Processor::switchAddressSpace(va);
#endif

    // Map and mark as used.
    for(ssize_t i = 0; i < nPages; ++i)
    {
        m_SlabRegionBitmap[entry] |= 1ULL << bit;

        // Handle crossing a bitmap entry boundary.
        if((++bit) >= 64)
        {
            ++entry;
            bit = 0;
        }

        void *p = reinterpret_cast<void *>(slab + (i * getPageSize()));
        allocateAndMapAt(p);
    }

#ifdef KERNEL_NEEDS_ADDRESS_SPACE_SWITCH
    if (Processor::m_Initialised == 2)
        Processor::switchAddressSpace(currva);
#endif

    m_HeapPageCount += fullSize / getPageSize();

    return slab;
}

void SlamAllocator::freeSlab(uintptr_t address, size_t length)
{
    size_t nPages = length / getPageSize();
    if(!nPages)
    {
        panic("Attempted to free a slab smaller than the native page size.");
    }

    LockGuard<Spinlock> guard(m_SlabRegionLock);

    // Perform unmapping first (so we can just modify 'address').

#ifdef KERNEL_NEEDS_ADDRESS_SPACE_SWITCH
    VirtualAddressSpace &va = VirtualAddressSpace::getKernelAddressSpace();
    VirtualAddressSpace &currva = Processor::information().getVirtualAddressSpace();
    if (Processor::m_Initialised == 2)
        Processor::switchAddressSpace(va);
#endif

    for(uintptr_t base = address; base < (address + length); base += getPageSize())
    {
        void *p = reinterpret_cast<void *>(base);
        unmap(p);
    }

#ifdef KERNEL_NEEDS_ADDRESS_SPACE_SWITCH
    if (Processor::m_Initialised == 2)
        Processor::switchAddressSpace(currva);
#endif

    // Adjust bitmap.
    address -= m_Base;
    address /= getPageSize();
    size_t entry = address / 64;
    size_t bit = address % 64;

    for(size_t i = 0; i < nPages; ++i)
    {
        m_SlabRegionBitmap[entry] &= ~(1ULL << bit);

        // Handle overflow (eg, if we cross a bitmap entry.)
        if((++bit) >= 64)
        {
            ++entry;
            bit = 0;
        }
    }

    m_HeapPageCount -= length / getPageSize();
}

size_t SlamAllocator::recovery(size_t maxSlabs)
{
    size_t nSlabs = 0;
    size_t nPages = 0;

    for (size_t i = 0; i < 32; ++i)
    {
        // Things without slabs don't get recovered.
        if(!m_Caches[i].slabSize())
            continue;

        size_t thisSlabs = m_Caches[i].recovery(maxSlabs);
        nPages += (thisSlabs * m_Caches[i].slabSize()) / getPageSize();
        nSlabs += thisSlabs;
        if(nSlabs >= maxSlabs)
        {
            break;
        }
    }

    return nPages;
}

uintptr_t SlamAllocator::allocate(size_t nBytes)
{
  nBytes = nBytes < 0x1000 ? 0x1000 : nBytes;
#if DEBUGGING_SLAB_ALLOCATOR
    NOTICE_NOLOCK("SlabAllocator::allocate(" << Dec << nBytes << Hex << ")");
#endif

    if (UNLIKELY(!m_bInitialised))
        initialise();

#if CRIPPLINGLY_VIGILANT
    if (m_bVigilant)
        for (int i = 0; i < 32; i++)
            m_Caches[i].check();
#endif

#ifdef MEMORY_TRACING
    size_t origSize = nBytes;
#endif

    // Return value.
    uintptr_t ret = 0;

    // Add in room for the allocation footer
    nBytes += sizeof(AllocHeader) + sizeof(AllocFooter);

    // Don't allow huge allocations.
    /// \note Even 2G is a stretch on most systems. Use some other allocator
    ///       to allocate such large buffers.
    assert(nBytes < (1U << 31));

    size_t orig = nBytes;

    // Default to minimum object size if we must.
    size_t lg2 = 0;
    if (LIKELY(nBytes < OBJECT_MINIMUM_SIZE))
    {
        nBytes = OBJECT_MINIMUM_SIZE;
    }
    else if ((nBytes & (nBytes - 1)) != 0)
    {
        // Not already a power of two, so we need to find the next highest.
        // Easy - number of leading zeroes indicates the highest set bit. Then,
        // we just need to turn that into a left shift. Ta-da, next
        // power-of-two.
        nBytes = 1U << (32 - __builtin_clz(nBytes));
    }

    // nBytes is now guaranteed to be a power-of-two.
    lg2 = __builtin_ffs(nBytes) - 1;
    assert((1U << lg2) == nBytes);

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
        if (Processor::m_Initialised == 2)
        {
            Backtrace bt;
            bt.performBpBacktrace(0, 0);
            memcpy(&head->backtrace, bt.m_pReturnAddresses, NUM_SLAM_BT_FRAMES*sizeof(uintptr_t));
            head->requested = nBytes;
            g_SlamCommand.addAllocation(head->backtrace, head->requested);
        }
    #endif
#endif
    }

#if DEBUGGING_SLAB_ALLOCATOR
    if(!ret)
        ERROR_NOLOCK("SlabAllocator::allocate: Allocation failed (" << Dec << nBytes << Hex << " bytes)");
#else
    assert(ret != 0);
#endif

#ifdef MEMORY_TRACING
    traceAllocation(reinterpret_cast<void *>(ret), MemoryTracing::Allocation, origSize);
#endif

    return ret;
}

size_t SlamAllocator::allocSize(uintptr_t mem)
{
    if(!mem)
        return 0;
    
    // Grab the header
    AllocHeader *head = reinterpret_cast<AllocHeader *>(mem - sizeof(AllocHeader));

    // If the cache is null, then the pointer is corrupted.
    assert(head->cache != 0);
    size_t result = head->cache->objectSize();

    // Remove size of header/footer.
    // This is important as we're returning the size of each object itself,
    // but we return memory framed by headers and footers. So, the "true" size
    // of memory pointed to by 'mem' is not the true object size.
    return result - (sizeof(AllocHeader) + sizeof(AllocFooter));
}

void SlamAllocator::free(uintptr_t mem)
{
#if DEBUGGING_SLAB_ALLOCATOR
    NOTICE_NOLOCK("SlabAllocator::free");
#endif
    
    // If we're not initialised, fix that
    if(UNLIKELY(!m_bInitialised))
        initialise();

#if CRIPPLINGLY_VIGILANT
    if (m_bVigilant)
        for (int i = 0; i < 32; i++)
            m_Caches[i].check();
#endif

    // Ensure this pointer is even on the heap...
#ifndef BENCHMARK
    if(!Processor::information().getVirtualAddressSpace().memIsInHeap(reinterpret_cast<void*>(mem)))
        FATAL_NOLOCK("SlamAllocator::free - given pointer '" << mem << "' was completely invalid.");
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
    #if VIGILANT_OVERRUN_CHECK
        if (Processor::m_Initialised == 2)
            g_SlamCommand.removeAllocation(head->backtrace, head->requested);
    #endif
#endif

    // Free the memory
    SlamCache *pCache = head->cache;
    head->cache = 0;  // Wipe out the cache - freed page.
    pCache->free(mem - sizeof(AllocHeader));

#ifdef MEMORY_TRACING
   traceAllocation(reinterpret_cast<void *>(mem), MemoryTracing::Free, 0);
#endif
}

bool SlamAllocator::isPointerValid(uintptr_t mem)
{
#if DEBUGGING_SLAB_ALLOCATOR
    NOTICE_NOLOCK("SlabAllocator::isPointerValid");
#endif

    // If we're not initialised, fix that
    if(UNLIKELY(!m_bInitialised))
        initialise();

    // 0 is fine to free.
    if (!mem) return true;

    // On the heap?
#ifndef BENCHMARK
    if(!Processor::information().getVirtualAddressSpace().memIsInHeap(reinterpret_cast<void*>(mem)))
        return false;
#endif

#if CRIPPLINGLY_VIGILANT
    if (m_bVigilant)
        for (int i = 0; i < 32; i++)
            m_Caches[i].check();
#endif

    // Grab the header
    AllocHeader *head = reinterpret_cast<AllocHeader *>(mem - sizeof(AllocHeader));

#if OVERRUN_CHECK
    if (head->magic != VIGILANT_MAGIC)
    {
        return false;
    }
    // Footer gets checked in SlamCache::free, as we don't know the object size.
#endif

    // If the cache is null, then the pointer is corrupted.
    if (head->cache == 0)
    {
        return false;
    }

    // Check for a valid cache
    bool bValid = false;
    for (int i = 0; i < 32; i++)
    {
        if(head->cache == &m_Caches[i])
        {
            bValid = true;
            break;
        }
    }
    
    if(!bValid)
    {
        WARNING_NOLOCK("SlamAllocator::isPointerValid - cache pointer '" << reinterpret_cast<uintptr_t>(head->cache) << "' is invalid.");
        return false;
    }

    // Free the memory
    head->cache->isPointerValid(mem - sizeof(AllocHeader));
    return true;
}

bool _assert_ptr_valid(uintptr_t ptr)
{
    return SlamAllocator::instance().isPointerValid(ptr);
}
