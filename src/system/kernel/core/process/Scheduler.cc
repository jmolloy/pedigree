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

#include <process/Scheduler.h>
#include <process/SchedulingAlgorithm.h>
#include <process/Thread.h>
#include <process/RoundRobin.h>
#include <process/initialiseMultitasking.h>
#include <processor/Processor.h>
#include <processor/StackFrame.h>
#include <Debugger.h>
#include <machine/Machine.h>
#include <panic.h>
#include <Log.h>

Scheduler Scheduler::m_Instance;

Scheduler::Scheduler() :
  m_Mutex(), m_pSchedulingAlgorithm(0), m_Processes(), m_NextPid(0)
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
  Processor::information().setCurrentThread(pThread);

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

size_t Scheduler::addProcess(Process *pProcess)
{
  m_Processes.pushBack(pProcess);
  return (m_NextPid += 1);
}

void Scheduler::removeProcess(Process *pProcess)
{
  for(Vector<Process*>::Iterator it = m_Processes.begin();
      it != m_Processes.end();
      it++)
  {
    if (*it == pProcess)
    {
      m_Processes.erase(it);
      break;
    }
  }
}

void Scheduler::threadStatusChanged(Thread *pThread)
{
  if (m_pSchedulingAlgorithm)
    m_pSchedulingAlgorithm->threadStatusChanged(pThread);
}

void Scheduler::schedule(Processor *pProcessor, InterruptState &state, Thread *pThread)
{
  if (pThread == 0)
    pThread = m_pSchedulingAlgorithm->getNext(pProcessor);
  Thread * const pOldThread = Processor::information().getCurrentThread();

  m_Mutex.acquire();

  pOldThread->setStatus(Thread::Ready);

  pThread->setStatus(Thread::Running);
  Processor::information().setCurrentThread(pThread);

  Processor::information().setKernelStack( reinterpret_cast<uintptr_t> (pThread->getKernelStack()) );
  Processor::switchAddressSpace( *pThread->getParent()->getAddressSpace() );
  VirtualAddressSpace::setCurrentAddressSpace( pThread->getParent()->getAddressSpace() );

  pOldThread->setInterruptState(&state);
  pOldThread->state().setStackPointer(Processor::getStackPointer());
  pOldThread->state().setBasePointer(Processor::getBasePointer());
  pOldThread->state().setInstructionPointer(Processor::getInstructionPointer());
  
  if (Processor::information().getCurrentThread() == pThread)
    Processor::contextSwitch(pThread->state());

  m_Mutex.release();
}

void Scheduler::switchToAndDebug(InterruptState &state, Thread *pThread)
{
  Thread * const pOldThread = Processor::information().getCurrentThread();

  // We shouldn't need the mutex, as we're debugging, so all processors are halted.
  //m_Mutex.acquire();

  pOldThread->setStatus(Thread::Ready);

  pThread->setStatus(Thread::Running);
  Processor::information().setCurrentThread(pThread);

  Processor::information().setKernelStack( reinterpret_cast<uintptr_t> (pThread->getKernelStack()) );
  Processor::switchAddressSpace( *pThread->getParent()->getAddressSpace() );
  VirtualAddressSpace::setCurrentAddressSpace( pThread->getParent()->getAddressSpace() );

  Debugger::instance().m_pTempState = pThread->getInterruptState();
  StackFrame::construct(pThread->state(), pThread->state().getInstructionPointer(), 0);
  pThread->state().setInstructionPointer(reinterpret_cast<uintptr_t> (&Debugger::switchedThread));

  pOldThread->setInterruptState(&state);
  pOldThread->state().setStackPointer(Processor::getStackPointer());
  pOldThread->state().setBasePointer(Processor::getBasePointer());
  pOldThread->state().setInstructionPointer(Processor::getInstructionPointer());

  if (Processor::information().getCurrentThread() == pThread)
    Processor::contextSwitch(pThread->state());

  Machine::instance().getIrqManager()->acknowledgeIrq(0x20);
}

void Scheduler::timer(uint64_t delta, InterruptState &state)
{
  // TODO processor not passed.
  schedule(0, state);
}

size_t Scheduler::getNumProcesses()
{
  return m_Processes.count();
}

Process *Scheduler::getProcess(size_t n)
{
  if (n >= m_Processes.count())
  {
    WARNING("Scheduler::getProcess(" << Dec << n << ") parameter outside range.");
    return 0;
  }
  return m_Processes[n];
}

#endif
