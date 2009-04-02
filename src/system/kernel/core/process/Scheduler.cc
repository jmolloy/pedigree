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
volatile uint32_t bSafeToDisembark = true;
void Scheduler::schedule(Processor *pProcessor, InterruptState &state, Thread *pThread)
{
  m_Mutex.acquire();

  if (pThread == 0)
    pThread = m_pSchedulingAlgorithm->getNext(pProcessor);
  Thread * const pOldThread = Processor::information().getCurrentThread();

  if (pThread == 0)
  {
    if(pOldThread->getParent()->getPendingSignals().count())
      FATAL("Schedule has no threads to switch to yet the process has pending signals.");
    m_Mutex.release();
    return;
  }

  if (pOldThread->getStatus() == Thread::Running)
    pOldThread->setStatus(Thread::Ready);
  else if (pOldThread->getStatus() == Thread::PreSleep)
    pOldThread->setStatus(Thread::Sleeping);

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

  while (bSafeToDisembark == 0) ;

  bSafeToDisembark = 0;
  m_Mutex.release();

#ifdef X86_COMMON
  /// \todo What IRQ do we ACK? It's machine specific.
#ifndef MULTIPROCESSOR
  Machine::instance().getIrqManager()->acknowledgeIrq(0x20);
#else
  LocalApic *lApic = static_cast<LocalApic*>(Machine::instance().getSchedulerTimer());
  lApic->ack();
#endif
#endif

  // find if the process has any queued signals
  List<void*>& sigq = pThread->getParent()->getPendingSignals();
  if(sigq.count())
  {
    // try to find a non-blocked signal
    Process::PendingSignal* sig;
    Process::PendingSignal* firstSig = 0;
    do
    {
      // grab the signal and check against the mask
      sig = reinterpret_cast<Process::PendingSignal*>(sigq.popFront());
      if(firstSig == 0)
        firstSig = sig;

      uint32_t sigmask = pThread->getParent()->getSignalMask();
      if(sigmask & (1 << sig->sig))
      {
        /// \todo This will mean the order gets totally messed up... Not so great!
        sigq.pushBack(sig);

        // Blocked, check the next one. If it's the first signal we popped, we've done a full loop
        // and we know there's no signals we can run.
        sig = reinterpret_cast<Process::PendingSignal*>(sigq.popFront());
        if(sig == firstSig)
        {
          sigq.pushFront(sig);
          sig = 0;
        }
      }
      else
        break;
    }
    while(sig);

    // did we find a non-blocked signal?
    if(sig)
    {
      // save the old state, because we're moving to a new state
      InterruptState* currState = pThread->getInterruptState();
      pThread->setSavedInterruptState(currState);
      ProcessorState procState;

      // setup the return instruction pointer
      /// \todo StackFrame members could make this cleaner
#ifdef X86_COMMON
      uintptr_t esp = currState->getStackPointer() - sizeof(InterruptState) - 4;
      *reinterpret_cast<uint32_t*>(esp) = pThread->getParent()->getSigReturnStub();
      procState.setStackPointer(esp);
#endif
#ifdef PPC_COMMON
      procState.m_Lr = pThread->getParent()->getSigReturnStub();
#endif

      // we're now jumping to the signal handler
      procState.setInstructionPointer(sig->sigLocation);

      // load the new InterruptState into the thread
      InterruptState* retState = currState->construct(procState, false);
      pThread->setInterruptState(retState);

      // we do not use the saved state yet
      pThread->useSaved(false);

      // setup the current signal information structure for the thread
      Thread::CurrentSignal curr;
      curr.bRunning = true;
      curr.currMask = sig->sigMask;
      curr.oldMask = pThread->getParent()->getSignalMask();
      curr.loc = sig->sigLocation;

      // set the process signal mask as per the mask in the PendingSignal structure
      pThread->getParent()->setSignalMask(sig->sigMask);
      pThread->setCurrentSignal(curr);

      // and now we can finally free the pending signal
      delete sig;
    }
  }

  // If we need to use the saved state, load it as the current state. This allows the
  // thread to run without using an invalid state (as would happen if the state from
  // the assignment earlier was kept.)
  if(pThread->shouldUseSaved())
  {
    pThread->setInterruptState(pThread->getSavedInterruptState());
    pThread->useSaved(false);
  }

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
