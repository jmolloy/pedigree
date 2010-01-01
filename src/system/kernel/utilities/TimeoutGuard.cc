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

#include <utilities/TimeoutGuard.h>
#ifdef THREADS
#include <process/Thread.h>
#include <process/PerProcessorScheduler.h>
#endif
#include <processor/Processor.h>
#include <machine/Timer.h>
#include <machine/Machine.h>
#include <LockGuard.h>
#include <Log.h>

TimeoutGuard::TimeoutGuard(size_t timeoutSecs) :
    m_pEvent(0), m_bTimedOut(false),
#ifdef THREADS
    m_State(),
#endif
    m_nLevel(0), m_Lock()
{
#ifdef THREADS
    if(timeoutSecs)
    {
        Thread *pThread = Processor::information().getCurrentThread();
        
        m_nLevel = pThread->getStateLevel();
        m_pEvent = new TimeoutGuardEvent(this, m_nLevel);
        
        Machine::instance().getTimer()->addAlarm(m_pEvent, timeoutSecs);
        
        // Generate the SchedulerState to restore to.
        Processor::saveState(m_State);
    }
#else
    WARNING("TimeoutGuard: TimeoutGuard needs thread support");
#endif
}

TimeoutGuard::~TimeoutGuard()
{
#ifdef THREADS
    // Stop any interrupts - now we know that we can't be preempted by
    // our own event handler.
    LockGuard<Spinlock> guard(m_Lock);

    // If the event hasn't fired yet, remove and delete it.
    if (m_pEvent)
    {
        Machine::instance().getTimer()->removeAlarm(m_pEvent);
        // Ensure that the event isn't queued.
        Processor::information().getCurrentThread()->cullEvent(m_pEvent);
        delete m_pEvent;
        m_pEvent = 0;
    }
#endif
}

void TimeoutGuard::cancel()
{
#ifdef THREADS
    // Called by TimeoutGuardEvent.
    m_bTimedOut = true;

    m_pEvent = 0;

    Processor::restoreState(m_State);
#endif
}

static void guardEventFired(uint8_t *pBuffer)
{
#ifdef THREADS
    NOTICE("GuardEventFired");
    TimeoutGuard::TimeoutGuardEvent e;
    if (!TimeoutGuard::TimeoutGuardEvent::unserialize(pBuffer, e))
    {
        FATAL("guardEventFired: Event is not a TimeoutGuardEvent!");
        return;
    }
    e.m_pTarget->cancel();
    NOTICE("Cancel finished");
#endif
}

TimeoutGuard::TimeoutGuardEvent::TimeoutGuardEvent(TimeoutGuard *pTarget, size_t specificNestingLevel) :
    Event(reinterpret_cast<uintptr_t>(&guardEventFired), true /* Deletable */, specificNestingLevel),
    m_pTarget(pTarget)
{
}

TimeoutGuard::TimeoutGuardEvent::~TimeoutGuardEvent()
{
}

size_t TimeoutGuard::TimeoutGuardEvent::serialize(uint8_t *pBuffer)
{
    size_t *pBufferSize_t = reinterpret_cast<size_t*> (pBuffer);
    pBufferSize_t[0] = EventNumbers::TimeoutGuard;
    pBufferSize_t[1] = reinterpret_cast<size_t> (m_pTarget);
    return 2*sizeof(size_t);
}

bool TimeoutGuard::TimeoutGuardEvent::unserialize(uint8_t *pBuffer, TimeoutGuardEvent &event)
{
    size_t *pBufferSize_t = reinterpret_cast<size_t*> (pBuffer);
    if (pBufferSize_t[0] != EventNumbers::TimeoutGuard)
        return false;
    event.m_pTarget = reinterpret_cast<TimeoutGuard*> (pBufferSize_t[1]);
    return true;
}
