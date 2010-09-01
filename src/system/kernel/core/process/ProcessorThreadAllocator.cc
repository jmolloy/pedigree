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
#include <process/ProcessorThreadAllocator.h>

#include <process/PerProcessorScheduler.h>
#include <process/Scheduler.h>
#include <process/Thread.h>

#include <processor/Processor.h>

ProcessorThreadAllocator ProcessorThreadAllocator::m_Instance;

ProcessorThreadAllocator::ProcessorThreadAllocator() :
    m_pAlgorithm(0)
{
}

ProcessorThreadAllocator::~ProcessorThreadAllocator()
{
    delete m_pAlgorithm;
}

void ProcessorThreadAllocator::addThread(Thread *pThread, Thread::ThreadStartFunc pStartFunction,
                   void *pParam, bool bUsermode, void *pStack)
{
    PerProcessorScheduler* pSchedule = m_pAlgorithm->allocateThread(pThread);
    
    Scheduler::instance().addThread(pThread, *pSchedule);
    
    pSchedule->addThread(pThread, pStartFunction, pParam, bUsermode, pStack);
}

void ProcessorThreadAllocator::threadRemoved(Thread *pThread)
{
}

