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

UserConsole::UserConsole() :
    RequestQueue(), m_pReq(0), m_pPendingReq(0)
{
}

UserConsole::~UserConsole()
{
}

void UserConsole::requestPending()
{
    // This stops a race where m_pReq is used or changed between setting
    // m_pPendingReq and setting m_pReq to zero.
    m_RequestQueueMutex.acquire();
    m_pPendingReq = m_pReq;
    if(m_pReq)
        m_pReq = m_pReq->next; // 0;
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

size_t UserConsole::nextRequest(size_t responseToLast, char *buffer, size_t *sz, size_t maxBuffSz, size_t *terminalId, bool bAsyncOnly)
{
    // Lock m_pReq. Any use of the RequestQueue will *probably* end up with a
    // change of m_pReq, so stop anyone from accessing the queue.
    m_RequestQueueMutex.acquire();
    if (m_pReq && !m_pReq->bReject)
    {
        m_pReq->ret = responseToLast;

        if (m_pReq->p1 == CONSOLE_READ)
            memcpy(reinterpret_cast<uint8_t*>(m_pReq->p4), buffer, responseToLast);

        bool bAsync = m_pReq->isAsync;

        // RequestQueue finished - post the request's mutex to wake the calling thread.
        m_pReq->mutex.release();

        if (bAsync)
        {
            WARNING("TUI: Async request in the console request list, bad things can happen");
            assert_heap_ptr_valid(m_pReq);
            if(m_pReq->pThread)
                m_pReq->pThread->removeRequest(m_pReq);
            delete m_pReq;
        }

        // Avoid using this request again if possible (it will probably be freed
        // by the thread that was waiting on the Semaphore if it wasn't async,
        // and it was just freed by us if it was async).
        m_pReq = 0;
    }
    m_RequestQueueMutex.release();

    // Sleep on the queue length semaphore - wake when there's something to do.
    if(bAsyncOnly)
    {
        if(!m_RequestQueueSize.tryAcquire())
        {
            return 0;
        }
    }
    else
    {
        m_RequestQueueSize.acquire();
    }

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
        m_RequestQueueMutex.release();

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
        // We have to allow escape codes through as they are needed to allow
        // ncurses applications to shutdown without leaving their interface
        // plastered across the TUI.
        char *buff = reinterpret_cast<char*>(m_pReq->p4);
        if(!buff || buff[0] != '\e')
        {
            WARNING("UserConsole: request rejected");
            return 0;
        }
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

    return command;
}
