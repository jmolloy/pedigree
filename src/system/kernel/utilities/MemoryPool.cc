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

#include <processor/Processor.h>
#include <utilities/MemoryPool.h>
#include <processor/VirtualAddressSpace.h>
#include <LockGuard.h>
#include <Log.h>

static void map(uintptr_t location)
{
    VirtualAddressSpace &va = VirtualAddressSpace::getKernelAddressSpace();
#ifdef KERNEL_NEEDS_ADDRESS_SPACE_SWITCH
    VirtualAddressSpace &currva = Processor::information().getVirtualAddressSpace();
    Processor::switchAddressSpace(va);
#endif

    void *page = page_align(reinterpret_cast<void *>(location));
    if (!va.isMapped(page))
    {
        physical_uintptr_t phys = PhysicalMemoryManager::instance().allocatePage();
        va.map(phys, page, VirtualAddressSpace::KernelMode | VirtualAddressSpace::Write);
    }

#ifdef KERNEL_NEEDS_ADDRESS_SPACE_SWITCH
    Processor::switchAddressSpace(currva);
#endif
}

static bool unmap(uintptr_t location)
{
    VirtualAddressSpace &va = VirtualAddressSpace::getKernelAddressSpace();
#ifdef KERNEL_NEEDS_ADDRESS_SPACE_SWITCH
    VirtualAddressSpace &currva = Processor::information().getVirtualAddressSpace();
    Processor::switchAddressSpace(va);
#endif

    void *page = page_align(reinterpret_cast<void *>(location));
    bool result = false;
    if ((result = va.isMapped(page)))
    {
        size_t flags = 0;
        physical_uintptr_t phys = 0;
        va.getMapping(page, phys, flags);

        va.unmap(page);
        PhysicalMemoryManager::instance().freePage(phys);
    }

#ifdef KERNEL_NEEDS_ADDRESS_SPACE_SWITCH
    Processor::switchAddressSpace(currva);
#endif

    return result;
}

MemoryPoolPressureHandler::MemoryPoolPressureHandler(MemoryPool *pool) :
    m_Pool(pool)
{
}

MemoryPoolPressureHandler::~MemoryPoolPressureHandler()
{
}

const String MemoryPoolPressureHandler::getMemoryPressureDescription()
{
    return String("MemoryPool: freeing unused pages");
}

bool MemoryPoolPressureHandler::compact()
{
    return m_Pool->trim();
}

MemoryPool::MemoryPool() :
#ifdef THREADS
    m_BlockSemaphore(0), m_BitmapLock(),
#endif
    m_BufferSize(1024), m_Pool("memory-pool"), m_bInitialised(false),
    m_AllocBitmap(), m_PressureHandler(this)
{
}

MemoryPool::MemoryPool(const char *poolName) :
#ifdef THREADS
    m_BlockSemaphore(0),
#endif
    m_BufferSize(1024), m_Pool(poolName), m_bInitialised(false), m_AllocBitmap(),
    m_PressureHandler(this)
{
}

MemoryPool::~MemoryPool()
{
    // Free all the buffers
    m_bInitialised = false;
#ifdef THREADS
    while(!m_BlockSemaphore.getValue())
        m_BlockSemaphore.release();
#endif
}

bool MemoryPool::initialise(size_t poolSize, size_t bufferSize)
{
    if(m_bInitialised)
        return true;

    if(!poolSize || !bufferSize || (bufferSize > (poolSize * PhysicalMemoryManager::getPageSize())))
        return false;

    // Find the next power of two for bufferSize, if it isn't already one
    if((bufferSize & (bufferSize - 1)))
    {
        size_t powerOf2 = 1;
        size_t lg2 = 0;
        while(powerOf2 < bufferSize)
        {
            powerOf2 <<= 1;
            lg2 ++;
        }
        bufferSize = powerOf2;
    }

    m_BufferSize = bufferSize;

    NOTICE("MemoryPool: allocating memory pool '" << m_Pool.name() << "', " << Dec << ((poolSize * 4096) / 1024) << Hex << "K. Buffer size is " << m_BufferSize << ".");
    m_bInitialised = PhysicalMemoryManager::instance().allocateRegion(
        m_Pool,
        poolSize,
        PhysicalMemoryManager::virtualOnly,
        VirtualAddressSpace::Write | VirtualAddressSpace::KernelMode
    );
    if(!m_bInitialised)
        return false;

    size_t nBuffers = (poolSize * 0x1000) / bufferSize;
#ifdef THREADS
    m_BlockSemaphore.release(nBuffers);
#endif

    // Register us as a memory pressure handler, with top priority. We should
    // very easily be able to free pages in most cases.
    MemoryPressureManager::instance().registerHandler(MemoryPressureManager::HighestPriority, &m_PressureHandler);

    return true;
}

uintptr_t MemoryPool::allocate()
{
    if(!m_bInitialised)
        return 0;

    /// \bug Race if another allocate() call occurs between the acquire and the doer
#ifdef THREADS
    if(!m_BlockSemaphore.tryAcquire())
    {
        ERROR("MemoryPool: COMPLETELY out of buffers!");
        m_BlockSemaphore.acquire();
    }
#endif
    return allocateDoer();
}

uintptr_t MemoryPool::allocateNow()
{
    if(!m_bInitialised)
        return 0;

#ifdef THREADS
    if(m_BlockSemaphore.tryAcquire())
        return allocateDoer();
    else
        return 0;
#else
    return allocateDoer(); // can't block without threads
#endif
}

uintptr_t MemoryPool::allocateDoer()
{
    // Find a free buffer
    size_t poolSize = m_Pool.size();
    size_t nBuffers = poolSize / m_BufferSize;
    uintptr_t poolBase = reinterpret_cast<uintptr_t>(m_Pool.virtualAddress());

    bool bInvalid = false;
    size_t n = 0;
    {
        LockGuard<Spinlock> guard(m_BitmapLock);
        n = m_AllocBitmap.getFirstClear();
        if(n >= nBuffers)
            bInvalid = true;
        else
            m_AllocBitmap.set(n);
    }

    if(bInvalid)
    {
        FATAL("MemoryPool::allocateDoer - no buffers available, shouldn't have been called.");
        return 0;
    }

    uintptr_t result = poolBase + (n * 0x1000);
    map(result);

    return result;
}

void MemoryPool::free(uintptr_t buffer)
{
    if(!m_bInitialised)
        return;

    {
        LockGuard<Spinlock> guard(m_BitmapLock);
        size_t n = (buffer - reinterpret_cast<uintptr_t>(m_Pool.virtualAddress())) / m_BufferSize;
        m_AllocBitmap.clear(n);
    }

#ifdef THREADS
    m_BlockSemaphore.release();
#endif
}

bool MemoryPool::trim()
{
    size_t poolSize = m_Pool.size();
    size_t nBuffers = poolSize / m_BufferSize;
    uintptr_t poolBase = reinterpret_cast<uintptr_t>(m_Pool.virtualAddress());

    // Easy trim if buffers are pages or larger (remember that buffer sizes are
    // rounded up to the next power of two).
    size_t nFreed = 0;
    if (m_BufferSize >= PhysicalMemoryManager::getPageSize())
    {
        for (size_t n = 0; n < nBuffers; ++n)
        {
            if (!m_AllocBitmap.test(n))
            {
                uintptr_t page = poolBase + (n * 0x1000);
                for (size_t off = 0; off < m_BufferSize; off += PhysicalMemoryManager::getPageSize())
                {
                    if (unmap(page + off))
                        ++nFreed;
                }
            }
        }
    }
    else
    {
        // Need to find N contiguous sets of bits.
        // We also need to navigate in blocks of pages.
        size_t N = PhysicalMemoryManager::getPageSize() / m_BufferSize;
        for (size_t n = 0, m = 0; n < nBuffers; n += N, ++m)
        {
            if (m_AllocBitmap.test(n))
                continue;

            bool ok = true;
            for (size_t y = 1; y < N; ++y)
            {
                if (m_AllocBitmap.test(n + y))
                {
                    ok = false;
                    break;
                }
            }

            if (!ok)
                continue;

            uintptr_t page = poolBase + (m * 0x1000);
            if (unmap(page))
                ++nFreed;
        }
    }

    return nFreed > 0;
}
