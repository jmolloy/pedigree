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

#ifdef THREADS

#include <process/PerProcessorScheduler.h>
#include <process/Thread.h>
#include <process/SchedulingAlgorithm.h>
#include <process/RoundRobin.h>

#include <processor/Processor.h>
#include <processor/PhysicalMemoryManager.h>
#include <processor/VirtualAddressSpace.h>

#include <machine/Machine.h>

#include <Spinlock.h>
#include <LockGuard.h>
#include <panic.h>
#include <Log.h>

#include <utilities/assert.h>

#ifdef TRACK_LOCKS
#include <LocksCommand.h>
#endif

PerProcessorScheduler::PerProcessorScheduler() :
    m_pSchedulingAlgorithm(0), m_NewThreadDataLock(false), m_NewThreadDataCount(0),
    m_NewThreadData()
#ifdef ARM_BEAGLE
    , m_TickCount(0)
#endif
{
}

PerProcessorScheduler::~PerProcessorScheduler()
{
}

struct newThreadData
{
    Thread *pThread;
    Thread::ThreadStartFunc pStartFunction;
    void *pParam;
    bool bUsermode;
    void *pStack;
};

int PerProcessorScheduler::processorAddThread(void *instance)
{
    PerProcessorScheduler *pInstance = reinterpret_cast<PerProcessorScheduler*>(instance);
    while(true)
    {
        pInstance->m_NewThreadDataCount.acquire();
        
        pInstance->m_NewThreadDataLock.acquire();
        void *p = pInstance->m_NewThreadData.popFront();
        pInstance->m_NewThreadDataLock.release();
        
        newThreadData *pData = reinterpret_cast<newThreadData*>(p);
        
        pData->pThread->setCpuId(Processor::id());
        pInstance->addThread(pData->pThread, pData->pStartFunction, pData->pParam, pData->bUsermode, pData->pStack);
        
        delete pData;
    }
    return 0;
}

void PerProcessorScheduler::initialise(Thread *pThread)
{
    m_pSchedulingAlgorithm = new RoundRobin();

    pThread->setStatus(Thread::Running);
    pThread->setCpuId(Processor::id());
    Processor::information().setCurrentThread(pThread);

    m_pSchedulingAlgorithm->addThread(pThread);
    Processor::information().setKernelStack( reinterpret_cast<uintptr_t> (pThread->getKernelStack()) );
    Processor::setTlsBase(pThread->getTlsBase());

    Machine::instance().getSchedulerTimer()->registerHandler(this);
    
    new Thread(pThread->getParent(), processorAddThread, reinterpret_cast<void*>(this), 0, false, true);
}

void PerProcessorScheduler::schedule(Thread::Status nextStatus, Thread *pNewThread, Spinlock *pLock)
{
    bool bWasInterrupts = Processor::getInterrupts();
    Processor::setInterrupts(false);

    Thread *pCurrentThread = Processor::information().getCurrentThread();

    // Grab the current thread's lock.
    pCurrentThread->getLock().acquire();

    // Now attempt to get another thread to run.
    // This will also get the lock for the returned thread.
    Thread *pNextThread;
    if(!pNewThread)
    {
        pNextThread = m_pSchedulingAlgorithm->getNext(pCurrentThread);
        if (pNextThread == 0)
        {
            // If we're supposed to be sleeping, this isn't a good place to be
            if(nextStatus != Thread::Ready)
                FATAL("No threads to switch to and the current thread is leaving the ready state!");

            // Nothing to switch to, just return.
            pCurrentThread->getLock().release();
            Processor::setInterrupts(bWasInterrupts);
            return;
        }
    }
    else
    {
        pNextThread = pNewThread;
        if(pNextThread != pCurrentThread)
            pNextThread->getLock().acquire();
    }

    // Now neither thread can be moved, we're safe to switch.
    pCurrentThread->setStatus(nextStatus);
    pNextThread->setStatus(Thread::Running);
    Processor::information().setCurrentThread(pNextThread);

    // Should *never* happen
    if(pLock && (pNextThread->getStateLevel() == reinterpret_cast<uintptr_t>(pLock)))
        FATAL("STATE LEVEL = LOCK PASSED TO SCHEDULER: " << pNextThread->getStateLevel() << "/" << reinterpret_cast<uintptr_t>(pLock) << "!");

    // Load the new kernel stack into the TSS, and the new TLS base and switch address spaces
    Processor::information().setKernelStack( reinterpret_cast<uintptr_t> (pNextThread->getKernelStack()) );
    Processor::switchAddressSpace( *pNextThread->getParent()->getAddressSpace() );
    Processor::setTlsBase(pNextThread->getTlsBase());

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
#ifdef TRACK_LOCKS
        g_LocksCommand.lockReleased(pLock);
#endif
    }

    pNextThread->getLock().release();

    // NOTICE_NOLOCK("calling saveState [schedule]");
    if (Processor::saveState(pCurrentThread->state()))
    {
        // Just context-restored, return.

        // Return to previous interrupt state.
        Processor::setInterrupts(bWasInterrupts);

        // Check the event state - we don't have a user mode stack available
        // to us, so pass zero and don't execute user-mode event handlers.
        checkEventState(0);

        return;
    }

    // Restore context, releasing the old thread's lock when we've switched stacks.
#ifdef TRACK_LOCKS
    g_LocksCommand.lockReleased(&pCurrentThread->getLock());
#endif
    Processor::restoreState(pNextThread->state(), &pCurrentThread->getLock().m_Atom.m_Atom);
    // Not reached.
}

void PerProcessorScheduler::checkEventState(uintptr_t userStack)
{
    bool bWasInterrupts = Processor::getInterrupts();
    Processor::setInterrupts(false);

    Thread *pThread = Processor::information().getCurrentThread();
    if (!pThread) return;

    Event *pEvent = pThread->getNextEvent();
    if (!pEvent)
    {
        Processor::setInterrupts(bWasInterrupts);
        return;
    }

    uintptr_t handlerAddress = pEvent->getHandlerAddress();

    // Simple heuristic for whether to launch the event handler in kernel or user mode - is the
    // handler address mapped kernel or user mode?
    VirtualAddressSpace &va = Processor::information().getVirtualAddressSpace();
    if (!va.isMapped(reinterpret_cast<void*>(handlerAddress)))
    {
        ERROR_NOLOCK("checkEventState: Handler address " << handlerAddress << " not mapped!");
        Processor::setInterrupts(bWasInterrupts);
        return;
    }

    SchedulerState &oldState = pThread->pushState();

    physical_uintptr_t page;
    size_t flags;
    va.getMapping(reinterpret_cast<void*>(handlerAddress), page, flags);
    if(!(flags & VirtualAddressSpace::KernelMode))
    {
      if(userStack != 0)
        va.getMapping(reinterpret_cast<void*>(userStack), page, flags);
      if(userStack == 0 || (flags & VirtualAddressSpace::KernelMode))
      {
          userStack = reinterpret_cast<uintptr_t>(pThread->getStateUserStack());
          if (!userStack)
          {
              // Oops, forgot to add in the actual setting of the userstack. -Matt
              userStack = reinterpret_cast<uintptr_t>(va.allocateStack());
              pThread->setStateUserStack(reinterpret_cast<void*>(userStack));
          }
          else
          {
              // Verify that the stack is mapped
              if(!va.isMapped(reinterpret_cast<void*>(userStack)))
              {
                  /// \todo This is a quickfix for a bigger problem. I imagine
                  ///       it has something to do with calling execve directly
                  ///       without fork, meaning the memory is cleaned up but
                  ///       the state level stack information is *not*.
                  userStack = reinterpret_cast<uintptr_t>(va.allocateStack());
                  pThread->setStateUserStack(reinterpret_cast<void*>(userStack));
              }
          }
      }
      else
      {
          va.getMapping(reinterpret_cast<void*>(userStack), page, flags);
          if(flags & VirtualAddressSpace::KernelMode)
          {
              NOTICE_NOLOCK("User stack for event in checkEventState is the kernel's!");
              pThread->sendEvent(pEvent);
              Processor::setInterrupts(bWasInterrupts);
              return;
          }
      }
    }

    // The address of the serialize buffer is determined by the thread ID and the nesting level.
    uintptr_t addr = EVENT_HANDLER_BUFFER + (pThread->getId() * MAX_NESTED_EVENTS +
                                             (pThread->getStateLevel()-1)) * PhysicalMemoryManager::getPageSize();

    // Ensure the page is mapped.
    if (!va.isMapped(reinterpret_cast<void*>(addr)))
    {
        physical_uintptr_t p = PhysicalMemoryManager::instance().allocatePage();
        if (!p)
        {
            panic("checkEventState: Out of memory!");
            return;
        }
        va.map(p, reinterpret_cast<void*>(addr), VirtualAddressSpace::Write);
    }

    pEvent->serialize(reinterpret_cast<uint8_t*>(addr));

    if (Processor::saveState(oldState))
    {
        // Just context-restored.
        Processor::setInterrupts(bWasInterrupts);
        return;
    }

    if (pEvent->isDeletable())
        delete pEvent;

    if (flags & VirtualAddressSpace::KernelMode)
    {
        void (*fn)(size_t) = reinterpret_cast<void (*)(size_t)>(handlerAddress);
        fn(addr);
        pThread->popState();
        return;
    }
    else if (userStack != 0)
    {
        Processor::jumpUser(0, EVENT_HANDLER_TRAMPOLINE, userStack, handlerAddress, addr);
        // Not reached.
    }
}

void PerProcessorScheduler::eventHandlerReturned()
{
    Processor::setInterrupts(false);

    Thread *pThread = Processor::information().getCurrentThread();
    pThread->popState();

    Processor::restoreState(pThread->state());
    // Not reached.
}

void PerProcessorScheduler::addThread(Thread *pThread, Thread::ThreadStartFunc pStartFunction, void *pParam, bool bUsermode, void *pStack)
{
    if(this != &Processor::information().getScheduler())
    {
        newThreadData *pData = new newThreadData;
        pData->pThread = pThread;
        pData->pStartFunction = pStartFunction;
        pData->pParam = pParam;
        pData->bUsermode = bUsermode;
        pData->pStack = pStack;
        
        m_NewThreadDataLock.acquire();
        m_NewThreadData.pushBack(pData);
        m_NewThreadDataLock.release();
    
        m_NewThreadDataCount.release();
        
        return;
    }
    
    pThread->setCpuId(Processor::id());

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
    Processor::setTlsBase(pThread->getTlsBase());

    // This thread is safe from being moved as its status is now "running".
    if (pThread->getLock().m_bInterrupts) bWasInterrupts = true;
    pThread->getLock().m_Atom.m_Atom = 1;
#ifdef TRACK_LOCKS
    g_LocksCommand.lockReleased(&pThread->getLock());
#endif

    if (Processor::saveState(pCurrentThread->state()))
    {
        // Just context-restored.
        if (bWasInterrupts) Processor::setInterrupts(true);
        return;
    }

    if (bUsermode)
        Processor::jumpUser(&pCurrentThread->getLock().m_Atom.m_Atom,
                            reinterpret_cast<uintptr_t>(pStartFunction),
                            reinterpret_cast<uintptr_t>(pStack),
                            reinterpret_cast<uintptr_t>(pParam));
    else
    {
        Processor::jumpKernel(&pCurrentThread->getLock().m_Atom.m_Atom,
                              reinterpret_cast<uintptr_t>(pStartFunction),
                              reinterpret_cast<uintptr_t>(pStack),
                              reinterpret_cast<uintptr_t>(pParam));
    }
}

void PerProcessorScheduler::addThread(Thread *pThread, SyscallState &state)
{
    pThread->setCpuId(Processor::id());
    
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
    Processor::setTlsBase(pThread->getTlsBase());

    // This thread is safe from being moved as its status is now "running".
    pThread->getLock().m_Atom.m_Atom = 1;

    // Copy the SyscallState into this thread's kernel stack.
    uintptr_t kStack = reinterpret_cast<uintptr_t>(pThread->getKernelStack());
    kStack -= sizeof(SyscallState);
    memcpy(reinterpret_cast<void*>(kStack), reinterpret_cast<void*>(&state), sizeof(SyscallState));

    if (Processor::saveState(pCurrentThread->state()))
    {
        // Just context-restored.
        if (bWasInterrupts) Processor::setInterrupts(true);
        return;
    }

    Processor::restoreState(reinterpret_cast<SyscallState&>(kStack), &pCurrentThread->getLock().m_Atom.m_Atom);
}


void PerProcessorScheduler::killCurrentThread()
{
    Processor::setInterrupts(false);

    Thread *pThread = Processor::information().getCurrentThread();

    // Removing the current thread. Grab its lock.
    pThread->getLock().acquire();

    // If we're tracking locks, don't pollute the results. Yes, we've kept
    // this lock held, but it no longer matters.
#ifdef TRACK_LOCKS
    g_LocksCommand.lockReleased(&pThread->getLock());
#endif

    // Get another thread ready to schedule.
    // This will also get the lock for the returned thread.
    Thread *pNextThread = m_pSchedulingAlgorithm->getNext(pThread);

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

    pNextThread->getLock().release();

    deleteThreadThenRestoreState(pThread, pNextThread->state());
}

void PerProcessorScheduler::deleteThread(Thread *pThread)
{
    delete pThread;
}

void PerProcessorScheduler::removeThread(Thread *pThread)
{
    m_pSchedulingAlgorithm->removeThread(pThread);
}

void PerProcessorScheduler::timer(uint64_t delta, InterruptState &state)
{
#ifdef ARM_BEAGLE // Timer at 1 tick per ms, we want to run every 100 ms
    m_TickCount++;
    if((m_TickCount % 100) == 0)
    {
#endif
        schedule();

        // Check if the thread should exit.
        Thread *pThread = Processor::information().getCurrentThread();
        if (pThread->getUnwindState() == Thread::Exit)
            pThread->getParent()->getSubsystem()->exit(0);
#ifdef ARM_BEAGLE
    }
#endif
}

void PerProcessorScheduler::threadStatusChanged(Thread *pThread)
{
    m_pSchedulingAlgorithm->threadStatusChanged(pThread);
}

#endif
