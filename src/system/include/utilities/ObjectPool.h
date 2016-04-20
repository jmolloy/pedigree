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

#ifndef KERNEL_UTILITY_OBJECT_POOL_H
#define KERNEL_UTILITY_OBJECT_POOL_H

#include <processor/types.h>
#ifdef THREADS
#include <Spinlock.h>
#include <LockGuard.h>
#endif
#include <utilities/Vector.h>

/// \todo add a MemoryPressureHandler here.

/**
 * ObjectPool manages a set of objects for rapid allocation and deallocation,
 * which is particularly useful for objects which are used frequently (e.g.
 * nodes in a list).
 *
 * If no objects are available (none have been deallocated yet), new objects
 * are allocated from the heap.
 */
template <class T, size_t poolSize = 16>
class ObjectPool
{
    public:
        ObjectPool() : m_Pool()
#ifdef THREADS
            , m_Spinlock()
#endif        
        {
        }

        virtual ~ObjectPool()
        {
            for (auto it = m_Pool.begin(); it != m_Pool.end(); ++it)
            {
                delete *it;
            }
        }

        T *allocate()
        {
#ifdef THREADS
            LockGuard<Spinlock> guard(m_Spinlock);
#endif

            m_Pool.reserve(poolSize, true);
            if (m_Pool.count())
            {
                // popping from the rear of a vector is much faster
                return m_Pool.popBack();
            }
            else
            {
                return new T;
            }
        }

        void deallocate(T *object)
        {
#ifdef THREADS
            LockGuard<Spinlock> guard(m_Spinlock);
#endif

            // We only add the object back to the pool if we aren't already at
            // capacity (otherwise we'd resize the Vector).
            m_Pool.reserve(poolSize, true);
            if (m_Pool.count() < poolSize)
            {
                m_Pool.pushBack(object);
            }
            else
            {
                delete object;
            }
        }

    private:
        Vector<T *> m_Pool;
#ifdef THREADS
        Spinlock m_Spinlock;
#endif
};

#endif  // KERNEL_UTILITY_OBJECT_POOL_H
