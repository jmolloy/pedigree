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

#ifdef THREADS

#include <process/Thread.h>
#include <process/Scheduler.h>
#include <process/SchedulingAlgorithm.h>
#include <process/ProcessorThreadAllocator.h>
#include <processor/Processor.h>
#include <processor/StackFrame.h>
#include <machine/Machine.h>
#include <Log.h>

#include <machine/InputManager.h>

#include <processor/MemoryRegion.h>
#include <processor/PhysicalMemoryManager.h>

#include <processor/ProcessorInformation.h>

Thread::Thread(Process *pParent, ThreadStartFunc pStartFunction, void *pParam,
               void *pStack, bool semiUser, bool bDontPickCore) :
    m_nStateLevel(0), m_pParent(pParent), m_Status(Ready), m_ExitCode(0), /* m_pKernelStack(0), */ m_pAllocatedStack(0), m_Id(0),
    m_Errno(0), m_bInterrupted(false), m_Lock(), m_ConcurrencyLock(), m_EventQueue(), m_DebugState(None), m_DebugStateAddress(0),
    m_UnwindState(Continue), m_pScheduler(&Processor::information().getScheduler()), m_Priority(DEFAULT_PRIORITY),
    m_PendingRequests(), m_pTlsBase(0), m_bRemovingRequests(false), m_pWaiter(0), m_bDetached(false)
{
  if (pParent == 0)
  {
    FATAL("Thread::Thread(): Parent process was NULL!");
  }

  // Initialise our kernel stack.
  m_pAllocatedStack = 0;

  // Initialise state level zero
  m_StateLevels[0].m_pAuxillaryStack = 0;
  allocateStackAtLevel(0);

  // If we've been given a user stack pointer, we are a user mode thread.
  bool bUserMode = true;
  if (pStack == 0)
  {
    bUserMode = false;
    pStack = m_StateLevels[0].m_pAuxillaryStack = m_StateLevels[0].m_pKernelStack;
    m_StateLevels[0].m_pKernelStack = 0; // No kernel stack if kernel mode thread - causes bug on PPC
  }
  else if(semiUser)
  {
      // Still have a kernel stack for when we jump to user mode, but start the
      // thread in kernel mode first.
      bUserMode = false;
  }

  m_Id = m_pParent->addThread(this);

  // Firstly, grab our lock so that the scheduler cannot preemptively load balance
  // us while we're starting.
  m_Lock.acquire();

  // Add to the scheduler
  if(!bDontPickCore)
      ProcessorThreadAllocator::instance().addThread(this, pStartFunction, pParam, bUserMode, pStack);
  else
  {
      Scheduler::instance().addThread(this, Processor::information().getScheduler());
      Processor::information().getScheduler().addThread(this, pStartFunction, pParam,
                                                        bUserMode, pStack);
  }
}

Thread::Thread(Process *pParent) :
    m_nStateLevel(0), m_pParent(pParent), m_Status(Running), m_ExitCode(0), /* m_pKernelStack(0), */ m_pAllocatedStack(0), m_Id(0),
    m_Errno(0), m_bInterrupted(false), m_Lock(), m_ConcurrencyLock(), m_EventQueue(), m_DebugState(None), m_DebugStateAddress(0),
    m_UnwindState(Continue), m_pScheduler(&Processor::information().getScheduler()), m_Priority(DEFAULT_PRIORITY),
    m_PendingRequests(), m_pTlsBase(0), m_bRemovingRequests(false), m_pWaiter(0), m_bDetached(false)
{
  if (pParent == 0)
  {
    FATAL("Thread::Thread(): Parent process was NULL!");
  }
  m_Id = m_pParent->addThread(this);

  // Initialise our kernel stack.
  // NO! No kernel stack for kernel-mode threads. On PPC, causes bug!
  //m_pKernelStack = VirtualAddressSpace::getKernelAddressSpace().allocateStack();
}

Thread::Thread(Process *pParent, SyscallState &state) :
    m_nStateLevel(0), m_pParent(pParent), m_Status(Ready), m_ExitCode(0), /* m_pKernelStack(0), */ m_pAllocatedStack(0), m_Id(0),
    m_Errno(0), m_bInterrupted(false), m_Lock(), m_ConcurrencyLock(), m_EventQueue(), m_DebugState(None), m_DebugStateAddress(0),
    m_UnwindState(Continue), m_pScheduler(&Processor::information().getScheduler()), m_Priority(DEFAULT_PRIORITY),
    m_PendingRequests(), m_pTlsBase(0), m_bRemovingRequests(false), m_pWaiter(0), m_bDetached(false)
{
  if (pParent == 0)
  {
    FATAL("Thread::Thread(): Parent process was NULL!");
  }

  // Initialise our kernel stack.
  // m_pKernelStack = VirtualAddressSpace::getKernelAddressSpace().allocateStack();
  m_pAllocatedStack = 0;

  // Initialise state level zero
  allocateStackAtLevel(0);

  m_Id = m_pParent->addThread(this);

  // Now we are ready to go into the scheduler.
  Scheduler::instance().addThread(this, Processor::information().getScheduler());
  Processor::information().getScheduler().addThread(this, state);
}

Thread::~Thread()
{
  if(InputManager::instance().removeCallbackByThread(this))
  {
    WARNING("A thread is being removed, but it never removed itself from InputManager.");
    WARNING("This warning indicates an application or kernel module is buggy!");
  }

  // Before removing from the scheduler, terminate if needed.
  if (!m_bRemovingRequests)
  {
      shutdown();
  }

  // Remove us from the scheduler.
  Scheduler::instance().removeThread(this);

  if (m_pParent)
      m_pParent->removeThread(this);

  // TODO delete any pointer data.

  //if (m_pAllocatedStack)
  //  VirtualAddressSpace::getKernelAddressSpace().freeStack(m_pAllocatedStack);

  for(size_t i = 0; i < MAX_NESTED_EVENTS; i++)
  {
    if(m_StateLevels[i].m_pKernelStack)
        VirtualAddressSpace::getKernelAddressSpace().freeStack(m_StateLevels[i].m_pKernelStack);
    else if(m_StateLevels[i].m_pAuxillaryStack)
        VirtualAddressSpace::getKernelAddressSpace().freeStack(m_StateLevels[i].m_pAuxillaryStack);
    if (m_StateLevels[i].m_pUserStack && m_pParent)
        // Can't use Processor::getCurrent.. as by the time we're called
        // we may have switched address spaces to allow the thread to die.
        m_pParent->getAddressSpace()->freeStack(m_StateLevels[i].m_pUserStack);
  }
  
  if(m_pTlsBase)
    delete m_pTlsBase;
}

void Thread::shutdown()
{
  // We are now removing requests from this thread - deny any other thread from
  // doing so, as that may invalidate our iterators.
  m_bRemovingRequests = true;

  if(m_PendingRequests.count())
  {
    for(List<RequestQueue::Request *>::Iterator it = m_PendingRequests.begin();
        it != m_PendingRequests.end();
        )
    {
        RequestQueue::Request *pReq = *it;
        RequestQueue *pQueue = pReq->owner;

        if (!pQueue)
        {
            ERROR("Thread::shutdown: request in pending requests list has no owner!");
            ++it;
            continue;
        }

        // Halt the owning RequestQueue while we tweak this request.
        pReq->owner->halt();

        // During the halt, we may have lost a request. Check.
        if (!pQueue->isRequestValid(pReq))
        {
            // Resume queue and skip this request - it's dead.
            /// \todo this may leak the request, if it was not async.
            FATAL("Thread::shutdown: request in pending list was executed during halt.");
            pQueue->resume();
            ++it;
            continue;
        }

        // Check for an already completed request. If we called addRequest, the
        // request will not have been destroyed as the RequestQueue is expecting
        // the calling thread to handle it.
        if(pReq->bCompleted)
        {
            // Only destroy if the refcount allows us to - other threads may be
            // also referencing this request (as RequestQueue has dedup).
            if(pReq->refcnt <= 1)
                delete pReq;
            else
            {
                pReq->refcnt--;

                // Ensure the RequestQueue is not referencing us - we're dying.
                if(pReq->pThread == this)
                    pReq->pThread = 0;
            }
        }
        else
        {
            // Not completed yet and the queue is halted. If there's more than
            // one thread waiting on the request, we can just decrease the
            // refcount and carry on. Otherwise, we can kill off the request.
            if (pReq->refcnt > 1)
            {
                pReq->refcnt--;
                if (pReq->pThread == this)
                    pReq->pThread = 0;
            }
            else
            {
                // Terminate.
                pReq->bReject = true;
                pReq->pThread = 0;
                pReq->mutex.release();
            }
        }

        // Allow the queue to resume operation now.
        pQueue->resume();

        // Remove the request from our internal list.
        it = m_PendingRequests.erase(it);
    }
  }

  // Notify any waiters on this thread.
  if (m_pWaiter) {
    m_pWaiter->getLock().acquire();
    m_pWaiter->setStatus(Thread::Ready);
    m_pWaiter->getLock().release();
  }

  // Mark us as waiting for a join if we aren't detached. This ensures that join
  // will not block waiting for this thread if it is called after this point.
  m_ConcurrencyLock.acquire();
  if (!m_bDetached) {
    m_Status = AwaitingJoin;
  }
  m_ConcurrencyLock.release();
}

void Thread::setStatus(Thread::Status s)
{
  m_Status = s;
  m_pScheduler->threadStatusChanged(this);

  if(s == Thread::Zombie)
  {
    // Notify parent process we have become a zombie.
    // We do this here to avoid an amazing race between calling notifyWaiters
    // and scheduling a process into the Zombie state that can cause some
    // processes to simply never be reaped.
    if(m_pParent)
    {
        m_pParent->notifyWaiters();
    }
  }
}

void Thread::threadExited()
{
  Processor::information().getScheduler().killCurrentThread();
}

void Thread::allocateStackAtLevel(size_t stateLevel)
{
    if(stateLevel >= MAX_NESTED_EVENTS)
        stateLevel = MAX_NESTED_EVENTS - 1;
    if(m_StateLevels[stateLevel].m_pKernelStack == 0)
        m_StateLevels[stateLevel].m_pKernelStack = VirtualAddressSpace::getKernelAddressSpace().allocateStack();
//    if(m_StateLevels[stateLevel].m_pUserStack == 0)
//        m_StateLevels[stateLevel].m_pUserStack = m_pParent->getAddressSpace()->allocateStack();
}

void *Thread::getKernelStack()
{
    if(m_nStateLevel >= MAX_NESTED_EVENTS)
        FATAL("m_nStateLevel > MAX_NESTED_EVENTS: " << m_nStateLevel << "...");
    return m_StateLevels[m_nStateLevel].m_pKernelStack;
}

void Thread::setKernelStack()
{
    if(m_StateLevels[m_nStateLevel].m_pKernelStack)
        Processor::information().setKernelStack(reinterpret_cast<uintptr_t>(m_StateLevels[m_nStateLevel].m_pKernelStack));
}

void Thread::sendEvent(Event *pEvent)
{
    LockGuard<Spinlock> guard(m_Lock);

    m_EventQueue.pushBack(pEvent);
    // NOTICE("Sending event: " << pEvent->getNumber() << ".");
    if (m_Status == Sleeping)
    {
        // Interrupt the sleeping thread, there's an event firing
        m_Status = Ready;
        // NOTICE("Set status");

        // Notify the scheduler that we're now ready, so we get put into the
        // scheduling algorithm's ready queue.
        Scheduler::instance().threadStatusChanged(this);
        // NOTICE("Notified the scheduler that we've changed status");
    }
}

void Thread::inhibitEvent(size_t eventNumber, bool bInhibit)
{
    LockGuard<Spinlock> guard(m_Lock);
    if (bInhibit)
        m_StateLevels[m_nStateLevel].m_InhibitMask.set(eventNumber);
    else
        m_StateLevels[m_nStateLevel].m_InhibitMask.clear(eventNumber);
}

void Thread::cullEvent(Event *pEvent)
{
    LockGuard<Spinlock> guard(m_Lock);

    for (List<Event*>::Iterator it = m_EventQueue.begin();
         it != m_EventQueue.end();
         it++)
    {
        if (*it == pEvent)
        {
            if ((*it)->isDeletable())
                delete (*it);
            it = m_EventQueue.erase(it);
        }
    }
}

void Thread::cullEvent(size_t eventNumber)
{
    LockGuard<Spinlock> guard(m_Lock);

    for (List<Event*>::Iterator it = m_EventQueue.begin();
         it != m_EventQueue.end();
         )
    {
        if ((*it)->getNumber() == eventNumber)
        {
            if ((*it)->isDeletable())
                delete (*it);
            it = m_EventQueue.erase(it);
        }
        else
            ++it;
    }
}

Event *Thread::getNextEvent()
{
    LockGuard<Spinlock> guard(m_Lock);

    for (size_t i = 0; i < m_EventQueue.count(); i++)
    {
        Event *e = m_EventQueue.popFront();
        if(!e)
        {
            ERROR("A null event was in a thread's event queue!");
            continue;
        }

        if (m_StateLevels[m_nStateLevel].m_InhibitMask.test(e->getNumber()) ||
            (e->getSpecificNestingLevel() != ~0UL &&
             e->getSpecificNestingLevel() != m_nStateLevel))
            m_EventQueue.pushBack(e);
        else
            return e;
    }

    return 0;
}

bool Thread::hasEvents()
{
    return m_EventQueue.count() != 0;
}

void Thread::addRequest(RequestQueue::Request *req)
{
    if(m_bRemovingRequests)
        return;

    m_PendingRequests.pushBack(req);
}

void Thread::removeRequest(RequestQueue::Request *req)
{
    if(m_bRemovingRequests)
        return;

    for(List<RequestQueue::Request *>::Iterator it = m_PendingRequests.begin();
        it != m_PendingRequests.end();
        it++)
    {
        if(req == *it)
        {
            m_PendingRequests.erase(it);
            return;
        }
    }
}

void Thread::unexpectedExit()
{
    NOTICE("Thread::unexpectedExit");
    if(m_bRemovingRequests)
        return;

#if 0
    for(List<RequestQueue::Request *>::Iterator it = m_PendingRequests.begin();
        it != m_PendingRequests.end();
        it++)
    {
        (*it)->bReject = true;
        (*it)->mutex.release();
    }
#endif

    NOTICE("Thread::unexpectedExit COMPLETE");
}

uintptr_t Thread::getTlsBase()
{
    if(!m_StateLevels[0].m_pKernelStack)
        return 0;

    // Solves a problem where threads are created pointing to different address
    // spaces than the process that creates them (for whatever reason). Because
    // this is usually only called right after the address space switch in
    // PerProcessorScheduler, the address space is set properly.
    if(!m_pTlsBase)
    {
        // Allocate space for this thread's TLS region
        m_pTlsBase = new MemoryRegion("thread-local-storage");
        if(!PhysicalMemoryManager::instance().allocateRegion(
                                        *m_pTlsBase,
                                        THREAD_TLS_SIZE / 0x1000,
                                        0,
                                        VirtualAddressSpace::Write))
        {
          delete m_pTlsBase;
          m_pTlsBase = 0;
          
          return 0;
        }
        else
        {
          NOTICE("Thread [" << Dec << m_Id << Hex << "]: allocated TLS area at " << reinterpret_cast<uintptr_t>(m_pTlsBase->virtualAddress()) << ".");
          
          uint32_t *tlsBase = reinterpret_cast<uint32_t*>(m_pTlsBase->virtualAddress());
          *tlsBase = static_cast<uint32_t>(m_Id);
        }
    }
    return reinterpret_cast<uintptr_t>(m_pTlsBase->virtualAddress());
}

bool Thread::join()
{
  Thread *pThisThread = Processor::information().getCurrentThread();

  m_ConcurrencyLock.acquire();

  // Can't join a detached thread.
  if (m_bDetached)
  {
    m_ConcurrencyLock.release();
    return false;
  }

  // Check thread state. Perhaps the join is just a matter of terminating this
  // thread, as it has died.
  if (m_Status != AwaitingJoin)
  {
    if (m_pWaiter)
    {
      // Another thread is already join()ing.
      m_ConcurrencyLock.release();
      return false;
    }

    m_pWaiter = pThisThread;
    pThisThread->setDebugState(Joining, reinterpret_cast<uintptr_t>(__builtin_return_address(0)));
    m_ConcurrencyLock.release();

    while (1)
    {
      Processor::information().getScheduler().sleep(0);
      if (!(pThisThread->wasInterrupted() || pThisThread->getUnwindState() != Thread::Continue))
        break;
    }

    pThisThread->setDebugState(None, 0);
  }
  else
  {
    m_ConcurrencyLock.release();
  }

  // Thread has terminated, we may now clean up.
  delete this;
  return true;
}

bool Thread::detach()
{
  if (m_Status == AwaitingJoin)
  {
    WARNING("Thread::detach() called on a thread that has already exited.");
    return join();
  }
  else
  {
    LockGuard<Spinlock> guard(m_ConcurrencyLock);

    if (m_pWaiter) {
      ERROR("Thread::detach() called while other threads are joining.");
      return false;
    }

    m_bDetached = true;
    return true;
  }
}

#endif // THREADS
