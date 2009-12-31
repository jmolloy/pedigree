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

#if 0 // Obsoleted

#include <utilities/TimedTask.h>
#ifdef THREADS
#include <process/Thread.h>
#endif
#include <process/PerProcessorScheduler.h>
#include <processor/Processor.h>
#include <machine/Timer.h>
#include <machine/Machine.h>
#include <LockGuard.h>
#include <Log.h>

static void timerEventFired(uint8_t *pBuffer)
{
    NOTICE("TimerEventFired");
    TimedTask::TimedTaskEvent e;
    if (!TimedTask::TimedTaskEvent::unserialize(pBuffer, e))
    {
        FATAL("timerEventFired: Event is not a TimedTaskEvent!");
        return;
    }
    e.m_pTarget->timeout();
    NOTICE("TimedTask timeout finished");
}

TimedTask::TimedTaskEvent::TimedTaskEvent(TimedTask *pTarget, size_t specificNestingLevel) :
    Event(reinterpret_cast<uintptr_t>(&timerEventFired), true /* Deletable */, specificNestingLevel),
    m_pTarget(pTarget)
{
}

TimedTask::TimedTaskEvent::~TimedTaskEvent()
{
}

size_t TimedTask::TimedTaskEvent::serialize(uint8_t *pBuffer)
{
    size_t *pBufferSize_t = reinterpret_cast<size_t*> (pBuffer);
    pBufferSize_t[0] = EventNumbers::TimedTask;
    pBufferSize_t[1] = reinterpret_cast<size_t> (m_pTarget);
    return 2*sizeof(size_t);
}

bool TimedTask::TimedTaskEvent::unserialize(uint8_t *pBuffer, TimedTaskEvent &event)
{
    size_t *pBufferSize_t = reinterpret_cast<size_t*> (pBuffer);
    if (pBufferSize_t[0] != EventNumbers::TimedTask)
        return false;
    event.m_pTarget = reinterpret_cast<TimedTask*> (pBufferSize_t[1]);
    return true;
}

#endif
