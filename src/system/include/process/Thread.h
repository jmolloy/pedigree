/*
 * Copyright (c) 2008-2014, Pedigree Developers
 *
 * Please see the CONTRIB file in the root of the source tree for a full
 * list of contributors.
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

#ifndef THREAD_H
#define THREAD_H

#ifdef THREADS

#include <Log.h>

#include <processor/state.h>
#include <processor/types.h>
#include <process/Event.h>

#include <utilities/List.h>
#include <utilities/RequestQueue.h>
#include <utilities/ExtensibleBitmap.h>

#include <processor/MemoryRegion.h>

// Hacky but I'd rather not c&p the typedef
#define _PROCESSOR_INFORMATION_ONLY_WANT_PROCESSORID
#include <processor/ProcessorInformation.h>
#undef _PROCESSOR_INFORMATION_ONLY_WANT_PROCESSORID

class Processor;
class Process;

/** Thread TLS area size */
#define THREAD_TLS_SIZE     0x100000

/**
 * An abstraction of a thread of execution.
 *
 * The thread maintains not just one execution context (SchedulerState) but a
 * stack of them, along with a stack of masks for inhibiting event dispatch.
 *
 * This enables event dispatch at any time without affecting the previous state,
 * as well as changing the event mask from nested event handlers without affecting
 * the state of any other running handler.
 */
class Thread
{
public:
    /** The state that a thread can possibly have. */
    enum Status
    {
        Ready,
        Running,
        Sleeping,
        Zombie,
        AwaitingJoin,
        Suspended, /// Suspended (eg, POSIX SIGSTOP)
    };

    /** "Debug state" - higher level state of the thread. */
    enum DebugState
    {
        None,
        SemWait,
        Joining
    };

    /** Thread start function type. */
    typedef int (*ThreadStartFunc)(void*);

    /** Creates a new Thread belonging to the given Process. It shares the Process'
     * virtual address space.
     *
     * The constructor registers itself with the Scheduler and parent process - this
     * does not need to be done manually.
     *
     * If kernelMode is true, and pStack is NULL, no stack space is assigned.
     *
     * \param pParent The parent process. Can never be NULL.
     * \param kernelMode Is the thread going to be operating in kernel space only?
     * \param pStartFunction The function to be run when the thread starts.
     * \param pParam A parameter to give the startFunction.
     * \param pStack (Optional) A (user mode) stack to give the thread - applicable for user mode threads
     *               only.
     * \param semiUser (Optional) Whether to start the thread as if it was a user mode thread, but begin
                     in kernel mode (to do setup and jump to usermode manually). */
    Thread(Process *pParent, ThreadStartFunc pStartFunction, void *pParam,
           void *pStack=0, bool semiUser = false, bool bDontPickCore = false);

    /** Alternative constructor - this should be used only by initialiseMultitasking() to
     * define the first kernel thread. */
    Thread(Process *pParent);

    /** Constructor for when forking a process. Assumes pParent has already been set up with a clone
     * of the current address space and sets up the new thread to return to the caller in that address space. */
    Thread(Process *pParent, SyscallState &state);

    /** Destroys the Thread.
     *
     * The destructor unregisters itself with the Scheduler and parent process - this
     * does not need to be done manually. */
    virtual ~Thread();

    /**
     * Performs termination steps on the thread, while the thread is still able
     * to reschedule. Required as ~Thread() is called in a context where the
     * thread has been removed from the scheduler, and triggering a reschedule
     * may add the thread back to the ready queue by accident.
     */
    void shutdown();

    /** Returns a reference to the Thread's saved context. This function is intended only
     * for use by the Scheduler. */
    SchedulerState &state();

    /** Increases the state nesting level by one - pushes a new state to the top of the state stack.
        This also pushes to the top of the inhibited events stack, copying the current inhibit mask.
        \todo This should also push errno and m_bInterrupted, so syscalls can be
              used safely in interrupt handlers.
        \return A reference to the previous state. */
    SchedulerState &pushState();

    /** Decreases the state nesting level by one, popping both the state stack and the inhibit mask
        stack.*/
    void popState();

    void *getStateUserStack();

    void setStateUserStack(void *st);

    /** Returns the state nesting level. */
    size_t getStateLevel() const;

    /** Allocates a new stack for a specific nesting level, if required */
    void allocateStackAtLevel(size_t stateLevel);

    /** Sets the new kernel stack for the current state level in the TSS */
    void setKernelStack();

    /** Overwrites the state at the given nesting level.
     *\param stateLevel The nesting level to edit.
     *\param state The state to copy.
     */
    void pokeState(size_t stateLevel, SchedulerState &state);

    /** Retrieves a pointer to this Thread's parent process. */
    Process *getParent() const
    {
        return m_pParent;
    }

    void setParent(Process *p)
    {
        m_pParent = p;
    }

    /** Retrieves our current status. */
    Status getStatus() const
    {
        return m_Status;
    }

    /** Sets our current status. */
    void setStatus(Status s);

    /** Retrieves the exit status of the Thread. */
    int getExitCode()
    {
        return m_ExitCode;
    }

    /** Retrieves a pointer to the top of the Thread's kernel stack. */
    void *getKernelStack();

    /** Returns the Thread's ID. */
    size_t getId()
    {
        return m_Id;
    }

    /** Returns the last error that occurred (errno). */
    size_t getErrno()
    {
        return m_Errno;
    }

    /** Sets the last error - errno. */
    void setErrno(size_t errno)
    {
        m_Errno = errno;
    }

    /** Returns whether the thread was just interrupted deliberately (e.g.
        because of a timeout). */
    bool wasInterrupted()
    {
        return m_bInterrupted;
    }

    /** Sets whether the thread was just interrupted deliberately. */
    void setInterrupted(bool b)
    {
        m_bInterrupted = b;
    }

    /** Enum used by the following function. */
    enum UnwindType
    {
        Continue = 0,              ///< No unwind necessary, carry on as normal.
        ReleaseBlockingThread, ///< (a) below.
        Exit                   ///< (b) below.
    };  

    /** Returns nonzero if the thread has been asked to unwind quickly.
        
        This happens if this thread (or a thread blocking on this thread) is scheduled for deletion.
        The intended behaviour is that the stack is unwound as quickly as possible with all semaphores
        and buffers deleted to a point where

          (a) no threads can possibly be blocking on this or
          (b) The thread has no more locks taken and is ready to be destroyed, at which point it should
              call the subsys exit() function.
        
        Whether to adopt option A or B depends on whether this thread or not has been asked to terminate,
        given by the return value. **/
    UnwindType getUnwindState()
    {
        return m_UnwindState;
    }
    /** Sets the above unwind state. */
    void setUnwindState(UnwindType ut)
    {
        m_UnwindState = ut;
    }

    void setBlockingThread(Thread *pT)
    {
        m_StateLevels[getStateLevel()].m_pBlockingThread = pT;
    }
    Thread *getBlockingThread(size_t level=~0UL)
    {
        if (level == ~0UL) level = getStateLevel();
        return m_StateLevels[level].m_pBlockingThread;
    }

    /** Returns the thread's debug state. */
    DebugState getDebugState(uintptr_t &address)
    {
        address = m_DebugStateAddress;
        return m_DebugState;
    }
    /** Sets the thread's debug state. */
    void setDebugState(DebugState state, uintptr_t address)
    {
        m_DebugState = state;
        m_DebugStateAddress = address;
    }

    /** Returns the thread's scheduler lock. */
    Spinlock &getLock()
    {return m_Lock;}

    /** Sends the asynchronous event pEvent to this thread.

        If the thread ID is greater than or equal to EVENT_TID_MAX, the event will be ignored. */
    void sendEvent(Event *pEvent);

    /** Sets the given event number as inhibited.
        \param bInhibit True if the event is to be inhibited, false if the event is to be allowed. */
    void inhibitEvent(size_t eventNumber, bool bInhibit);

    /** Walks the event queue, removing the event \p pEvent , if found. */
    void cullEvent(Event *pEvent);

    /** Walks the event queue, removing the event with number \p eventNumber , if found. */
    void cullEvent(size_t eventNumber);

    /** Grabs the first available unmasked event and pops it off the queue.
        This also pushes the inhibit mask stack.

        \note This is intended only to be called by PerProcessorScheduler. */
    Event *getNextEvent();

    bool hasEvents();

    void setPriority(size_t p)
    {m_Priority = p;}
    size_t getPriority()
    {return m_Priority;}

    /** Adds a request to the Thread's pending request list */
    void addRequest(RequestQueue::Request *req);

    /** Removes a request from the Thread's pending request list */
    void removeRequest(RequestQueue::Request *req);

    /** An unexpected exit has occurred, perform cleanup */
    void unexpectedExit();
    
    /** Gets the TLS base address for this thread. */
    uintptr_t getTlsBase();
    
    /** Gets this thread's CPU ID */
    inline
#ifdef MULTIPROCESSOR
    ProcessorId
#else
    size_t
#endif
    getCpuId()
    {
        return m_ProcId;
    }
    
    /** Sets this thread's CPU ID */
    inline void setCpuId(
#ifdef MULTIPROCESSOR
    ProcessorId
#else
    size_t
#endif
    id)
    {
        m_ProcId = id;
    }

    /**
     * Blocks until the Thread returns.
     *
     * After join() returns successfully, the thread object is NOT valid.
     *
     * \return whether the thread was joined or not.
     */
    bool join();

    /**
     * Marks the thread as detached.
     *
     * A detached thread cannot be joined and will be automatically cleaned up
     * when the thread entry point returns, or the thread is otherwise
     * terminated. A thread cannot be detached if another thread is already
     * join()ing it.
     */
    bool detach();

    /**
     * Checks detached state of the thread.
     */
    bool detached() const {
        return m_bDetached;
    }

    /**
     * Sets the exit code of the Thread and sets the state to Zombie, if it is being waited on;
     * if it is not being waited on the Thread is destroyed.
     * \note This is meant to be called only by the thread trampoline - this is the only reason it
     *       is public. It should NOT be called by anyone else!
     */
    static void threadExited();
private:
    /** Copy-constructor */
    Thread(const Thread &);
    /** Assignment operator */
    Thread &operator = (const Thread &);

    /** A level of thread state */
    struct StateLevel
    {
        StateLevel();
        ~StateLevel();

        StateLevel(const StateLevel &s);
        StateLevel &operator = (const StateLevel &s);

        /** The processor state for this level. */
        SchedulerState *m_State;

        /** Our kernel stack. */
        void *m_pKernelStack;

        void *m_pUserStack;

        /** Auxillary stack, to be freed in case the kernel stack is null.
         *  This allows kernel mode threads to have stacks freed, as they
         *  are left hanging otherwise.
         */
        void *m_pAuxillaryStack;

        /** Stack of inhibited Event masks, gets pushed with a new value when an Event handler is run, and
            popped when one completes.

            \note A '1' here means the event is inhibited, '0' means it can be fired. */
        ExtensibleBitmap *m_InhibitMask;

        Thread *m_pBlockingThread;
    } m_StateLevels[MAX_NESTED_EVENTS];

    /** The current index into m_States (head of the state stack). */
    size_t m_nStateLevel;

    /** Our parent process. */
    Process *m_pParent;

    /** Our current status. */
    volatile Status m_Status;

    /** Our exit code. */
    int m_ExitCode;

    /** The stack that we allocated from the VMM. This may or may not also be
        the kernel stack - depends on whether we are a user or kernel mode
        thread. This is used solely for housekeeping/cleaning up purposes. */
    void *m_pAllocatedStack;

    /** Our thread ID. */
    size_t m_Id;

    /** The number of the last error to occur. */
    size_t m_Errno;

    /** Whether the thread was interrupted deliberately.
        \see Thread::wasInterrupted */
    bool m_bInterrupted;

    /** Lock for schedulers. */
    Spinlock m_Lock;

    /** General concurrency lock, not touched by schedulers. */
    Spinlock m_ConcurrencyLock;

    /** Queue of Events ready to run. */
    List<Event*> m_EventQueue;

    /** Debug state - a higher level state information for display in the debugger for debugging races and deadlocks. */
    DebugState m_DebugState;
    /** Address to supplement the DebugState information */
    uintptr_t m_DebugStateAddress;

    UnwindType m_UnwindState;

    class PerProcessorScheduler *m_pScheduler;

    /** Thread priority: 0..MAX_PRIORITIES-1, 0 being highest. */
    size_t m_Priority;

    /** List of requests pending on this Thread */
    List<RequestQueue::Request*> m_PendingRequests;
    
    /** Memory region for the TLS base of this thread (userspace-only) */
    MemoryRegion *m_pTlsBase;
    
#ifdef MULTIPROCESSOR
    ProcessorId
#else
    size_t
#endif
    m_ProcId;

    /** Are we in the process of removing tracked RequestQueue::Request objects? */
    bool m_bRemovingRequests;

    /** Waiters on this thread. */
    Thread *m_pWaiter;

    /** Whether this thread has been detached or not. */
    bool m_bDetached;
};

#endif

#endif
