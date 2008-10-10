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

#include <utilities/RequestQueue.h>
#include <processor/Processor.h>
#include <panic.h>
#include <Log.h>

RequestQueue::RequestQueue() :
  m_pRequestQueue(0), m_RequestQueueSize(0), m_Stop(false), m_RequestQueueMutex(false)
{
}

RequestQueue::~RequestQueue()
{
}

void RequestQueue::initialise()
{
  // Start the worker thread.
#ifdef THREADS
  new Thread(Processor::information().getCurrentThread()->getParent(),
             reinterpret_cast<Thread::ThreadStartFunc> (&trampoline),
             reinterpret_cast<void*> (this));
#endif
}

void RequestQueue::destroy()
{
  // Cause the worker thread to stop.
  m_Stop = true;
  // Post to the queue length semaphore to ensure the worker thread wakes up.
  m_RequestQueueSize.release();
}

uint64_t RequestQueue::addRequest(uint64_t p1, uint64_t p2, uint64_t p3, uint64_t p4,
                                   uint64_t p5, uint64_t p6, uint64_t p7, uint64_t p8)
{
  // Create a new request object.
  Request *pReq = new Request();
  pReq->p1 = p1; pReq->p2 = p2; pReq->p3 = p3; pReq->p4 = p4; pReq->p5 = p5; pReq->p6 = p6; pReq->p7 = p7; pReq->p8 = p8;

  // Add to the request queue.
  m_RequestQueueMutex.acquire();
  
  if (m_pRequestQueue == 0)
    m_pRequestQueue = pReq;
  else
  {
    Request *p = m_pRequestQueue;
    while (p->next != 0)
      p = p->next;
    p->next = pReq;
  }

  // Increment the number of items on the request queue.
  m_RequestQueueSize.release();

  m_RequestQueueMutex.release();

  // Wait for the request to be satisfied. This should sleep the thread.
  pReq->mutex.acquire();

  // Grab the result.
  uintptr_t ret = pReq->ret;
  
  // Delete the request structure.
  delete pReq;

  return ret;
}

int RequestQueue::trampoline(void *p)
{
  RequestQueue *pRQ = reinterpret_cast<RequestQueue*> (p);
  return pRQ->work();
}

int RequestQueue::work()
{
  while (true)
  {    
    // Sleep on the queue length semaphore - wake when there's something to do.
    m_RequestQueueSize.acquire();

    // Check why we were woken - is m_Stop set? If so, quit.
    if (m_Stop)
      return 0;

    // Get the first request from the queue.
    m_RequestQueueMutex.acquire();

    Request *pReq = m_pRequestQueue;
    // Quick sanity check:
    if (pReq == 0) panic("RequestQueue: Worker thread woken but no requests pending!");
    m_pRequestQueue = pReq->next;

    m_RequestQueueMutex.release();

    // Perform the request.
    pReq->ret = executeRequest(pReq->p1, pReq->p2, pReq->p3, pReq->p4, pReq->p5, pReq->p6, pReq->p7, pReq->p8);

    // Request finished - post the request's mutex to wake the calling thread.
    pReq->mutex.release();
  }
}
