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

#ifndef THREAD_H
#define THREAD_H

#include <Log.h>

#include <processor/state.h>
#include <processor/types.h>
#include <process/Event.h>

#include <utilities/List.h>
#include <utilities/ExtensibleBitmap.h>

class Processor;
class Process;

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
        Zombie
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
     *               only. */
    Thread(Process *pParent, ThreadStartFunc pStartFunction, void *pParam,
           void *pStack=0);

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

    /** Returns a reference to the Thread's saved context. This function is intended only
     * for use by the Scheduler. */
    SchedulerState &state()
    {return m_StateLevels[m_nStateLevel].m_State;}

    /** Increases the state nesting level by one - pushes a new state to the top of the state stack.
        This also pushes to the top of the inhibited events stack, copying the current inhibit mask.
        \todo This should also push errno and m_bInterrupted, so syscalls can be
              used safely in interrupt handlers.
        \return A reference to the previous state. */
    SchedulerState &pushState()
    {
        if (m_nStateLevel >= MAX_NESTED_EVENTS)
        {
            ERROR("Thread: Max nested events!");
            /// \todo Take some action here - possibly kill the thread?
            return m_StateLevels[m_nStateLevel].m_State;
        }
        m_nStateLevel++;
        m_StateLevels[m_nStateLevel].m_InhibitMask = m_StateLevels[m_nStateLevel - 1].m_InhibitMask;
        allocateStackAtLevel(m_nStateLevel);

        setKernelStack();

        return m_StateLevels[m_nStateLevel - 1].m_State;
    }

    /** Decreases the state nesting level by one, popping both the state stack and the inhibit mask
        stack.*/
    void popState()
    {
        if (m_nStateLevel == 0)
        {
            ERROR("Thread: popStack() called with state level 0!");
        }
        m_nStateLevel --;

        setKernelStack();
    }

    /** Returns the state nesting level. */
    size_t getStateLevel()
    {return m_nStateLevel;}

    /** Allocates a new stack for a specific nesting level, if required */
    void allocateStackAtLevel(size_t stateLevel);

    /** Sets the new kernel stack for the current state level in the TSS */
    void setKernelStack();

    /** Overwrites the state at the given nesting level.
     *\param stateLevel The nesting level to edit.
     *\param state The state to copy.
     */
    void pokeState(size_t stateLevel, SchedulerState &state)
    {
        if (stateLevel >= MAX_NESTED_EVENTS)
        {
            ERROR("Thread::pokeState(): stateLevel `" << stateLevel << "' is over the maximum.");
            return;
        }
        m_StateLevels[stateLevel].m_State = state;
    }

    /** Retrieves a pointer to this Thread's parent process. */
    Process *getParent() const
    {return m_pParent;}

    /** Retrieves our current status. */
    Status getStatus() const
    {return m_Status;}

    /** Sets our current status. */
    void setStatus(Status s);

    /** Retrieves the exit status of the Thread.
     * \note Valid only if the Thread is in the Zombie state. */
    int getExitCode()
    {return m_ExitCode;}

    /** Retrieves a pointer to the top of the Thread's kernel stack. */
    void *getKernelStack()
    {
        return m_StateLevels[m_nStateLevel].m_pKernelStack;
    }
    /*
        return m_pKernelStack;}
    */

    /** Returns the Thread's ID. */
    size_t getId()
    {return m_Id;}

    /** Returns the last error that occurred (errno). */
    size_t getErrno()
    {return m_Errno;}

    /** Sets the last error - errno. */
    void setErrno(size_t errno)
    {m_Errno = errno;}

    /** Returns whether the thread was just interrupted deliberately (e.g.
        because of a timeout). */
    bool wasInterrupted()
    {return m_bInterrupted;}

    /** Sets whether the thread was just interrupted deliberately. */
    void setInterrupted(bool b)
    {m_bInterrupted = b;}


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

    /** Grabs the first available unmasked event and pops it off the queue.
        This also pushes the inhibit mask stack.

        \note This is intended only to be called by PerProcessorScheduler. */
    Event *getNextEvent();

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
        StateLevel() :
            m_State(), m_pKernelStack(0), m_InhibitMask()
        {}
        ~StateLevel()
        {}

        StateLevel(const StateLevel &s) :
            m_State(s.m_State), m_pKernelStack(s.m_pKernelStack), m_InhibitMask(s.m_InhibitMask)
        {}

        StateLevel &operator = (const StateLevel &s)
        {
            m_State = s.m_State;
            m_pKernelStack = s.m_pKernelStack;
            m_InhibitMask = s.m_InhibitMask;
            return *this;
        }

        /** The processor state for this level. */
        SchedulerState m_State;

        /** Our kernel stack. */
        void *m_pKernelStack;

        ExtensibleBitmap m_InhibitMask;
    } m_StateLevels[MAX_NESTED_EVENTS];

        // SchedulerState m_States[MAX_NESTED_EVENTS];

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

    /** Stack of inhibited Event masks, gets pushed with a new value when an Event handler is run, and
        popped when one completes.

        \note A '1' here means the event is inhibited, '0' means it can be fired. */
    // ExtensibleBitmap m_InhibitMasks[MAX_NESTED_EVENTS];

    /** Queue of Events ready to run. */
    List<Event*> m_EventQueue;
};

#endif
