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

#include <compiler.h>
#include <machine/Machine.h>
#include <process/Event.h>
#include <process/Thread.h>
#include <processor/Processor.h>
#include "Timer.h"

#include <SlamAllocator.h>

// Millisecond interval (tick every ms)
#define INTERVAL 1000000

using namespace __pedigree_hosted;

// Set by PhysicalMemoryManager.
extern size_t g_FreePages;
extern size_t g_AllocedPages;

HostedTimer HostedTimer::m_Instance;

uint8_t daysPerMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

void HostedTimer::addAlarm(Event *pEvent, size_t alarmSecs, size_t alarmUsecs)
{
    Alarm *pAlarm = new Alarm(pEvent, alarmSecs * 1000000 + alarmUsecs + getTickCount(),
                              Processor::information().getCurrentThread());
    m_Alarms.pushBack(pAlarm);
}

void HostedTimer::removeAlarm(Event *pEvent)
{
    for (List<Alarm*>::Iterator it = m_Alarms.begin();
            it != m_Alarms.end();
            it++)
    {
        if ( (*it)->m_pEvent == pEvent )
        {
            m_Alarms.erase(it);
            return;
        }
    }
}

size_t HostedTimer::removeAlarm(class Event *pEvent, bool bRetZero)
{
    size_t currTime = getTickCount();

    for (List<Alarm*>::Iterator it = m_Alarms.begin();
            it != m_Alarms.end();
            it++)
    {
        if ( (*it)->m_pEvent == pEvent )
        {
            size_t ret = 0;
            if (!bRetZero)
            {
                size_t alarmEndTime = (*it)->m_Time;

                // Is it later than the end of the alarm?
                if (alarmEndTime < currTime)
                    ret = 0;
                else
                {
                    size_t diff = alarmEndTime - currTime;
                    ret = (diff / 1000) + 1;
                }
            }

            m_Alarms.erase(it);
            return ret;
        }
    }

    return 0;
}

bool HostedTimer::registerHandler(TimerHandler *handler)
{
    // find a spare spot and install
    size_t nHandler;
    for (nHandler = 0; nHandler < MAX_TIMER_HANDLERS; nHandler++)
    {
        if (m_Handlers[nHandler] == 0)
        {
            m_Handlers[nHandler] = handler;
            return true;
        }
    }

    // no room!
    return false;
}

bool HostedTimer::unregisterHandler(TimerHandler *handler)
{
    // find a spare spot and install
    size_t nHandler;
    for (nHandler = 0; nHandler < MAX_TIMER_HANDLERS; nHandler++)
    {
        if (m_Handlers[nHandler] == handler)
        {
            m_Handlers[nHandler] = 0;
            return true;
        }
    }

    // not found
    return false;
}

size_t HostedTimer::getYear()
{
    return m_Year;
}

uint8_t HostedTimer::getMonth()
{
    return m_Month;
}

uint8_t HostedTimer::getDayOfMonth()
{
    return m_DayOfMonth;
}

uint8_t HostedTimer::getDayOfWeek()
{
    return m_DayOfWeek;
}

uint8_t HostedTimer::getHour()
{
    return m_Hour;
}

uint8_t HostedTimer::getMinute()
{
    return m_Minute;
}

uint8_t HostedTimer::getSecond()
{
    return m_Second;
}

uint64_t HostedTimer::getNanosecond()
{
    return m_Nanosecond;
}

uint64_t HostedTimer::getTickCount()
{
    struct timespec tv;
    clock_gettime(CLOCK_MONOTONIC, &tv);
    return (tv.tv_sec * 1000ULL) + (tv.tv_nsec / 1000ULL);
}

bool HostedTimer::initialise()
{
    synchronise();

    memset(m_Handlers, 0, sizeof(TimerHandler*) * MAX_TIMER_HANDLERS);

    struct sigevent sv;
    memset(&sv, 0, sizeof(sv));
    sv.sigev_notify = SIGEV_SIGNAL;
    sv.sigev_signo = SIGUSR1;
    sv.sigev_value.sival_ptr = this;
    int r = timer_create(CLOCK_MONOTONIC, &sv, &m_Timer);
    if(r != 0)
    {
        /// \todo error message or something
        return false;
    }

    struct itimerspec interval;
    memset(&interval, 0, sizeof(interval));
    interval.it_interval.tv_nsec = INTERVAL;
    interval.it_value.tv_nsec = INTERVAL;
    r = timer_settime(m_Timer, 0, &interval, 0);
    if(r != 0)
    {
        timer_delete(m_Timer);
        return false;
    }

    IrqManager &irqManager = *Machine::instance().getIrqManager();
    m_IrqId = irqManager.registerIsaIrqHandler(0, this);
    if (m_IrqId == 0)
    {
        timer_delete(m_Timer);
        return false;
    }

    return true;
}

void HostedTimer::synchronise()
{
    struct timespec tv;
    clock_gettime(CLOCK_REALTIME, &tv);
    struct tm *t = gmtime(&tv.tv_sec);

    m_Nanosecond = tv.tv_nsec;
    m_Second = t->tm_sec;
    m_Minute = t->tm_min;
    m_Hour = t->tm_hour;
    m_DayOfMonth = t->tm_mday;
    m_Month = t->tm_mon;
    m_Year = t->tm_year + 1900;  // Years since 1900.
    m_DayOfWeek = t->tm_wday;
}

void HostedTimer::uninitialise()
{
    synchronise();

    timer_delete(m_Timer);

    // Unregister the irq
    IrqManager &irqManager = *Machine::instance().getIrqManager();
    irqManager.unregisterHandler(m_IrqId, this);
}

HostedTimer::HostedTimer() : m_Year(0), m_Month(0), m_DayOfMonth(0),
    m_DayOfWeek(0), m_Hour(0), m_Minute(0), m_Second(0), m_Nanosecond(0),
    m_IrqId(0), m_Alarms()
{
}

bool HostedTimer::irq(irq_id_t number, InterruptState &state)
{
    // Should we handle this?
    uint64_t opaque = state.getRegister(0);
    if(opaque != reinterpret_cast<uint64_t>(this))
    {
        return false;
    }

    // Update tick count.
    uint64_t delta = INTERVAL;

    // Calculate the new time/date
    m_Nanosecond += delta;

    // Check for alarms.
    while (true)
    {
        bool bDispatched = false;
        for (List<Alarm*>::Iterator it = m_Alarms.begin();
                it != m_Alarms.end();
                it++)
        {
            Alarm *pA = *it;
            if ( pA->m_Time <= getTickCount() )
            {
                pA->m_pThread->sendEvent(pA->m_pEvent);
                m_Alarms.erase(it);
                bDispatched = true;
                break;
            }
        }
        if (!bDispatched)
            break;
    }

    if (UNLIKELY(m_Nanosecond >= 1000000ULL))
    {
        // Every millisecond, unblock any interrupts which were halted and halt any
        // which need to be halted.
        Machine::instance().getIrqManager()->tick();
    }

    if (UNLIKELY(m_Nanosecond >= 1000000000ULL))
    {
        ++m_Second;
        m_Nanosecond -= 1000000000ULL;

#ifdef MEMORY_LOGGING_ENABLED
        Serial *pSerial = Machine::instance().getSerial(1);
        NormalStaticString str;
        str += "Heap: ";
        str += SlamAllocator::instance().heapPageCount() * 4;
        str += "K\tPages: ";
        str += (g_AllocedPages * 4096) / 1024;
        str += "K\t Free: ";
        str += (g_FreePages * 4096) / 1024;
        str += "K\n";

        pSerial->write(str);
#endif

        if (UNLIKELY(m_Second == 60))
        {
            ++m_Minute;
            m_Second = 0;

            if (UNLIKELY(m_Minute == 60))
            {
                ++m_Hour;
                m_Minute = 0;

                if (UNLIKELY(m_Hour == 24))
                {
                    ++m_DayOfMonth;
                    m_Hour = 0;

                    // Are we in a leap year
                    bool isLeap = ((m_Year % 4) == 0) & (((m_Year % 100) != 0) | ((m_Year % 400) == 0));

                    if (UNLIKELY(((m_DayOfMonth > daysPerMonth[m_Month - 1]) && ((m_Month != 2) || isLeap == false)) ||
                                 (m_DayOfMonth > (daysPerMonth[m_Month - 1] + 1))))
                    {
                        ++m_Month;
                        m_DayOfMonth = 1;

                        if (UNLIKELY(m_Month > 12))
                        {
                            ++m_Year;
                            m_Month = 1;
                        }
                    }
                }
            }
        }
    }

    // call handlers
    size_t nHandler;
    for (nHandler = 0; nHandler < MAX_TIMER_HANDLERS; nHandler++)
    {
        // timer delta is in nanoseconds
        if (m_Handlers[nHandler])
            m_Handlers[nHandler]->timer(delta, state);
    }

    return true;
}
