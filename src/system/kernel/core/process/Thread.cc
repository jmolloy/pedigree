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

#include <process/Thread.h>
#include <process/Scheduler.h>
#include <processor/Processor.h>
#include <processor/StackFrame.h>
#include <machine/Machine.h>
#include <Log.h>

Thread::Thread(Process *pParent, ThreadStartFunc pStartFunction, void *pParam, 
               void *pStack) :
    m_nStateLevel(0), m_pParent(pParent), m_Status(Ready), m_ExitCode(0),  m_pKernelStack(0), m_pAllocatedStack(0), m_Id(0),
    m_Errno(0), m_Lock(), m_EventQueue()
{
  if (pParent == 0)
  {
    FATAL("Thread::Thread(): Parent process was NULL!");
  }

  // Initialise our kernel stack.
  m_pKernelStack = VirtualAddressSpace::getKernelAddressSpace().allocateStack();
  m_pAllocatedStack = m_pKernelStack;

  // If we've been given a user stack pointer, we are a user mode thread.
  bool bUserMode = true;
  if (pStack == 0)
  {
    bUserMode = false;
    pStack = m_pKernelStack;
    m_pKernelStack = 0; // No kernel stack if kernel mode thread - causes bug on PPC
  }

  m_Id = m_pParent->addThread(this);
  
  // Firstly, grab our lock so that the scheduler cannot preemptively load balance
  // us while we're starting.
  m_Lock.acquire();

  // Add us to the general scheduler.
  Scheduler::instance().addThread(this, Processor::information().getScheduler());
  
  // Add us to the per-processor scheduler.
  Processor::information().getScheduler().addThread(this, pStartFunction, pParam, 
                                                    bUserMode, pStack);
}

Thread::Thread(Process *pParent) :
    m_nStateLevel(0), m_pParent(pParent), m_Status(Running), m_ExitCode(0), m_pKernelStack(0), m_pAllocatedStack(0), m_Id(0),
    m_Errno(0), m_Lock(), m_EventQueue()
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
    m_nStateLevel(0), m_pParent(pParent), m_Status(Ready), m_ExitCode(0),  m_pKernelStack(0), m_pAllocatedStack(0), m_Id(0),
    m_Errno(0), m_Lock(), m_EventQueue()
{
  if (pParent == 0)
  {
    FATAL("Thread::Thread(): Parent process was NULL!");
  }

  // Initialise our kernel stack.
  m_pKernelStack = VirtualAddressSpace::getKernelAddressSpace().allocateStack();
  m_pAllocatedStack = m_pKernelStack;

  m_Id = m_pParent->addThread(this);
  
  // Now we are ready to go into the scheduler.
  Scheduler::instance().addThread(this, Processor::information().getScheduler());
  Processor::information().getScheduler().addThread(this, state);
}

Thread::~Thread()
{
  // Remove us from the scheduler.
  Scheduler::instance().removeThread(this);

  m_pParent->removeThread(this);
  
  // TODO delete any pointer data.

  if (m_pAllocatedStack)
    VirtualAddressSpace::getKernelAddressSpace().freeStack(m_pAllocatedStack);
}

void Thread::setStatus(Thread::Status s)
{
  m_Status = s;
//  Scheduler::instance().threadStatusChanged(this);
}

void Thread::threadExited()
{
  NOTICE("Thread exited");
  Processor::information().getScheduler().killCurrentThread();
}

void Thread::sendEvent(Event *pEvent)
{
    LockGuard<Spinlock> guard(m_Lock);

    m_EventQueue.pushBack(pEvent);
    if (m_Status == Sleeping)
    {
        NOTICE("Set ready, thread " << getId());
        m_Status = Ready;
    }
}

void Thread::inhibitEvent(size_t eventNumber, bool bInhibit)
{
    LockGuard<Spinlock> guard(m_Lock);
    if (bInhibit)
        m_InhibitMasks[m_nStateLevel].set(eventNumber);
    else
        m_InhibitMasks[m_nStateLevel].clear(eventNumber);
}

void Thread::cullEvent(Event *pEvent)
{
    LockGuard<Spinlock> guard(m_Lock);
    FATAL("cullEvent");
    for (List<Event*>::Iterator it = m_EventQueue.begin();
         it != m_EventQueue.end();
         it++)
    {
        if (*it == pEvent)
        {
            m_EventQueue.erase(it);
            return;
        }
    }
}

Event *Thread::getNextEvent()
{
    LockGuard<Spinlock> guard(m_Lock);

    for (size_t i = 0; i < m_EventQueue.count(); i++)
    {
        Event *e = m_EventQueue.popFront();
        if (m_InhibitMasks[m_nStateLevel].test(e->getNumber()) ||
            (e->getSpecificNestingLevel() != ~0UL &&
             e->getSpecificNestingLevel() != m_nStateLevel))
            m_EventQueue.pushBack(e);
        else
            return e;
    }

    return 0;
}
