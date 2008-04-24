/*
 * Copyright (c) 2008 James Molloy, James Pritchett, Jörg Pfähler, Matthew Iselin
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
#include <process/RoundRobin.h>
#include <process/Thread.h>
#include <Log.h>

RoundRobin::RoundRobin() :
    m_List()
{
}

RoundRobin::~RoundRobin()
{
}

void RoundRobin::addThread(Thread *pThread)
{
  // TODO ZOMG TEH LOCKZ
  m_List.pushBack(pThread);
}

void RoundRobin::removeThread(Thread *pThread)
{
  // TODO ZOMG TEH LOCKZ
  for(ThreadList::Iterator it = m_List.begin();
      it != m_List.end();
      it++)
  {
    if (*it == pThread)
    {
      m_List.erase (it);
      return;
    }
  }
  ERROR ("RoundRobin::removeThread called for nonexistant thread " << Hex <<
          reinterpret_cast<uintptr_t> (pThread));
}

Thread *RoundRobin::getNext(Processor *pProcessor)
{
  // TODO ZOMG TEH LOCKZ
  Thread *pThread;
  do
  {
    pThread = m_List.popFront();
    m_List.pushBack(pThread);
  } while (pThread->getStatus() != Thread::Ready);
  return pThread;
}

void RoundRobin::threadStatusChanged(Thread *pThread)
{
}
