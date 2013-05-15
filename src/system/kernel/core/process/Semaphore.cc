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

/// \note The implementation of Semaphore is removed when threads are disabled
///       but the definition of Semaphore is left intact. This reduces the
///       number of #ifdef's required around the place and also throws an error
///       when Semaphores are compiled in a non-threaded environment.
#ifdef THREADS

#include <Log.h>
#include <machine/Machine.h>
#include <machine/Timer.h>
#include <process/Scheduler.h>
#include <process/Semaphore.h>
#include <processor/Processor.h>

#include <utilities/assert.h>

void interruptSemaphore(uint8_t *pBuffer)
{
    Processor::information().getCurrentThread()->setInterrupted(true);
}

Semaphore::SemaphoreEvent::SemaphoreEvent() :
    Event(reinterpret_cast<uintptr_t>(&interruptSemaphore), false /* Not deletable */)
{
}

Semaphore::Semaphore(size_t nInitialValue)
    : magic(0xdeadbaba), m_Counter(nInitialValue), m_BeingModified(false), m_Queue()
{
    assert(magic == 0xdeadbaba);
}

Semaphore::~Semaphore()
{
    assert(magic == 0xdeadbaba);
}

void Semaphore::removeThread(Thread *pThread)
{
    m_BeingModified.acquire();
    for(List<Thread*>::Iterator it = m_Queue.begin(); it != m_Queue.end(); ++it)
    {
        if((*it) == pThread)
        {
            m_Queue.erase(it);
            break; /// \note Assume no other pointers that match
        }
    }
    m_BeingModified.release();
}

bool Semaphore::acquire(size_t n, size_t timeoutSecs, size_t timeoutUsecs)
{
    if(magic != 0xdeadbaba)
    {
        NOTICE(magic);
        assert(false);
    }
  // Spin 10 times in the case that the lock is about to be released on
  // multiprocessor systems, and just once for uniprocessor systems, so we don't
  // go through the rigmarole of creating a timeout event if the lock is
  // available.
#ifdef MULTIPROCESSOR
  for (int i = 0; i < 10; i++)
#endif
    if (tryAcquire(n))
      return true;

  // If we have a timeout, create the event and register it.
  Event *pEvent = 0;
  if (timeoutSecs || timeoutUsecs)
  {
      pEvent = new SemaphoreEvent();
      Machine::instance().getTimer()->addAlarm(pEvent, timeoutSecs, timeoutUsecs);
  }

  while (true)
  {
    Thread *pThread = Processor::information().getCurrentThread();
    if (tryAcquire(n))
    {
      if (pEvent)
      {
          Machine::instance().getTimer()->removeAlarm(pEvent);
          delete pEvent;
      }

      removeThread(pThread);
      return true;
    }

    m_BeingModified.acquire();
    bool bWasInterrupts = m_BeingModified.interrupts();

    // To avoid a race condition, check again here after we've disabled interrupts.
    // This stops the condition where the lock is released after tryAcquire returns false,
    // but before we grab the "being modified" lock, which means the lock could be released by this point!
    if (tryAcquire(n))
    {
      if (pEvent)
      {
        Machine::instance().getTimer()->removeAlarm(pEvent);
        delete pEvent;
      }
      m_BeingModified.release();
      removeThread(pThread);
      return true;
    }

    m_Queue.pushBack(pThread);

    pThread->setInterrupted(false);
    pThread->setDebugState(Thread::SemWait, reinterpret_cast<uintptr_t>(__builtin_return_address(0)));
    Processor::information().getScheduler().sleep(&m_BeingModified);
    pThread->setDebugState(Thread::None, 0);

    // Either acquired or interrupted, either way, we don't need to be woken again
    removeThread(pThread);

    // Why were we woken?
    if (pThread->wasInterrupted() || pThread->getUnwindState() != Thread::Continue)
    {
        // We were deliberately interrupted - most likely because of a timeout.
        if (pEvent)
        {
          Machine::instance().getTimer()->removeAlarm(pEvent);
          delete pEvent;
        }

        // Restore interrupt state. It turns out that we can sometimes come
        // back without interrupts enabled in some circumstances when they were
        // enabled previously, which is a terrible side-effect for a timed-out
        // Semaphore acquire to have.
        Processor::setInterrupts(bWasInterrupts);
        return false;
    }
  }

}

bool Semaphore::tryAcquire(size_t n)
{
  ssize_t value = m_Counter;

  if ((value - static_cast<ssize_t>(n)) < 0)return false;
  if (m_Counter.compareAndSwap(value, value - n))
  {
    #ifdef STRICT_LOCK_ORDERING
      // TODO LockManager::acquired(*this);
    #endif
    return true;
  }
  return false;
}

void Semaphore::release(size_t n)
{
    assert(magic == 0xdeadbaba);
  m_Counter += n;

  m_BeingModified.acquire();

  while(m_Queue.count() != 0)
  {
    Thread *pThread = m_Queue.popFront();
    if(!pThread)
    {
        WARNING("Null thread in a Semaphore thread queue");
        continue;
    }
    else if(!Scheduler::instance().threadInSchedule(pThread))
    {
        WARNING("A thread that was to be woken by a Semaphore is no longer in the scheduler");
        continue;
    }
    else if(pThread->getStatus() != Thread::Sleeping)
    {
        if(pThread->getStatus() == Thread::Zombie)
            WARNING("Semaphore has a zombie thread in its thread queue");
        continue;
    }

    pThread->getLock().acquire();
    pThread->setStatus(Thread::Ready);
    pThread->getLock().release();
  }

  m_BeingModified.release();

  #ifdef STRICT_LOCK_ORDERING
    // TODO LockManager::released(*this);
  #endif
}

ssize_t Semaphore::getValue()
{
    return static_cast<ssize_t>(m_Counter);
}

#endif
