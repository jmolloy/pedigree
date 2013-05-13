/*
 * Copyright (c) 2010 Matthew Iselin
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
#ifndef _UTILITY_MEMORY_POOL
#define _UTILITY_MEMORY_POOL

#include <processor/types.h>
#ifdef THREADS
#include <process/Semaphore.h>
#endif
#include <processor/MemoryRegion.h>
#include <utilities/ExtensibleBitmap.h>
#include <processor/PhysicalMemoryManager.h>

/** MemoryPool - a class which encapsulates a pool of memory with constant
  * sized buffers that can be allocated around the kernel. Intended to be
  * used instead of the heap for areas where similar sized buffers are
  * regularly allocated, such as networking code. */
class MemoryPool
{
    public:
        MemoryPool();
        MemoryPool(const char *poolName);
        virtual ~MemoryPool();

        /// Initialises the pool, preparing it for use
        /// @param poolSize Number of pages in the pool.
        /// @param bufferSize Size of each buffer. Will be rounded to the
        ///                   next power of two.
        bool initialise(size_t poolSize, size_t bufferSize = 1024);
        
        /// Call if you aren't certain that the object has been initialised yet
        inline bool initialised()
        {
            return m_bInitialised;
        }

        /// Allocates a buffer from the pool. Will block if no buffers are
        /// available yet.
        uintptr_t allocate();

        /// Allocates a buffer from the pool. If no buffers are available, this
        /// function will return straight away.
        /// @return Zero if a buffer couldn't be allocated.
        uintptr_t allocateNow();

        /// Frees an allocated buffer, allowing it to be used elsewhere
        void free(uintptr_t buffer);

    private:
#ifdef THREADS
        /// This Semaphore tracks the number of buffers allocated, and allows
        /// blocking when the buffers run out.
        Semaphore m_BlockSemaphore;

        /// This Spinlock turns bitmap operations into an atomic operation to
        /// avoid race conditions where the same buffer is allocated twice.
        Spinlock m_BitmapLock;
#endif

        /// Size of each buffer in this pool
        size_t m_BufferSize;

        /// MemoryRegion describing the actual pool of memory
        MemoryRegion m_Pool;
        
        /// Has this instance been initialised yet?
        bool m_bInitialised;

        /// Allocation bitmap
        ExtensibleBitmap m_AllocBitmap;

        /// Allocation doer
        uintptr_t allocateDoer();
};

#endif
