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
    NOTICE("PPSched::initialise");
    m_pSchedulingAlgorithm = new RoundRobin();

    pThread->setStatus(Thread::Running);
    Processor::information().setCurrentThread(pThread);

    m_pSchedulingAlgorithm->addThread(pThread);
    Processor::information().setKernelStack( reinterpret_cast<uintptr_t> (pThread->getKernelStack()) );

    Machine::instance().getSchedulerTimer()->registerHandler(this);
}

void PerProcessorScheduler::schedule(Thread::Status nextStatus)
{
    // A spinlock with lockguard is the easiest way of disabling interrupts.
    Spinlock lock;
    LockGuard<Spinlock> guard(lock);

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
        return;
    }

    // Now neither thread can be moved, we're safe to switch.

    pCurrentThread->setStatus(nextStatus);
    pNextThread->setStatus(Thread::Running);
    Processor::information().setCurrentThread(pNextThread);
    Processor::information().setKernelStack( reinterpret_cast<uintptr_t> (pNextThread->getKernelStack()) );
    Processor::switchAddressSpace( *pNextThread->getParent()->getAddressSpace() );

    checkSignalState(pNextThread);

    pNextThread->getLock().release();

    contextSwitch(pCurrentThread->state(), pNextThread->state(),
                  pCurrentThread->getLock().m_Atom.m_Atom);
}

void PerProcessorScheduler::checkSignalState(Thread *pThread)
{
}

void PerProcessorScheduler::signalHandlerReturned()
{
}

void PerProcessorScheduler::addThread(Thread *pThread, Thread::ThreadStartFunc pStartFunction, void *pParam, bool bUsermode, void *pStack)
{
    NOTICE("PPSched::addThread");

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
    pThread->getLock().release();

    launchThread(pCurrentThread->state(),
                 pCurrentThread->getLock().m_Atom.m_Atom,
                 reinterpret_cast<uintptr_t>(pStack),
                 reinterpret_cast<uintptr_t>(pStartFunction),
                 reinterpret_cast<uintptr_t>(pParam),
                 (bUsermode) ? 1 : 0);
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
    pThread->getLock().release();

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
