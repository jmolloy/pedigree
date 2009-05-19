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
#include <processor/PhysicalMemoryManager.h>
#include <processor/VirtualAddressSpace.h>

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

    pNextThread->getLock().release();

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
        ERROR("checkEventState: Handler address not mapped!");
        //eventHandlerReturned();
        return;
    }
    physical_uintptr_t page;
    size_t flags;
    va.getMapping(reinterpret_cast<void*>(handlerAddress), page, flags);
    if(!(flags & VirtualAddressSpace::KernelMode))
    {
      if(userStack == 0)
      {
          NOTICE("No user stack for usermode event in checkEventState");
          pThread->sendEvent(pEvent);
          return;
      }
    }

    SchedulerState &oldState = pThread->pushState();

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
        va.map(p, reinterpret_cast<void*>(addr), 0);
    }

    pEvent->serialize(reinterpret_cast<uint8_t*>(addr));

    NOTICE("Saving state, loc = " << reinterpret_cast<uintptr_t>(&oldState) << "...");

    /** I've included the debugging output from a bad run inline with the NOTICEs **/

    uint32_t whee1, whee2;
    asm volatile("mov 4(%%ebp), %0" : "=r" (whee1));
    NOTICE("Returning to = " << whee1 << ".");
    asm volatile("mov (%%ebp), %0" : "=r" (whee2));
    NOTICE("Old EBP = " << whee2 << ".");

    /*
    (NN) Returning to = 0xfa0873a9.
    (NN) Old EBP = 0xff39bf84.
    */

    uint32_t esp, ebp;
    asm volatile("mov %%esp, %0" : "=r" (esp));
    asm volatile("mov %%ebp, %0" : "=r" (ebp));
    NOTICE("1. esp = " << esp << ", ebp = " << ebp << ".");

    /*
    (NN) 1. esp = 0xff39befc, ebp = 0xff39bf54.
    */

    if (Processor::saveState(oldState))
    {
        uint32_t esp2, ebp2;
        asm volatile("mov %%esp, %0" : "=r" (esp2));
        asm volatile("mov %%ebp, %0" : "=r" (ebp2));
        NOTICE("2. esp = " << esp2 << ", ebp = " << ebp2 << ".");
        NOTICE("3. esp = " << esp << ", ebp = " << ebp << ".");

        /*
        (NN) 2. esp = 0xff39bf0c, ebp = 0xff39bf54.
        (NN) 3. esp = 0x10, ebp = 0xff39bf44.
        */

        /** The 0x10 between the current stack and the previous stack is
          * correct. The value for the previous stack variables however is
          * not.
          *
          * If, instead of calling jumpUser below, eventHandlerReturned is
          * called, the three dumps are:
          * (NN) 1. esp = 0xff39bf0c, ebp = 0xff39bf54.
          * (NN) 2. esp = 0xff39bf1c, ebp = 0xff39bf54.
          * (NN) 3. esp = 0xff39bf0c, ebp = 0xff39bf54.
          *
          * Somewhere the stack is being corrupted, it would appear.
          */

        // Just context-restored.
        NOTICE("huzzah");
        Processor::setInterrupts(bWasInterrupts);
        NOTICE("current stack is safe, returning!");

        return;
    }

    if (pEvent->isDeletable())
        delete pEvent;

    if (flags & VirtualAddressSpace::KernelMode)
    {
        void (*fn)(size_t) = reinterpret_cast<void (*)(size_t)>(handlerAddress);
        NOTICE("Calling fn " << handlerAddress);
        fn(addr);
        pThread->popState();
        NOTICE("CheckEventState returning");
        return;
    }
    else if (userStack != 0)
    {
        NOTICE("addr = " << addr << ".");
        eventHandlerReturned();
        Processor::jumpUser(0, EVENT_HANDLER_TRAMPOLINE, userStack, handlerAddress, addr);
        // Not reached.
    }
    else
    {
        NOTICE("Fail");
    }
}

void PerProcessorScheduler::eventHandlerReturned()
{
    Processor::setInterrupts(false);

    NOTICE("here");

    Thread *pThread = Processor::information().getCurrentThread();
    NOTICE("here2");
    pThread->popState();

    NOTICE("here3");
    Processor::restoreState(pThread->state());
    FATAL("eventHandlerReturned() - restoreState returned!?");
    // Not reached.
}

void PerProcessorScheduler::addThread(Thread *pThread, Thread::ThreadStartFunc pStartFunction, void *pParam, bool bUsermode, void *pStack)
{
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
        Processor::jumpKernel(&pCurrentThread->getLock().m_Atom.m_Atom,
                              reinterpret_cast<uintptr_t>(pStartFunction),
                              reinterpret_cast<uintptr_t>(pStack),
                              reinterpret_cast<uintptr_t>(pParam));
}

void PerProcessorScheduler::addThread(Thread *pThread, SyscallState &state)
{
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
