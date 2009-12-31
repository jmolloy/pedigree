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
#ifndef TIMED_TASK_H
#define TIMED_TASK_H

/**\file   TimedTask.h
 *\author Matthew Iselin
 *\date   Sun May 10 16:16:56 2009
 *\brief  A task to be performed with a soft deadline timeout. */

#if 0 // Obsoleted
 
#include <process/Thread.h>
#include <processor/state.h>
#include <processor/types.h>
#include <process/Semaphore.h>
#include <process/Scheduler.h>
#include <processor/Processor.h>
#include <machine/Timer.h>
#include <machine/Machine.h>

/** TimedTask defines a specific task that needs to be performed
  * within a specific timeframe.
  * Inherit from this and define your own task() function in order
  * to perform different tasks.
  */
class TimedTask
{
    public:

        TimedTask() :
            m_Timeout(30), m_Success(false), m_Active(false), m_ReturnValue(0), m_Task(0),
            m_pTimeoutEvent(0), m_WaitSem(0), m_Lock(), m_nLevel(0)
        {};

        virtual ~TimedTask()
        {
            if(m_WaitSem)
            {
                m_WaitSem->release();
                m_WaitSem = 0;
            }

            // Delete the event, if needed
            cleanup(true);
        }

        /** Internal event class */
        class TimedTaskEvent : public Event
        {
        public:
            TimedTaskEvent() :
                Event(0, false), m_pTarget(0)
            {}
            TimedTaskEvent(TimedTask *pTarget, size_t specificNestingLevel);
            virtual ~TimedTaskEvent();

            virtual size_t serialize(uint8_t *pBuffer);
            static bool unserialize(uint8_t *pBuffer, TimedTaskEvent &event);

            virtual size_t getNumber()
            {return EventNumbers::TimedTask;}

            /** The TimedTask to cancel. */
            TimedTask *m_pTarget;

        private:
            TimedTaskEvent(const TimedTaskEvent &);
            TimedTaskEvent &operator = (const TimedTaskEvent &);
        };

        virtual int task() = 0;

        void begin()
        {
            if(m_WaitSem && !m_pTimeoutEvent)
            {
                Thread *pThread = Processor::information().getCurrentThread();

                m_nLevel = pThread->getStateLevel();
                m_pTimeoutEvent = new TimedTaskEvent(this, m_nLevel);

                // The implementation of task should verify the timeout, and if it is zero, should not loop. Even so,
                // we want to be able to execute the task.
                Machine::instance().getTimer()->addAlarm(m_pTimeoutEvent, m_Timeout ? m_Timeout : 10);

                m_Active = true;
                Process *pProcess = Processor::information().getCurrentThread()->getParent();
                m_Task = new Thread(
                                pProcess,
                                reinterpret_cast<Thread::ThreadStartFunc>(taskTrampoline),
                                reinterpret_cast<void *>(this)
                                );
            }
            else
                WARNING("TimedTask::begin() called with no semaphore");
        }

        void cancel()
        {
            m_Success = false;
            if(m_WaitSem)
            {
                m_WaitSem->release();
                m_WaitSem = 0;
            }

            cleanup(true);
        }

        void timeout()
        {
            // Event fired, already freed.
            m_pTimeoutEvent = 0;
            cleanup(true);
        }

        /** Called when the task completes successfully */
        void end(int returnValue)
        {
            m_Active = false;

            m_Success = true;
            m_ReturnValue = returnValue;

            if(m_WaitSem)
            {
                m_WaitSem->release();
                m_WaitSem = 0;
            }

            m_Task = 0;

            cleanup(false);
        }

        static int taskTrampoline(void *ptr)
        {
            TimedTask *t = reinterpret_cast<TimedTask *>(ptr);
            if(!t)
                return 0;

            // Run the task
            int ret = t->task();

            // We don't actually know if we'll ever reach this point, but if we do,
            // we're done :)
            t->end(ret);

            // Exit the thread
            return 0;
        }

        void setSemaphore(Semaphore *mySem)
        {
            m_WaitSem = mySem;
        }

        bool wasSuccessful()
        {
            return m_Success;
        }

        void setTimeout(uint32_t timeout)
        {
            m_Timeout = timeout;
        }

        /** Will a timeout cause the task to be cancelled or not? */
        bool isTimeoutActive()
        {
            return (m_pTimeoutEvent != 0);
        }

        /** Don't use this if wasSuccessful() == false */
        int getReturnValue()
        {
            return m_ReturnValue;
        }

    private:
        TimedTask(const TimedTask&);
        const TimedTask& operator = (const TimedTask&);

    protected:
        uint32_t m_Timeout; // defaults to 30 seconds

    private:

        void cleanup(bool bFreeTask)
        {
            // Stop any interrupts - now we know that we can't be preempted by
            // our own event handler.
            LockGuard<Spinlock> guard(m_Lock);

            if(m_pTimeoutEvent)
            {
                // The event hasn't fired yet, remove and delete it.
                Machine::instance().getTimer()->removeAlarm(m_pTimeoutEvent);

                // Ensure that the event isn't queued.
                Processor::information().getCurrentThread()->cullEvent(m_pTimeoutEvent);
                delete m_pTimeoutEvent;
                m_pTimeoutEvent = 0;
            }

            if(m_Task && bFreeTask)
            {
                // Kill the thread
#ifndef MULTIPROCESSOR
                Scheduler::instance().removeThread(m_Task);
                delete m_Task;
                m_Task = 0;
#else
                /// \todo Write a method for deleting the old task
                FATAL("Cannot cancel TimedTask on a multiprocessor system.");
#endif
            }
        }

        bool m_Success;
        bool m_Active;

        int m_ReturnValue;

        Thread *m_Task;

        /// This is used to perform the timeout & task cancellation
        TimedTaskEvent *m_pTimeoutEvent;

        /** Passed to us by the caller, this is how the caller knows that this code
        * has completed executing (whether it's cancelled or completed successfully
        * is another thing :) )
        */
        Semaphore *m_WaitSem;

        /** Our own personal lock. */
        Spinlock m_Lock;

        /** Nesting level */
        size_t m_nLevel;
};

#endif

#endif
