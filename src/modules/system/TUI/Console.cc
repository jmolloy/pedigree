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
#include "Console.h"
#include <panic.h>
#include <utilities/assert.h>
#include <processor/PhysicalMemoryManager.h>

UserConsole::UserConsole() :
    RequestQueue(), m_pReq(0), m_pPendingReq(0), m_TuiReadHandler(0), m_TuiWriteHandler(0), m_TuiMiscHandler(0),
    m_pTuiThread(0), m_NextRequestID(), m_ActiveRequests()
{
}

UserConsole::~UserConsole()
{
}

uint64_t UserConsole::addRequest(size_t priority, uint64_t p1, uint64_t p2, uint64_t p3, uint64_t p4, uint64_t p5,
                                                  uint64_t p6, uint64_t p7, uint64_t p8)
{
    if(!m_TuiReadHandler)
        FATAL("UserConsole: TUI hasn't registered yet!");

    Thread *pThread = Processor::information().getCurrentThread();
    Process *pProcess = pThread->getParent();

    // NOTICE("[" << pProcess->getId() << "]    addRequest(" << p1 << ", " << p2 << ", " << p3 << ", " << p4 << ", " << p5 << ")");

    m_RequestQueueMutex.acquire();
    pThread->addSpinlock(&m_RequestQueueMutex);

    // Prepare the request information structure
    TuiRequest *pReq = new TuiRequest;
    pReq->id = m_NextRequestID++;

    // Decide what to do about the buffer.
    uintptr_t handlerAddress = 0, virtualBase = 0;
    if(p1 == CONSOLE_READ || p1 == CONSOLE_WRITE)
    {
        // Because we're playing with buffers here, we need to switch to the TUI
        // address space and map in the buffer.
        VirtualAddressSpace &oldVa = Processor::information().getVirtualAddressSpace();
        Processor::switchAddressSpace(*m_pTuiThread->getParent()->getAddressSpace());
        
        // We need a buffer. A read needs to write to the buffer, and a write
        // needs to read from it, so we need to allocate some memory for the
        // TUI to use.
        size_t pgSize = PhysicalMemoryManager::instance().getPageSize();
        size_t nPages = (p3 / pgSize) + 1;

        m_pTuiThread->getParent()->getSpaceAllocator().allocate(p3, virtualBase);
        for(size_t i = 0; i < nPages; i++)
        {
            physical_uintptr_t phys = PhysicalMemoryManager::instance().allocatePage();
            Processor::information().getVirtualAddressSpace().map(phys,
                                                                  reinterpret_cast<void*> (virtualBase + (i * pgSize)),
                                                                  VirtualAddressSpace::Write);
        }

        pReq->buffer = virtualBase;
        pReq->bufferSize = p3;

        if(p1 == CONSOLE_READ)
            handlerAddress = m_TuiReadHandler;
        else if(p1 == CONSOLE_WRITE)
        {
            memcpy(reinterpret_cast<uint8_t*>(virtualBase), reinterpret_cast<uint8_t*>(p4), p3);
            handlerAddress = m_TuiWriteHandler;
        }

        Processor::switchAddressSpace(oldVa);
    }
    else
    {
        // No buffer needed
        pReq->buffer = pReq->bufferSize = 0;
        handlerAddress = m_TuiMiscHandler;
    }

    // Set up the rest of the request
    pReq->returnValue = 0;
    pReq->reqType = p1;

    // Add this to our tree
    m_ActiveRequests.insert(pReq->id, pReq);

    // Create the event to send to the TUI
    TuiRequestEvent *pEvent = new TuiRequestEvent(pReq->reqType, pReq->buffer, pReq->bufferSize, pReq->id, p2, handlerAddress);

    // Send it and then acquire the mutex to wait for it to complete
    m_pTuiThread->sendEvent(pEvent);
    pReq->mutex.acquire();

    // Copy the buffer now, if needed
    size_t ret = pReq->returnValue;

    // If there was a buffer created, use it (if needed) and free it
    if(virtualBase)
    {
        // Because we're playing with buffers here, we need to switch to the TUI
        // address space and map in the buffer.
        VirtualAddressSpace &oldVa = Processor::information().getVirtualAddressSpace();
        Processor::switchAddressSpace(*m_pTuiThread->getParent()->getAddressSpace());

        // Copy the buffer of read bytes first (if there are any)
        if(p1 == CONSOLE_READ && ret > 0)
        {
            if(ret > p3)
                ret = p3;
            memcpy(reinterpret_cast<uint8_t*>(p4), reinterpret_cast<uint8_t*>(virtualBase), ret);
        }

        // And then free the pages
        size_t pgSize = PhysicalMemoryManager::instance().getPageSize();
        size_t nPages = (p3 / pgSize) + 1;
        for(size_t i = 0; i < nPages; i++)
        {
            size_t flags;
            physical_uintptr_t phys;
            void *addr = reinterpret_cast<void*>(virtualBase + (i * pgSize));
            if(Processor::information().getVirtualAddressSpace().isMapped(addr))
            {
                Processor::information().getVirtualAddressSpace().getMapping(addr, phys, flags);
                Processor::information().getVirtualAddressSpace().unmap(addr);
                PhysicalMemoryManager::instance().freePage(phys);
            }
        }

        m_pTuiThread->getParent()->getSpaceAllocator().free(virtualBase, p3);

        Processor::switchAddressSpace(oldVa);
    }

    /// \todo Handle unusual exit cases
    delete pReq;
    pThread->removeSpinlock(&m_RequestQueueMutex);
    m_RequestQueueMutex.release();

    return ret;
}

void UserConsole::requestPending()
{
    // This stops a race where m_pReq is used or changed between setting
    // m_pPendingReq and setting m_pReq to zero.
    m_RequestQueueMutex.acquire();
    m_pPendingReq = m_pReq;
    if(m_pReq)
        m_pReq = m_pReq->next;
    m_RequestQueueMutex.release();
}

void UserConsole::respondToPending(size_t response, char *buffer, size_t sz)
{
    m_RequestQueueMutex.acquire();
    if (m_pPendingReq)
    {
        m_pPendingReq->ret = response;

        if (m_pPendingReq->p1 == CONSOLE_READ)
        {
            memcpy(reinterpret_cast<uint8_t*>(m_pPendingReq->p4), buffer, response);
        }

        // RequestQueue finished - post the request's mutex to wake the calling thread.
        m_pPendingReq->mutex.release();

        if (m_pPendingReq->isAsync)
            delete m_pPendingReq;
        m_pPendingReq = m_pPendingReq->next;
    }
    m_RequestQueueMutex.release();
}

void UserConsole::stopCurrentBlock()
{
    if(!m_RequestQueueSize.getValue())
    {
        m_Stop = 1;
        m_RequestQueueSize.release();
    }
}

size_t UserConsole::nextRequest(size_t responseToLast, char *buffer, size_t *sz, size_t maxBuffSz, size_t *terminalId)
{
    // Lock m_pReq. Any use of the RequestQueue will *probably* end up with a
    // change of m_pReq, so stop anyone from accessing the queue.
    m_RequestQueueMutex.acquire();
    if (m_pReq)
    {
        m_pReq->ret = responseToLast;

        if (m_pReq->p1 == CONSOLE_READ)
        {
            memcpy(reinterpret_cast<uint8_t*>(m_pReq->p4), buffer, responseToLast);
        }

        bool bAsync = m_pReq->isAsync;

        // RequestQueue finished - post the request's mutex to wake the calling thread.
        m_pReq->mutex.release();

        if (bAsync)
        {
            assert_heap_ptr_valid(m_pReq);
//            if(!m_pReq->bReject)
//                m_pReq->pThread->removeRequest(m_pReq);
            delete m_pReq;
            m_pReq = 0;
        }
    }
    m_RequestQueueMutex.release();

    // Sleep on the queue length semaphore - wake when there's something to do.
    m_RequestQueueSize.acquire();

    // Check why we were woken - is m_Stop set? If so, quit.
    if (m_Stop)
    {
      m_Stop = 0;
      return 0;
    }

    // Get the first request from the queue.
    m_RequestQueueMutex.acquire();

    // Get the most important queue with data in.
    /// \todo Stop possible starvation here.
    size_t priority = 0;
    for (priority = 0; priority < REQUEST_QUEUE_NUM_PRIORITIES-1; priority++)
        if (m_pRequestQueue[priority])
            break;

    m_pReq = m_pRequestQueue[priority];
    // Quick sanity check:
    if (m_pReq == 0)
    {
        if(Processor::information().getCurrentThread()->getUnwindState() == Thread::Exit)
            return 0;
        ERROR("Unwind state: " << (size_t)Processor::information().getCurrentThread()->getUnwindState());
        FATAL("RequestQueue: Worker thread woken but no requests pending!");
    }
    m_pRequestQueue[priority] = m_pReq->next;

    assert_heap_ptr_valid(m_pReq->next);
    assert_heap_ptr_valid(m_pReq);

    m_RequestQueueMutex.release();

    // Perform the request.
    size_t command = m_pReq->p1;
    *sz = static_cast<size_t>(m_pReq->p3);

    // Verify that it's still valid to run the request
    if(m_pReq->bReject)
    {
        WARNING("UserConsole: request rejected");
        m_pReq->ret = 0;
        m_pReq->mutex.release();
        return 0;
    }

    *terminalId = m_pReq->p2;

    if (command == CONSOLE_WRITE)
    {
        if (*sz > maxBuffSz) m_pReq->p3 = maxBuffSz;
        memcpy(buffer, reinterpret_cast<uint8_t*>(m_pReq->p4), m_pReq->p3);
        if (m_pReq->isAsync)
        {
            assert_heap_ptr_valid(reinterpret_cast<uint8_t*>(m_pReq->p4));
            delete [] reinterpret_cast<uint8_t*>(m_pReq->p4);
        }
    }
    else if (command == TUI_CHAR_RECV || command == TUI_MODE_CHANGED)
    {
        memcpy(buffer, reinterpret_cast<uint8_t*>(&m_pReq->p3), 8);
        *sz = 8;
    }

    return command;
}
