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
#include <utilities/MemoryTracing.h>
#include <LockGuard.h>

#include <machine/Machine.h>
#include <processor/Processor.h>
#include <processor/VirtualAddressSpace.h>
#include <processor/PhysicalMemoryManager.h>
#include <panic.h>

#include <Backtrace.h>
#include <SlamCommand.h>

#define TEMP_MAGIC 0x67845753

SlamAllocator SlamAllocator::m_Instance;

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
        m_PartialLists[i] = 0;

    assert( (m_SlabSize % m_ObjectSize) == 0 );
}

uintptr_t SlamCache::allocate()
{
#ifdef MULTIPROCESSOR
    size_t thisCpu = Processor::id();
#else
    size_t thisCpu = 0;
#endif

    if (m_PartialLists[thisCpu] != 0)
    {
        Node *N =0, *pNext =0;
        do
        {
            N = const_cast<Node*>(m_PartialLists[thisCpu]);

            if(N && (N->next == reinterpret_cast<Node*>(VIGILANT_MAGIC)))
            {
                ERROR_NOLOCK("SlamCache::allocate hit a free block that probably wasn't free");
                m_PartialLists[thisCpu] = N = 0; // Free list is borked, start over with a new slab
            }
#if USING_MAGIC
            if(N && ((N->magic != TEMP_MAGIC) && (N->magic != MAGIC_VALUE)))
            {
                ERROR_NOLOCK("SlamCache::allocate hit a free block that probably wasn't free");
                m_PartialLists[thisCpu] = N = 0; // Free list is borked, start over with a new slab
            }
#endif

            if (N == 0)
            {
                Node *pNode = initialiseSlab(getSlab());
                uintptr_t slab = reinterpret_cast<uintptr_t>(pNode);
#if CRIPPLINGLY_VIGILANT
                if (SlamAllocator::instance().getVigilance())
                    trackSlab(slab);
#endif
                return slab;
            }

            pNext = N->next;
        } while(!__sync_bool_compare_and_swap(&m_PartialLists[thisCpu], N, pNext));
        
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
    N->prev = 0;
#endif

    Node *pPartialPointer =0;
    do
    {
        pPartialPointer = const_cast<Node*>(m_PartialLists[thisCpu]);
        N->next = pPartialPointer;
    } while(!__sync_bool_compare_and_swap(&m_PartialLists[thisCpu], pPartialPointer, N));
    
#if USING_MAGIC
    if (pPartialPointer)
        pPartialPointer->prev = N;
#endif
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
    LockGuard<Spinlock> guard(m_RecoveryLock);

#ifdef MULTIPROCESSOR
    size_t thisCpu = Processor::id();
#else
    size_t thisCpu = 0;
#endif

    volatile Node *N = m_PartialLists[thisCpu];
    if (!N)
    {
        // Nothing available for recovery!
        return 0;
    }

    size_t origMaxSlabs = maxSlabs;

    while (N && maxSlabs)
    {
        // Check for particularly large objects (ie, 1 object per slab)
        if(m_ObjectSize >= PhysicalMemoryManager::getPageSize())
        {
            Node *pNext = N->next;

            // N can be outright freed.
            if(N->magic == MAGIC_VALUE)
            {
                uintptr_t slab = reinterpret_cast<uintptr_t>(N);
                if (N->next)
                    N->next->prev = N->prev;
                if (N->prev)
                    N->prev->next = N->next;
                freeSlab(slab);

                __sync_bool_compare_and_swap(&m_PartialLists[thisCpu], N, pNext);
            }

            N = pNext;
            continue;
        }

        // Not a page or larger, we can just round down to get the slab.
        uintptr_t slab = reinterpret_cast<uintptr_t>(N) & ~(PhysicalMemoryManager::getPageSize() - 1);

        bool bSlabNotFree = false;
        for (size_t i = 0; i < (m_SlabSize / m_ObjectSize); ++i)
        {
            Node *pNode = reinterpret_cast<Node*> (slab + (i * m_ObjectSize));
            if(pNode->magic != MAGIC_VALUE)
            {
                // Not free.
                N = N->next;
                bSlabNotFree = true;
                break;
            }
        }

        if(bSlabNotFree)
            continue;

        // Find next node to iterate to (must not be on this slab!)
        Node *pNext = N->next;
        while(pNext)
        {
            uintptr_t nextSlab = reinterpret_cast<uintptr_t>(pNext) & ~(PhysicalMemoryManager::getPageSize() - 1);
            // Not on the same slab!
            if(nextSlab != slab)
            {
                break;
            }
            pNext = pNext->next;
        }

        // Adjust head of partial list, if we are the head.
        __sync_bool_compare_and_swap(&m_PartialLists[thisCpu], N, pNext);

        // Prepare for next iteration.
        N = pNext;

        // Slab free. Unlink all items from the partial list.
        for (size_t i = 0; i < (m_SlabSize / m_ObjectSize); ++i)
        {
            Node *pNode = reinterpret_cast<Node*> (slab + (i * m_ObjectSize));
            if(pNode->next)
                pNode->next->prev = pNode->prev;
            if(pNode->prev)
                pNode->prev->next = pNode->next;

            // Replace partial list head, if needed.
            __sync_bool_compare_and_swap(&m_PartialLists[thisCpu], pNode, pNext);
        }

        // Kill off the slab!
        freeSlab(slab);

        --maxSlabs;
    }

    return origMaxSlabs - maxSlabs;
}

SlamCache::Node *SlamCache::initialiseSlab(uintptr_t slab)
{
#ifdef MULTIPROCESSOR
    size_t thisCpu = Processor::id();
#else
    size_t thisCpu = 0;
#endif

    // All object in slab are free, generate Node*'s for each (except the first) and
    // link them together.
    Node *pFirst=0, *pLast=0;

    size_t nObjects = m_SlabSize/m_ObjectSize;

    for (size_t i = 1; i < nObjects; i++)
    {
        Node *pNode = reinterpret_cast<Node*> (slab + (i * m_ObjectSize));
        pNode->next = ((i + 1) >= nObjects) ? 0 : reinterpret_cast<Node*> (slab + ((i + 1) * m_ObjectSize));
#if USING_MAGIC
        pNode->prev = (i == 1) ? 0 : reinterpret_cast<Node*> (slab + ((i - 1) * m_ObjectSize));
        pNode->magic = MAGIC_VALUE;
#endif

        if (i == 1)
            pFirst = pNode;
        if ((i + 1) >= nObjects)
            pLast = pNode;
    }

    Node *N = reinterpret_cast<Node*> (slab);
#if USING_MAGIC
    N->magic = TEMP_MAGIC;
#endif

    // Link this slab in as the first in the partial list
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
    , m_SlabRegion(), m_HeapPageCount(0), m_SlabRegionLock(false)
{
    m_SlabRegion.clear();
}

SlamAllocator::~SlamAllocator()
{
}

void SlamAllocator::initialise()
{
    LockGuard<Spinlock> guard(m_SlabRegionLock);

    // We need to allocate our bitmap for this purpose.
    uintptr_t bitmapBase = VirtualAddressSpace::getKernelAddressSpace().getKernelHeapStart();
    uintptr_t heapEnd = VirtualAddressSpace::getKernelAddressSpace().getKernelHeapEnd();
    size_t heapSize = heapEnd - bitmapBase;
    size_t bitmapBytes = (heapSize / PhysicalMemoryManager::getPageSize()) / 8;

    m_SlabRegionBitmap = reinterpret_cast<uint64_t *>(bitmapBase);
    m_SlabRegionBitmapEntries = bitmapBytes / sizeof(uint64_t);

    // Ensure the bitmap size is now page-aligned before we allocate it.
    if(bitmapBytes & (PhysicalMemoryManager::getPageSize() - 1))
    {
        bitmapBytes &= ~(PhysicalMemoryManager::getPageSize() - 1);
        bitmapBytes += PhysicalMemoryManager::getPageSize();
    }

    m_Base = bitmapBase + bitmapBytes;

#ifdef KERNEL_NEEDS_ADDRESS_SPACE_SWITCH
    VirtualAddressSpace &currva = Processor::information().getVirtualAddressSpace();
    VirtualAddressSpace &va = VirtualAddressSpace::getKernelAddressSpace();
    if (Processor::m_Initialised == 2)
        Processor::switchAddressSpace(va);
#endif

    // Allocate bitmap.
    for(uintptr_t addr = bitmapBase; addr < m_Base; addr += PhysicalMemoryManager::getPageSize())
    {
        physical_uintptr_t phys = PhysicalMemoryManager::instance().allocatePage();
        va.map(phys, reinterpret_cast<void *>(addr), VirtualAddressSpace::KernelMode | VirtualAddressSpace::Write);
    }

#ifdef KERNEL_NEEDS_ADDRESS_SPACE_SWITCH
    if (Processor::m_Initialised == 2)
        Processor::switchAddressSpace(currva);
#endif

    // Good to go - wipe the bitmap and we are now configured.
    memset(m_SlabRegionBitmap, 0, bitmapBytes);

    NOTICE("Kernel heap range prepared from " << m_Base << " to " << heapEnd << ", size: " << (heapEnd - m_Base));

    for (size_t i =0; i < 32; i++)
    {
        m_Caches[i].initialise(1ULL << i);
    }

    m_bInitialised = true;
}

uintptr_t SlamAllocator::getSlab(size_t fullSize)
{
    size_t nPages = fullSize / PhysicalMemoryManager::getPageSize();
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
                size_t trailing = __builtin_ctzll(m_SlabRegionBitmap[entry]);
                if(trailing < nPages)
                {
                    size_t leading = __builtin_clzll(m_SlabRegionBitmap[entry]);
                    if(leading < nPages)
                    {
                        // No or not enough leading/trailing zeroes. Have to
                        // fall back to a more linear search.

                        size_t c = 0;
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

                        if(LIKELY(b != ~0))
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

    uintptr_t slab = m_Base + (((entry * 64) + bit) * PhysicalMemoryManager::getPageSize());

#ifdef KERNEL_NEEDS_ADDRESS_SPACE_SWITCH
    VirtualAddressSpace &currva = Processor::information().getVirtualAddressSpace();
    VirtualAddressSpace &va = VirtualAddressSpace::getKernelAddressSpace();
    if (Processor::m_Initialised == 2)
        Processor::switchAddressSpace(va);
#endif

    // Map and mark as used.
    for(size_t i = 0; i < nPages; ++i)
    {
        m_SlabRegionBitmap[entry] |= 1ULL << bit;

        // Handle crossing a bitmap entry boundary.
        if((++bit) >= 64)
        {
            ++entry;
            bit = 0;
        }

        void *p = reinterpret_cast<void *>(slab + (i * PhysicalMemoryManager::getPageSize()));
        physical_uintptr_t phys = PhysicalMemoryManager::instance().allocatePage();
        va.map(phys, p, VirtualAddressSpace::KernelMode | VirtualAddressSpace::Write);
    }

#ifdef KERNEL_NEEDS_ADDRESS_SPACE_SWITCH
    if (Processor::m_Initialised == 2)
        Processor::switchAddressSpace(currva);
#endif

    m_HeapPageCount += fullSize / PhysicalMemoryManager::getPageSize();

    return slab;
}

void SlamAllocator::freeSlab(uintptr_t address, size_t length)
{
    size_t nPages = length / PhysicalMemoryManager::getPageSize();
    if(!nPages)
    {
        panic("Attempted to free a slab smaller than the native page size.");
    }

    LockGuard<Spinlock> guard(m_SlabRegionLock);

    // Perform unmapping first (so we can just modify 'address').

#ifdef KERNEL_NEEDS_ADDRESS_SPACE_SWITCH
    VirtualAddressSpace &currva = Processor::information().getVirtualAddressSpace();
    VirtualAddressSpace &va = VirtualAddressSpace::getKernelAddressSpace();
    if (Processor::m_Initialised == 2)
        Processor::switchAddressSpace(va);
#endif

    for(uintptr_t base = address; base < (address + length); base += PhysicalMemoryManager::getPageSize())
    {
        void *p = reinterpret_cast<void *>(base);
        physical_uintptr_t phys; size_t flags;
        if(va.isMapped(p))
        {
            va.getMapping(p, phys, flags);
            va.unmap(p);

            PhysicalMemoryManager::instance().freePage(phys);
        }
    }

#ifdef KERNEL_NEEDS_ADDRESS_SPACE_SWITCH
    if (Processor::m_Initialised == 2)
        Processor::switchAddressSpace(currva);
#endif

    // Adjust bitmap.
    address -= m_Base;
    address /= PhysicalMemoryManager::getPageSize();
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

    m_HeapPageCount -= length / PhysicalMemoryManager::getPageSize();
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
        nPages += thisSlabs / m_Caches[i].slabSize();
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
#if DEBUGGING_SLAB_ALLOCATOR
    NOTICE_NOLOCK("SlabAllocator::allocate(" << Dec << nBytes << Hex << ")");
#endif
    
    if (!m_bInitialised)
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

    // Find nearest power of 2, if needed.
    size_t powerOf2 = 1;
    size_t lg2 = 0;
    while (powerOf2 < nBytes || powerOf2 < OBJECT_MINIMUM_SIZE)
    {
        powerOf2 <<= 1;
        lg2 ++;
    }

    // Allocate >2GB and I'll kick your teeth in.
    assert(lg2 < 31);

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
    return head->cache->objectSize();
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
            m_Caches[i].check();
#endif

    // Ensure this pointer is even on the heap...
    if(!Processor::information().getVirtualAddressSpace().memIsInHeap(reinterpret_cast<void*>(mem)))
        FATAL_NOLOCK("SlamAllocator::free - given pointer '" << mem << "' was completely invalid.");

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
    head->cache->free(mem - sizeof(AllocHeader));

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
    if(!m_bInitialised)
        initialise();

    // 0 is fine to free.
    if (!mem) return true;

    // On the heap?
    if(!Processor::information().getVirtualAddressSpace().memIsInHeap(reinterpret_cast<void*>(mem)))
        return false;

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
