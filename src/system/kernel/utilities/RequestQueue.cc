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

#include <utilities/assert.h>

RequestQueue::RequestQueue() :
  m_RequestQueueSize(0),
  m_Stop(false), m_RequestQueueMutex(false), m_pThread(0)
{
    for (size_t i = 0; i < REQUEST_QUEUE_NUM_PRIORITIES; i++)
        m_pRequestQueue[i] = 0;
}

RequestQueue::~RequestQueue()
{
}

void RequestQueue::initialise()
{
  // Start the worker thread.
#ifdef THREADS
  m_pThread = new Thread(Processor::information().getCurrentThread()->getParent(),
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

uint64_t RequestQueue::addRequest(size_t priority, uint64_t p1, uint64_t p2, uint64_t p3, uint64_t p4,
                                  uint64_t p5, uint64_t p6, uint64_t p7, uint64_t p8)
{
  // Create a new request object.
  Request *pReq = new Request();
  pReq->p1 = p1; pReq->p2 = p2; pReq->p3 = p3; pReq->p4 = p4; pReq->p5 = p5; pReq->p6 = p6; pReq->p7 = p7; pReq->p8 = p8;
  pReq->isAsync = false;
  pReq->next = 0;

  // Add to the request queue.
  m_RequestQueueMutex.acquire();

  if (m_pRequestQueue[priority] == 0)
    m_pRequestQueue[priority] = pReq;
  else
  {
    Request *p = m_pRequestQueue[priority];
    while (p->next != 0)
      p = p->next;
    p->next = pReq;
  }

  // Increment the number of items on the request queue.
  m_RequestQueueSize.release();

  m_RequestQueueMutex.release();

  // We are waiting on the worker thread - mark the thread as such.
  Thread *pThread = Processor::information().getCurrentThread();
  pThread->setBlockingThread(m_pThread);

  // Wait for the request to be satisfied. This should sleep the thread.
  pReq->mutex.acquire();

  pThread->setBlockingThread(0);

  if(pThread->wasInterrupted() || pThread->getUnwindState() == Thread::Exit)
  {
      // The request was interrupted somehow. We cannot assume that pReq's
      // contents are valid, so just return zero. The caller may have to redo
      // their request.
      // By releasing here, the worker thread can detect that the request was
      // interrupted and clean up by itself.
      NOTICE("RequestQueue::addRequest - interrupted");
      pReq->mutex.release();
      return 0;
  }

  // Grab the result.
  uintptr_t ret = pReq->ret;

  // Delete the request structure.
  delete pReq;

  return ret;
}

uint64_t RequestQueue::addAsyncRequest(size_t priority, uint64_t p1, uint64_t p2, uint64_t p3, uint64_t p4,
                                       uint64_t p5, uint64_t p6, uint64_t p7, uint64_t p8)
{
  // Create a new request object.
  Request *pReq = new Request();
  pReq->p1 = p1; pReq->p2 = p2; pReq->p3 = p3; pReq->p4 = p4; pReq->p5 = p5; pReq->p6 = p6; pReq->p7 = p7; pReq->p8 = p8;
  pReq->isAsync = true;
  pReq->next = 0;

  // Add to the request queue.
  m_RequestQueueMutex.acquire();

  if (m_pRequestQueue[priority] == 0)
    m_pRequestQueue[priority] = pReq;
  else
  {
    Request *p = m_pRequestQueue[priority];
    while (p->next != 0)
    {
      if(p == pReq)
        return 0;
      p = p->next;
    }
    if(p != pReq)
      p->next = pReq;
    else
      return 0;
  }

  assert_heap_ptr_valid(pReq);

  // Increment the number of items on the request queue.
  m_RequestQueueSize.release();

  // If the queue is too long, make this block.
  if (m_RequestQueueSize.getValue() > REQUEST_QUEUE_MAX_QUEUE_SZ)
  {
      pReq->isAsync = false;

      m_RequestQueueMutex.release();

      // We are waiting on the worker thread - mark the thread as such.
      Thread *pThread = Processor::information().getCurrentThread();
      pThread->setBlockingThread(m_pThread);

      // Wait for the request to be satisfied. This should sleep the thread.
      pReq->mutex.acquire();

      pThread->setBlockingThread(0);

      if(pThread->wasInterrupted() || pThread->getUnwindState() == Thread::Exit)
      {
          // The request was interrupted somehow. We cannot assume that pReq's
          // contents are valid, so just return zero. The caller may have to redo
          // their request.
          // By releasing here, the worker thread can detect that the request was
          // interrupted and clean up by itself.
          NOTICE("RequestQueue::addRequest - interrupted");
          pReq->mutex.release();
          return 0;
      }

      // Delete the request structure.
      delete pReq;
  }
  else
      m_RequestQueueMutex.release();

  return 0;
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

    // Get the most important queue with data in.
    /// \todo Stop possible starvation here.
    size_t priority = 0;
    for (priority = 0; priority < REQUEST_QUEUE_NUM_PRIORITIES-1; priority++)
        if (m_pRequestQueue[priority])
            break;

    Request *pReq = m_pRequestQueue[priority];
    // Quick sanity check:
    if (pReq == 0)
    {
        if(Processor::information().getCurrentThread()->getUnwindState() == Thread::ReleaseBlockingThread)
            continue;
        ERROR("Unwind state: " << (size_t)Processor::information().getCurrentThread()->getUnwindState());
        FATAL("RequestQueue: Worker thread woken but no requests pending!");
    }
    m_pRequestQueue[priority] = pReq->next;

    m_RequestQueueMutex.release();

    // Perform the request.
    pReq->ret = executeRequest(pReq->p1, pReq->p2, pReq->p3, pReq->p4, pReq->p5, pReq->p6, pReq->p7, pReq->p8);
    if(pReq->mutex.tryAcquire())
    {
        // Something's gone wrong - the calling thread has released the Mutex. Destroy the request
        // and grab the next request from the queue. The calling thread has long since stopped
        // caring about whether we're done or not.
        NOTICE("RequestQueue::work - caller interrupted");
        delete pReq;
        continue;
    }
    switch (Processor::information().getCurrentThread()->getUnwindState())
    {
        case Thread::Continue:
            break;
        case Thread::Exit:
            return 0;
        case Thread::ReleaseBlockingThread:
            Processor::information().getCurrentThread()->setUnwindState(Thread::Continue);
            break;
    }

    bool bAsync = pReq->isAsync;

    // Request finished - post the request's mutex to wake the calling thread.
    pReq->mutex.release();

    // If the request was asynchronous, destroy the request structure.
    if (bAsync)
        delete pReq;
  }
  return 0;
}
