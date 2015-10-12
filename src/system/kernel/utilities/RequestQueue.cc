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

#include <utilities/RequestQueue.h>
#include <processor/Processor.h>
#include <process/Scheduler.h>
#include <panic.h>
#include <Log.h>

#include <utilities/assert.h>

RequestQueue::RequestQueue() :
  m_Stop(false), m_RequestQueueMutex(false)
#ifdef THREADS
  , m_RequestQueueSize(0), m_pThread(0)
#endif
  , m_Halted(false), m_HaltAcknowledged(false)
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
  if(m_pThread)
  {
    WARNING("RequestQueue initialised multiple times - don't do this.");
    return;
  }

  // Start RequestQueue workers in the kernel process only.
  Process *pProcess = Scheduler::instance().getKernelProcess();

  m_Stop = false;
  m_pThread = new Thread(pProcess, &trampoline, reinterpret_cast<void*>(this));
  m_Halted = false;
#else
  WARNING("RequestQueue: This build does not support threads");
#endif
}

void RequestQueue::destroy()
{
#ifdef THREADS
  // Halt the queue - we're done.
  halt();

  /// \todo We really should clean up the queue.
#endif
}

uint64_t RequestQueue::addRequest(size_t priority, uint64_t p1, uint64_t p2, uint64_t p3, uint64_t p4,
                                  uint64_t p5, uint64_t p6, uint64_t p7, uint64_t p8)
{
#ifdef THREADS
  // Create a new request object.
  Request *pReq = new Request();
  pReq->p1 = p1; pReq->p2 = p2; pReq->p3 = p3; pReq->p4 = p4; pReq->p5 = p5; pReq->p6 = p6; pReq->p7 = p7; pReq->p8 = p8;
  pReq->next = 0;
  pReq->bReject = false;
  pReq->refcnt = 1;
  pReq->owner = this;
  pReq->priority = priority;

  // Do we own pReq?
  bool bOwnRequest = true;

  // Add to the request queue.
  m_RequestQueueMutex.acquire();

  if (m_pRequestQueue[priority] == 0)
    m_pRequestQueue[priority] = pReq;
  else
  {
    Request *p = m_pRequestQueue[priority];
    while (p->next != 0)
    {
        // Wait for duplicates instead of re-inserting, if the compare function is defined.
        if(compareRequests(*p, *pReq))
        {
            bOwnRequest = false;
            delete pReq;
            pReq = p;
            break;
        }
      p = p->next;
    }

    if(bOwnRequest && compareRequests(*p, *pReq))
    {
        bOwnRequest = false;
        delete pReq;
        pReq = p;
    }
    else if(bOwnRequest)
        p->next = pReq;
  }

  if(!bOwnRequest)
  {
    ++pReq->refcnt;
  }
  else
  {
    pReq->pThread = Processor::information().getCurrentThread();
    pReq->pThread->addRequest(pReq);
  }

  // Increment the number of items on the request queue.
  m_RequestQueueSize.release();

  m_RequestQueueMutex.release();

  // We are waiting on the worker thread - mark the thread as such.
  Thread *pThread = Processor::information().getCurrentThread();
  pThread->setBlockingThread(m_pThread);

  if(pReq->bReject)
  {
      // Hmm, in the time the RequestQueueMutex was being acquired, we got
      // pre-empted, and then an unexpected exit event happened. The request
      // is to be rejected, so don't acquire the mutex at all.
      if(!--pReq->refcnt)
        delete pReq;
      return 0;
  }

  // Wait for the request to be satisfied. This should sleep the thread.
  pReq->mutex.acquire();

  // Don't use the Thread object if it may be already freed
  if(!pReq->bReject)
      pThread->setBlockingThread(0);

  if(pReq->bReject || pThread->wasInterrupted() || pThread->getUnwindState() == Thread::Exit)
  {
      // The request was interrupted somehow. We cannot assume that pReq's
      // contents are valid, so just return zero. The caller may have to redo
      // their request.
      // By releasing here, the worker thread can detect that the request was
      // interrupted and clean up by itself.
      NOTICE("RequestQueue::addRequest - interrupted");
      if(pReq->bReject && !--pReq->refcnt)
          delete pReq; // Safe to delete, unexpected exit condition
      else
          pReq->mutex.release();
      return 0;
  }

  // Grab the result.
  uintptr_t ret = pReq->ret;

  // Delete the request structure.
  if(bOwnRequest && pReq->pThread)
    pReq->pThread->removeRequest(pReq);
  if(!--pReq->refcnt)
    delete pReq;
  else
    pReq->mutex.release();

  return ret;
#else
  return executeRequest(p1, p2, p3, p4, p5, p6, p7, p8);
#endif
}

int RequestQueue::doAsync(void *p)
{
  RequestQueue::Request *pReq = reinterpret_cast<RequestQueue::Request *>(p);
  pReq->owner->addRequest(pReq->priority, pReq->p1, pReq->p2, pReq->p3,
                          pReq->p4, pReq->p5, pReq->p6, pReq->p7, pReq->p8);
  delete pReq;
  return 0;
}

uint64_t RequestQueue::addAsyncRequest(size_t priority, uint64_t p1, uint64_t p2, uint64_t p3, uint64_t p4,
                                       uint64_t p5, uint64_t p6, uint64_t p7, uint64_t p8)
{
#ifndef THREADS
  return addRequest(priority, p1, p2, p3, p4, p5, p6, p7, p8);
#else
  // Create a new request object.
  Request *pReq = new Request();
  pReq->p1 = p1; pReq->p2 = p2; pReq->p3 = p3; pReq->p4 = p4; pReq->p5 = p5; pReq->p6 = p6; pReq->p7 = p7; pReq->p8 = p8;
  pReq->next = 0;
  pReq->bReject = false;
  pReq->refcnt = 0;
  pReq->owner = this;
  pReq->priority = priority;

  // Add to RequestQueue.
  Process *pProcess = Scheduler::instance().getKernelProcess();
  Thread *pThread = new Thread(pProcess, &doAsync,
      reinterpret_cast<void *>(pReq));
  pThread->detach();
#endif

  return 0;
}

void RequestQueue::halt()
{
  LockGuard<Mutex> guard(m_RequestQueueMutex);

  if(!m_Halted)
  {
    m_Stop = true;
    m_RequestQueueSize.release();
    m_pThread->join();
    m_pThread = 0;
    m_Halted = true;
  }
}

void RequestQueue::resume()
{
  LockGuard<Mutex> guard(m_RequestQueueMutex);

  if(m_Halted)
  {
    initialise();
  }
}

int RequestQueue::trampoline(void *p)
{
  RequestQueue *pRQ = reinterpret_cast<RequestQueue*> (p);
  return pRQ->work();
}

int RequestQueue::work()
{
#ifdef THREADS
  while (true)
  {
    // Are we halted?
    if (m_Halted)
    {
      // We will halt as soon as we hit the mutex acquire - all good for the
      // caller of halt() to continue now.
      m_HaltAcknowledged.release();
    }

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
        // Probably got woken up by a resume() after halt() left the mutex with
        // an un-acked count.
        m_RequestQueueMutex.release();
        continue;
    }
    m_pRequestQueue[priority] = pReq->next;

    m_RequestQueueMutex.release();

    // Verify that it's still valid to run the request
    if (pReq->bReject)
    {
        continue;
    }

    // Perform the request.
    pReq->ret = executeRequest(pReq->p1, pReq->p2, pReq->p3, pReq->p4, pReq->p5, pReq->p6, pReq->p7, pReq->p8);
    if (pReq->mutex.tryAcquire())
    {
        // Something's gone wrong - the calling thread has released the Mutex. Destroy the request
        // and grab the next request from the queue. The calling thread has long since stopped
        // caring about whether we're done or not.
        NOTICE("RequestQueue::work - caller interrupted");
        if(pReq->pThread)
            pReq->pThread->removeRequest(pReq);
        continue;
    }
    switch (Processor::information().getCurrentThread()->getUnwindState())
    {
        case Thread::Continue:
            break;
        case Thread::Exit:
            WARNING("RequestQueue: unwind state is Exit, request not cleaned up. Leak?");
            return 0;
        case Thread::ReleaseBlockingThread:
            Processor::information().getCurrentThread()->setUnwindState(Thread::Continue);
            break;
    }

    // Request finished - post the request's mutex to wake the calling thread.
    pReq->bCompleted = true;
    pReq->mutex.release();
  }
#endif
  return 0;
}

bool RequestQueue::isRequestValid(const Request *r)
{
  // Halted RequestQueue already has the RequestQueue mutex held.
  LockGuard<Mutex> guard(m_RequestQueueMutex);

  for (size_t priority = 0; priority < REQUEST_QUEUE_NUM_PRIORITIES - 1; ++priority)
  {
    Request *pReq = m_pRequestQueue[priority];
    while (pReq)
    {
      if (pReq == r)
        return true;

      pReq = pReq->next;
    }
  }

  return false;
}
