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

#if defined(THREADS)
#include <process/RoundRobin.h>
#include <process/Thread.h>
#include <processor/Processor.h>
#include <Log.h>
#include <LockGuard.h>
#include <utilities/assert.h>

RoundRobin::RoundRobin() :
  m_Lock(false)
{
}

RoundRobin::~RoundRobin()
{
}

void RoundRobin::addThread(Thread *pThread)
{
}

void RoundRobin::removeThread(Thread *pThread)
{
  LockGuard<Spinlock> guard(m_Lock);

  for (size_t i = 0; i < MAX_PRIORITIES; i++)
  {
      for(ThreadList::Iterator it = m_pReadyQueues[i].begin();
          it != m_pReadyQueues[i].end();
          it++)
      {
          if (*it == pThread)
          {
              m_pReadyQueues[i].erase (it);
              return;
          }
      }
  }
}

Thread *RoundRobin::getNext(Thread *pCurrentThread)
{
    LockGuard<Spinlock> guard(m_Lock);

    Thread *pThread = 0;
    for (size_t i = 0; i < MAX_PRIORITIES; i++)
    {
        if (m_pReadyQueues[i].size())
        {
            pThread = m_pReadyQueues[i].popFront();
            if(pThread == pCurrentThread)
                continue;

            if (pThread)
            {
                // Sanity check
                if(pThread != pCurrentThread)
                    pThread->getLock().acquire();
                return pThread;
            }
        }
    }
    return 0;
}

void RoundRobin::threadStatusChanged(Thread *pThread)
{
    if (pThread->getStatus() == Thread::Ready)
    {
        assert (pThread->getPriority() < MAX_PRIORITIES);
        
        for(List<Thread*>::Iterator it = m_pReadyQueues[pThread->getPriority()].begin(); it != m_pReadyQueues[pThread->getPriority()].end(); ++it)
        {
            if((*it) == pThread)
            {
                // WARNING("RoundRobin: A thread was already in this priority queue");
                return;
            }
        }

        m_pReadyQueues[pThread->getPriority()].pushBack(pThread);
    }
}

#endif
