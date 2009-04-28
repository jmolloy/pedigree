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

#include <process/PerProcessorScheduler.h>
#include <process/Thread.h>
#include <process/SchedulingAlgorithm.h>
#include <process/RoundRobin.h>

#include <processor/Processor.h>

#include <machine/Machine.h>

#include <Spinlock.h>
#include <LockGuard.h>
#include <panic.h>
#include <Log.h>

PerProcessorScheduler::PerProcessorScheduler() :
    m_pSchedulingAlgorithm(0)
{
}

PerProcessorScheduler::~PerProcessorScheduler()
{
}

void PerProcessorScheduler::initialise(Thread *pThread)
{
    m_pSchedulingAlgorithm = new RoundRobin();

    pThread->setStatus(Thread::Running);
    Processor::information().setCurrentThread(pThread);

    m_pSchedulingAlgorithm->addThread(pThread);
    Processor::information().setKernelStack( reinterpret_cast<uintptr_t> (pThread->getKernelStack()) );

    Machine::instance().getSchedulerTimer()->registerHandler(this);
}

void PerProcessorScheduler::schedule(Thread::Status nextStatus, Spinlock *pLock)
{
    bool bWasInterrupts = Processor::getInterrupts();
    Processor::setInterrupts(false);

    Thread *pCurrentThread = Processor::information().getCurrentThread();

    // Grab the current thread's lock.
    pCurrentThread->getLock().acquire();

    // Now attempt to get another thread to run.
    // This will also get the lock for the returned thread.
    Thread *pNextThread = m_pSchedulingAlgorithm->getNext();

    if (pNextThread == 0)
    {
        // Nothing to switch to, just return.
        pCurrentThread->getLock().release();
        Processor::setInterrupts(bWasInterrupts);
        return;
    }

    // Now neither thread can be moved, we're safe to switch.

    pCurrentThread->setStatus(nextStatus);
    pNextThread->setStatus(Thread::Running);
    Processor::information().setCurrentThread(pNextThread);
    Processor::information().setKernelStack( reinterpret_cast<uintptr_t> (pNextThread->getKernelStack()) );
    Processor::switchAddressSpace( *pNextThread->getParent()->getAddressSpace() );

    if (pLock)
    {
        // We cannot call ->release() here, because this lock was grabbed
        // before we disabled interrupts, so it may re-enable interrupts.
        // And that would be a very bad thing.
        //
        // We instead store the interrupt state of the spinlock, and manually
        // unlock it.
        if (pLock->m_bInterrupts)
            bWasInterrupts = true;
        pLock->m_Atom.m_Atom = 1;
    }

    checkSignalState(pNextThread);

    pNextThread->getLock().release();

    contextSwitch(pCurrentThread->state(), pNextThread->state(),
                  pCurrentThread->getLock().m_Atom.m_Atom);

    // Return to previous interrupt state.
    Processor::setInterrupts(bWasInterrupts);
}

void PerProcessorScheduler::checkSignalState(Thread *pThread)
{
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
      // We have a signal to execute...
      pThread->setIsInSigHandler(true);

      // Set the signal handler state (state() returns a reference to the
      // signal state if we're in a signal handler as set above)
      SchedulerState newState = pThread->state();

/** \note This is the old way of doing things. We need to return to a specific address
          after executing the handler. This is supposed to allow that.

#ifdef X86_COMMON
      uintptr_t esp = newState.getStackPointer() - sizeof(InterruptState) - 4;
      *reinterpret_cast<uint32_t*>(esp) = pThread->getParent()->getSigReturnStub();
      newState.setStackPointer(esp);
#endif
#ifdef PPC_COMMON
      newState.m_Lr = pThread->getParent()->getSigReturnStub();
#endif

      newState.setInstructionPoint(sig->sigLocation);

*/

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

      // save the old state, because we're moving to a new state
      /*InterruptState* currState = pThread->getInterruptState();
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
      delete sig;*/
    }
  }

  // If we need to use the saved state, load it as the current state. This allows the
  // thread to run without using an invalid state (as would happen if the state from
  // the assignment earlier was kept.)
  /*if(pThread->shouldUseSaved())
  {
    pThread->setInterruptState(pThread->getSavedInterruptState());
    pThread->useSaved(false);
  }*/
}

void PerProcessorScheduler::signalHandlerReturned()
{
  // We have a signal to execute...
  Thread *pThread = Processor::information().getCurrentThread();
  pThread->setIsInSigHandler(true);

  NOTICE("signal handler returned =O");
}

void PerProcessorScheduler::addThread(Thread *pThread, Thread::ThreadStartFunc pStartFunction, void *pParam, bool bUsermode, void *pStack)
{
    NOTICE("PPSched::addThread");

    bool bWasInterrupts = Processor::getInterrupts();
    Processor::setInterrupts(false);

    // We assume here that pThread's lock is already taken.

    Thread *pCurrentThread = Processor::information().getCurrentThread();

    // Grab the current thread's lock.
    pCurrentThread->getLock().acquire();

    m_pSchedulingAlgorithm->addThread(pThread);

    // Now neither thread can be moved, we're safe to switch.

    pCurrentThread->setStatus(Thread::Ready);
    pThread->setStatus(Thread::Running);
    Processor::information().setCurrentThread(pThread);
    Processor::information().setKernelStack( reinterpret_cast<uintptr_t> (pThread->getKernelStack()) );
    Processor::switchAddressSpace( *pThread->getParent()->getAddressSpace() );

    // This thread is safe from being moved as its status is now "running".
    if (pThread->getLock().m_bInterrupts) bWasInterrupts = true;
    pThread->getLock().m_Atom.m_Atom = 1;

    launchThread(pCurrentThread->state(),
                 pCurrentThread->getLock().m_Atom.m_Atom,
                 reinterpret_cast<uintptr_t>(pStack),
                 reinterpret_cast<uintptr_t>(pStartFunction),
                 reinterpret_cast<uintptr_t>(pParam),
                 (bUsermode) ? 1 : 0);

    if (bWasInterrupts) Processor::setInterrupts(true);
}

void PerProcessorScheduler::addThread(Thread *pThread, SyscallState &state)
{
    NOTICE("AddThread syscall");

    // A spinlock with lockguard is the easiest way of disabling interrupts.
    Spinlock lock;
    LockGuard<Spinlock> guard(lock);

    // We assume here that pThread's lock is already taken.

    Thread *pCurrentThread = Processor::information().getCurrentThread();

    // Grab the current thread's lock.
    pCurrentThread->getLock().acquire();

    m_pSchedulingAlgorithm->addThread(pThread);

    // Now neither thread can be moved, we're safe to switch.

    pCurrentThread->setStatus(Thread::Ready);
    pThread->setStatus(Thread::Running);
    Processor::information().setCurrentThread(pThread);
    Processor::information().setKernelStack( reinterpret_cast<uintptr_t> (pThread->getKernelStack()) );
    Processor::switchAddressSpace( *pThread->getParent()->getAddressSpace() );

    // This thread is safe from being moved as its status is now "running".
    pThread->getLock().m_Atom.m_Atom = 1;

    // Copy the SyscallState into this thread's kernel stack.
    uintptr_t kStack = reinterpret_cast<uintptr_t>(pThread->getKernelStack());
    kStack -= sizeof(SyscallState);
    memcpy(reinterpret_cast<void*>(kStack), reinterpret_cast<void*>(&state), sizeof(SyscallState));

    launchThread(pCurrentThread->state(),
                 pCurrentThread->getLock().m_Atom.m_Atom,
                 reinterpret_cast<SyscallState&>(kStack));
}


void PerProcessorScheduler::killCurrentThread()
{
    // A spinlock with lockguard is the easiest way of disabling interrupts.
    Spinlock lock;
    LockGuard<Spinlock> guard(lock);

    Thread *pThread = Processor::information().getCurrentThread();

    // Removing the current thread. Grab its lock.
    pThread->getLock().acquire();

    // Get another thread ready to schedule.
    // This will also get the lock for the returned thread.
    Thread *pNextThread = m_pSchedulingAlgorithm->getNext();

    if (pNextThread == 0)
    {
        // Nothing to switch to, we're in a VERY bad situation.
        panic("Attempting to kill only thread on this processor!");
        return;
    }

    pNextThread->setStatus(Thread::Running);
    Processor::information().setCurrentThread(pNextThread);
    Processor::information().setKernelStack( reinterpret_cast<uintptr_t> (pNextThread->getKernelStack()) );
    Processor::switchAddressSpace( *pNextThread->getParent()->getAddressSpace() );

    checkSignalState(pNextThread);

    pNextThread->getLock().release();

    deleteThreadThenContextSwitch(pThread, pNextThread->state());
}

void PerProcessorScheduler::deleteThread(Thread *pThread)
{
    NOTICE("AnuS!");
//     delete pThread;
}

void PerProcessorScheduler::removeThread(Thread *pThread)
{
    m_pSchedulingAlgorithm->removeThread(pThread);
}
