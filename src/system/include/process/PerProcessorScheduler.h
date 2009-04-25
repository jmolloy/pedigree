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

#ifndef PERPROCESSORSCHEDULER_H
#define PERPROCESSORSCHEDULER_H

#include <processor/types.h>
#include <processor/state.h>
#include <machine/TimerHandler.h>
#include <process/Mutex.h>
#include <process/Process.h>
#include <process/Thread.h>
#include <process/SchedulerState.h>
#include <Atomic.h>

class SchedulingAlgorithm;
class Processor;
class Thread;
class Spinlock;

class PerProcessorScheduler : public TimerHandler
{
public:
    /** Default constructor - Creates an empty scheduler with a new idle thread. */
    PerProcessorScheduler();

    ~PerProcessorScheduler();

    /** Initialises the scheduler with the given thread. */
    void initialise(Thread *pThread);

    /** Picks another thread to run, if there is one, and switches to it.
        \param nextStatus The thread status to assign the current thread when
                          it is swapped.
        \param pLock      Optional lock to release when the thread is safely
                          locked. */
    void schedule(Thread::Status nextStatus=Thread::Ready, Spinlock *pLock=0);

    /** Assumes this thread has just returned from executing a signal handler,
        and lets it resume normal execution. */
    void signalHandlerReturned();

    /** Adds a new thread.
        \param pThread The thread to add.
        \param pStartFunction The function to start the thread with.
        \param pParam void* parameter to give to the function.
        \param bUsermode Start the thread in User Mode?
        \param pStack Stack to start the thread with. */
    void addThread(Thread *pThread, Thread::ThreadStartFunc pStartFunction,
                   void *pParam, bool bUsermode, void *pStack);

    /** Adds a new thread.
        \param pThread The thread to add.
        \param state The syscall state to jump to. */
    void addThread(Thread *pThread, SyscallState &state);

    /** Destroys the currently running thread.
        \note This calls Thread::~Thread itself! */
    void killCurrentThread();

    /** Puts a thread to sleep.
        \param pLock Optional, will release this lock when the thread is successfully 
                     in the sleep state.
        \note This function is here because it acts on the current thread. Its
              counterpart, wake(), is in Scheduler as it could be called from
              any thread. */
    inline void sleep(Spinlock *pLock=0)
    {
        schedule(Thread::Sleeping, pLock);
    }

    /** TimerHandler callback. */
    void timer(uint64_t delta, InterruptState &state)
    {
        schedule();
    }

    void removeThread(Thread *pThread);

private:
    /** Copy-constructor
     *  \note Not implemented - singleton class. */
    PerProcessorScheduler(const PerProcessorScheduler &);
    /** Assignment operator
     *  \note Not implemented - singleton class */
    PerProcessorScheduler &operator = (const PerProcessorScheduler &);

    /** Checks whether any signals are pending on the given Thread, and dispatches
        if needed. */
    void checkSignalState(Thread *pThread);

    /** Perform a context switch. The current processor state is stored in oldState,
        with the new state switched in from newState. When all resources from 
        oldState are used (i.e. when the stack is switched), lock is set to
        zero.

        \note Implemented in core/processor/ARCH/asm. */
    static void contextSwitch(SchedulerState &oldState, SchedulerState &newState,
                              volatile uintptr_t &lock);

    /** Start a thread. This overload stores the current processor state in
        oldState, releases lock, then creates a stack frame consisting of a call
        to startFunction with a parameter param, on the stack 'stack', in 
        user privilege mode if usermode is true.

        \note Implemented in core/processor/ARCH/asm. */
    static void launchThread(SchedulerState &oldState, volatile uintptr_t &lock, 
                             uintptr_t stack, uintptr_t startFunction, uintptr_t param,
                             uintptr_t usermode);

    /** Start a thread. This overload stores the current processor state in oldState,
        releases lock, then jumps to the state given in syscallState. */
    static void launchThread(SchedulerState &oldState, volatile uintptr_t &lock,
                             SyscallState &syscallState);

    /** Switches stacks, calls PerProcessorScheduler::deleteThread, then context
        switches.

        \note Implemented in core/processor/ARCH/asm*/
    static void deleteThreadThenContextSwitch(Thread *pThread, uintptr_t &newState);

    static void deleteThread(Thread *pThread);

    /** The current SchedulingAlgorithm */
    SchedulingAlgorithm *m_pSchedulingAlgorithm;
};

#endif
