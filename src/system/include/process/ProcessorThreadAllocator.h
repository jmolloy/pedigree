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

#ifndef _PROCESSOR_THREAD_ALLOCATOR_H
#define _PROCESSOR_THREAD_ALLOCATOR_H

#include <process/ThreadToCoreAllocationAlgorithm.h>
#include <process/PerProcessorScheduler.h>
#include <process/Thread.h>

#include <processor/types.h>
#include <processor/state.h>

class ProcessorThreadAllocator
{
    public:
        ProcessorThreadAllocator();
        virtual ~ProcessorThreadAllocator();
        
        static ProcessorThreadAllocator &instance()
        {
            return m_Instance;
        }
        
        /// Called when a thread is to be added to the system schedule. This
        /// function adds the thread to the correct PerProcessorScheduler (as
        /// per the allocation algorithm) and deals with everything on the
        /// scheduler side. If a new thread is to be added to the schedule, only
        /// this function should need to be called.
        void addThread(Thread *pThread, Thread::ThreadStartFunc pStartFunction,
                   void *pParam, bool bUsermode, void *pStack);

        /// Same as the other addThread(), but takes a SyscallState instead.
        void addThread(Thread *pThread, SyscallState &state);
        
        /// Notifies an algorithm a thread has been removed. This allows a
        /// rebalance operation or something similar to take place.
        void threadRemoved(Thread *pThread);
        
        /// Sets the algorithm to use for allocating threads to cores.
        inline void setAlgorithm(ThreadToCoreAllocationAlgorithm *pAlgorithm)
        {
            m_pAlgorithm = pAlgorithm;
        }
    
    private:
    
        ThreadToCoreAllocationAlgorithm *m_pAlgorithm;
        
        static ProcessorThreadAllocator m_Instance;
};

#endif

