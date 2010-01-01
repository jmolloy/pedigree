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
#ifndef TIMEOUT_GUARD_H
#define TIMEOUT_GUARD_H

/**\file  TimeoutGuard.h
 *\author James Molloy <jamesm@osdev.org>
 *\date   Mon May 11 11:27:37 2009
 *\brief  Implements the class TimeoutGuard, which cancels any operation after
          a given amount of time. */

#include <processor/types.h>
#include <process/Event.h>
#include <process/eventNumbers.h>
#include <process/PerProcessorScheduler.h>
#include <Spinlock.h>

/** This class waits (in the background) for a given amount of time to elapse,
    then cancels whatever operation is currently taking place.

    The class functions in a similar way to setjmp/longjmp, except using
    saveState/editState. The timeout is cancelled when the class goes out of
    scope.

    \note Because it uses 'longjmp', this class can cause memory leaks and 
          skip destructors for stack-local objects. Use it wisely. */
class TimeoutGuard
{
public:
    /** Creates a new TimeoutGuard, with the given timeout, in seconds. */
    TimeoutGuard(size_t timeoutSecs);

    /** Destroys the TimeoutGuard, cancelling the timeout. */
    ~TimeoutGuard();

    /** Returns true if the guard just timed out, false if not. This should be
        called just after the constructor. */
    bool timedOut()
    {return m_bTimedOut;}

    /** Cancels the current operation.
        \note This is intended only to be called from a TimeoutGuardEvent. */
    void cancel();

    /** Internal event class. */
    class TimeoutGuardEvent : public Event
    {
    public:
        TimeoutGuardEvent() :
            Event(0, false), m_pTarget(0)
        {}
        TimeoutGuardEvent(TimeoutGuard *pTarget, size_t specificNestingLevel);
        virtual ~TimeoutGuardEvent();

        virtual size_t serialize(uint8_t *pBuffer);
        static bool unserialize(uint8_t *pBuffer, TimeoutGuardEvent &event);

        virtual size_t getNumber()
        {return EventNumbers::TimeoutGuard;}

        /** The TimeoutGuard to cancel. */
        TimeoutGuard *m_pTarget;

    private:
        TimeoutGuardEvent(const TimeoutGuardEvent &);
        TimeoutGuardEvent &operator = (const TimeoutGuardEvent &);
    };

private:
    TimeoutGuard(const TimeoutGuard &);
    TimeoutGuard &operator = (const TimeoutGuard &);

    /** The event, which will be automatically freed when it fires. */
    TimeoutGuardEvent *m_pEvent;

    /** Did the operation time out? */
    bool m_bTimedOut;

    /** Saved state. */
#ifdef THREADS
    SchedulerState m_State;
#endif

    /** Saved nesting level. */
    size_t m_nLevel;

    /** Our own personal lock. */
    Spinlock m_Lock;
};

#endif
