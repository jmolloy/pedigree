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

#ifndef SCHEDULER_H
#define SCHEDULER_H

#ifdef THREADS

#include <process/PerProcessorScheduler.h>
#include <processor/types.h>
#include <processor/state.h>
#include <machine/TimerHandler.h>
#include <process/Mutex.h>
#include <process/Process.h>
#include <Atomic.h>

class Thread;
class Processor;

/** \brief This class manages how processes and threads are scheduled across processors.
 * 
 * This is the "long term" scheduler - it load balances between processors and provides
 * the interface for adding, listing and removing threads.
 *
 * The load balancing is "lazy" in that the algorithm only runs on thread addition
 * and removal.
 */
class Scheduler
{
public:
    /** Get the instance of the scheduler */
    inline static Scheduler &instance() {return m_Instance;}

    /** Initialises the scheduler. */
    bool initialise();

    /** Adds a thread to be load-balanced and accounted.
        \param pThread The new thread.
        \param PPSched The per-processor scheduler the thread will start on. */
    void addThread(Thread *pThread, PerProcessorScheduler &PPSched);
    /** Removes a thread from being load-balanced and accounted. */
    void removeThread(Thread *pThread);

    /** Whether a thread is entered into the scheduler at all. */
    bool threadInSchedule(Thread *pThread);
  
    /** Adds a process.
     *  \note This is purely for enumeration purposes.
     *  \return The ID that should be applied to this Process. */
    size_t addProcess(Process *pProcess);
    /** Removes a process.
     *  \note This is purely for enumeration purposes. */
    void removeProcess(Process *pProcess);
  
    /** Causes a manual reschedule. */
    void yield();

    /** Returns the number of processes currently in operation. */
    size_t getNumProcesses();
  
    /** Returns the n'th process currently in operation. */
    Process *getProcess(size_t n);

    void threadStatusChanged(Thread *pThread);

private:
    /** Default constructor
     *  \note Private - singleton class. */
    Scheduler();
    /** Copy-constructor
     *  \note Not implemented - singleton class. */
    Scheduler(const Scheduler &);
    /** Destructor
     *  \note Private - singleton class. */
    ~Scheduler();
    /** Assignment operator
     *  \note Not implemented - singleton class */
    Scheduler &operator = (const Scheduler &);
  
    /** The Scheduler instance. */
    static Scheduler m_Instance;
  
    /** All the processes currently in operation, for enumeration purposes. */
    List<Process*> m_Processes;

    /** The next available process ID. */
    Atomic<size_t> m_NextPid;

    /** Map of processor->thread mappings, for load-balance accounting. */
    Tree<PerProcessorScheduler*, List<Thread*>*> m_PTMap;

    /** Map of thread->processor mappings. */
    Tree<Thread*, PerProcessorScheduler*> m_TPMap;
};

#endif

#endif
