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

#include <time/Time.h>
#include <machine/Machine.h>
#include <machine/Timer.h>
#include <process/Event.h>
#include <process/eventNumbers.h>
#include <processor/Processor.h>

namespace Time
{

static void delayTimerFired(uint8_t *pBuffer)
{
    Processor::information().getCurrentThread()->setInterrupted(true);
}

class DelayTimerEvent : public Event
{
public:
    DelayTimerEvent() :
        Event(reinterpret_cast<uintptr_t>(&delayTimerFired), false)
    {
    }
    virtual ~DelayTimerEvent()
    {
    }

    virtual size_t serialize(uint8_t *pBuffer)
    {
        return 0;
    }
    static bool unserialize(uint8_t *pBuffer, DelayTimerEvent &event)
    {
        return true;
    }

    virtual size_t getNumber()
    {
        return EventNumbers::DelayTimer;
    }
};

bool delay(Timestamp nanoseconds)
{
    Thread *pThread = Processor::information().getCurrentThread();
    Event *pEvent = new DelayTimerEvent();
    uint64_t usecs = nanoseconds / Multiplier::MICROSECOND;
    if (!usecs)
        ++usecs;  /// \todo perhaps change addAlarm to take ns.
    Machine::instance().getTimer()->addAlarm(pEvent, 0, usecs);

    /// \todo possible race condition for very short alarm times
    while (true)
    {
        pThread->setInterrupted(false);
        Processor::information().getScheduler().sleep(0);
        if (pThread->wasInterrupted())
        {
            Machine::instance().getTimer()->removeAlarm(pEvent);
            delete pEvent;
            break;
        }
        else if (pThread->getUnwindState() != Thread::Continue)
        {
            Machine::instance().getTimer()->removeAlarm(pEvent);
            delete pEvent;
            return false;
        }
    }

    return true;
}

}
