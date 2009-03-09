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
#include <processor/KernelCoreSyscallManager.h>
#include <Debugger.h>
#include <machine/Machine.h>
#include <panic.h>
#include <Log.h>
#include <machine/x86_common/LocalApic.h>

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
  m_pSchedulingAlgorithm = new RoundRobin();

  pThread->setStatus(Thread::Running);
  Processor::information().setCurrentThread(pThread);

  m_pSchedulingAlgorithm->addThread(pThread);
  Processor::information().setKernelStack( reinterpret_cast<uintptr_t> (pThread->getKernelStack()) );

  Machine::instance().getSchedulerTimer()->registerHandler(this);

  return true;
}

#ifdef MULTIPROCESSOR
  void Scheduler::initialiseProcessor(Thread *pThread)
  {
    pThread->setStatus(Thread::Running);
    Processor::information().setCurrentThread(pThread);

    m_pSchedulingAlgorithm->addThread(pThread);

    Processor::information().setKernelStack( reinterpret_cast<uintptr_t> (pThread->getKernelStack()) );
  }
#endif

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
  m_Mutex.acquire();
  for(List<Process*>::Iterator it = m_Processes.begin();
      it != m_Processes.end();
      it++)
  {
    if (*it == pProcess)
    {
      m_Processes.erase(it);
      break;
    }
  }
  m_Mutex.release();
}

void Scheduler::threadStatusChanged(Thread *pThread)
{
  if (m_pSchedulingAlgorithm)
    m_pSchedulingAlgorithm->threadStatusChanged(pThread);
}

void Scheduler::schedule(Processor *pProcessor, InterruptState &state, Thread *pThread)
{
  m_Mutex.acquire();

  if (pThread == 0)
    pThread = m_pSchedulingAlgorithm->getNext(pProcessor);
  Thread * const pOldThread = Processor::information().getCurrentThread();

  if (pThread == 0)
  {
    NOTICE("Fail");
    m_Mutex.release();
    return;
  }

  if (pOldThread->getStatus() == Thread::Running)
    pOldThread->setStatus(Thread::Ready);
  else if (pOldThread->getStatus() == Thread::PreSleep)
  {
    NOTICE("FNARR");
    pOldThread->setStatus(Thread::Sleeping);
  }

  if (pThread->getParent() == 0)
  {
    FATAL ("Address space zero: tid " << Hex << pThread->getId());
  }

  pThread->setStatus(Thread::Running);
  Processor::information().setCurrentThread(pThread);

  Processor::information().setKernelStack( reinterpret_cast<uintptr_t> (pThread->getKernelStack()) );
  Processor::switchAddressSpace( *pThread->getParent()->getAddressSpace() );

  pOldThread->setInterruptState(&state);
  pOldThread->state() = state;

  m_Mutex.release();

#ifdef X86_COMMON
  /// \todo What IRQ do we ACK? It's machine specific.
  //Machine::instance().getIrqManager()->acknowledgeIrq(0x20);
  LocalApic *lApic = static_cast<LocalApic*>(Machine::instance().getSchedulerTimer());
  lApic->ack();
#endif
  Processor::contextSwitch(pThread->getInterruptState());
}

void Scheduler::timer(uint64_t delta, InterruptState &state)
{
  // TODO processor not passed.
  schedule(0, state);
}

void Scheduler::yield(Thread *pThread)
{
  KernelCoreSyscallManager::instance().call(KernelCoreSyscallManager::yield, reinterpret_cast<uintptr_t>(pThread));
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
  size_t i = 0;
  for(List<Process*>::Iterator it = m_Processes.begin();
      it != m_Processes.end();
      it++)
  {
    if (i == n)
      return *it;
    i++;
  }

  return 0;
}

#endif
