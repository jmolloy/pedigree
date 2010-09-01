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

#ifdef THREADS

#include <processor/types.h>
#include <processor/state.h>
#include <machine/TimerHandler.h>
#include <process/Mutex.h>
#include <process/Process.h>
#include <process/Thread.h>
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
        \param pNewThread Overrides the next thread to switch to.
        \param pLock      Optional lock to release when the thread is safely
                          locked. */
    void schedule(Thread::Status nextStatus=Thread::Ready, Thread *pNewThread = 0, Spinlock *pLock=0);

    /** Looks for event handlers to run, and if found, dispatches one.
        \param userStack The stack to use if the event has a user-mode handler. Usually obtained
                         from an interruptState or syscallState. */
    void checkEventState(uintptr_t userStack);

    /** Assumes this thread has just returned from executing a event handler,
        and lets it resume normal execution. */
    void eventHandlerReturned();

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
        schedule(Thread::Sleeping, 0, pLock);
    }

    /** TimerHandler callback. */
    void timer(uint64_t delta, InterruptState &state);

    void removeThread(Thread *pThread);

    void threadStatusChanged(Thread *pThread);

private:
    /** Copy-constructor
     *  \note Not implemented - singleton class. */
    PerProcessorScheduler(const PerProcessorScheduler &);
    /** Assignment operator
     *  \note Not implemented - singleton class */
    PerProcessorScheduler &operator = (const PerProcessorScheduler &);

    /** Switches stacks, calls PerProcessorScheduler::deleteThread, then context
        switches.

        \note Implemented in core/processor/ARCH/asm*/
    static void deleteThreadThenRestoreState(Thread *pThread, SchedulerState &newState);

    static void deleteThread(Thread *pThread);

    /** The current SchedulingAlgorithm */
    SchedulingAlgorithm *m_pSchedulingAlgorithm;
    
    Mutex m_NewThreadDataLock;
    Semaphore m_NewThreadDataCount;
    
    List<void*> m_NewThreadData;
    
    static int processorAddThread(void *instance);

#ifdef ARM_BEAGLE
    size_t m_TickCount;
#endif
};

#endif

#endif
