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
#include <process/Scheduler.h>
#include <process/SchedulingAlgorithm.h>
#include <process/Thread.h>
#include <process/RoundRobin.h>
#include <process/initialiseMultitasking.h>
#include <processor/Processor.h>
#include <machine/Machine.h>
#include <Log.h>

Scheduler Scheduler::m_Instance;

Scheduler::Scheduler() :
    m_pSchedulingAlgorithm(0)
{
}

Scheduler::~Scheduler()
{
}

bool Scheduler::initialise(Thread *pThread)
{
  NOTICE("Scheduler booting.");
  m_pSchedulingAlgorithm = new RoundRobin();

  pThread->setStatus(Thread::Running);
  g_pCurrentThread = pThread;

  m_pSchedulingAlgorithm->addThread(pThread);
  
  Machine::instance().getSchedulerTimer()->registerHandler(this);
  return true;
}

void Scheduler::addThread(Thread *pThread)
{
  m_pSchedulingAlgorithm->addThread(pThread);
}

void Scheduler::removeThread(Thread *pThread)
{
  m_pSchedulingAlgorithm->removeThread(pThread);
}

void Scheduler::threadStatusChanged(Thread *pThread)
{
  if (m_pSchedulingAlgorithm)
    m_pSchedulingAlgorithm->threadStatusChanged(pThread);
}

void Scheduler::schedule(Processor *pProcessor, ProcessorState &state)
{
  NOTICE("Woot.\n");
  Processor::breakpoint();
  // TODO ZOMG TEH LOCKZ!
//   Thread *pThread = m_pSchedulingAlgorithm->getNext(pProcessor);
  
  // TODO if (pThread == g_pCurrentThread) return;
  
//   g_pCurrentThread->setStatus(Thread::Ready);
//   Processor::saveState (g_pCurrentThread->state());
//   g_pCurrentThread->state().setInstructionPointer (&endOfScheduleLabel);
//   
//   g_pCurrentThread = pThread;
//   g_pCurrentThread->setStatus(Thread::Running);
//   Processor::restoreState(g_pCurrentThread->state());


  // TODO Change VirtualAddressSpace. This will be a member of Process.
}

void Scheduler::timer(uint64_t delta, ProcessorState &state)
{
  // TODO processor not passed.
  schedule(0, state);
}
