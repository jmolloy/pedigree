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

#ifndef KERNEL_MACHINE_HOSTED_COMMON_TIMER_H
#define KERNEL_MACHINE_HOSTED_COMMON_TIMER_H

#include <machine/IrqManager.h>
#include <machine/Timer.h>
#include <machine/SchedulerTimer.h>

namespace __pedigree_hosted {
    #include <time.h>
    #include <signal.h>
}

#define MAX_TIMER_HANDLERS    32

/** @addtogroup kernelmachinehosted
 * @{ */

class HostedTimer : public Timer, private IrqHandler
{
  public:
    inline static HostedTimer &instance(){return m_Instance;}

    //
    // Timer interface
    //
    virtual bool registerHandler(TimerHandler *handler);
    virtual bool unregisterHandler(TimerHandler *handler);
    virtual void addAlarm(class Event *pEvent, size_t alarmSecs, size_t alarmUsecs=0);
    virtual void removeAlarm(class Event *pEvent);
    virtual size_t removeAlarm(class Event *pEvent, bool bRetZero);
    virtual size_t getYear();
    virtual uint8_t getMonth();
    virtual uint8_t getDayOfMonth();
    virtual uint8_t getDayOfWeek();
    virtual uint8_t getHour();
    virtual uint8_t getMinute();
    virtual uint8_t getSecond();
    virtual uint64_t getNanosecond();
    virtual uint64_t getTickCount();

    /** Initialises the class
     *\return true, if successfull, false otherwise */
    bool initialise() INITIALISATION_ONLY;
    /** Synchronise the time/date with the hardware */
    virtual void synchronise(bool tohw=false);
    /** Uninitialises the class */
    void uninitialise();

  protected:
    /** The default constructor */
    HostedTimer() INITIALISATION_ONLY;
    /** The destructor */
    inline virtual ~HostedTimer(){}

  private:
    /** The copy-constructor
     *\note NOT implemented */
    HostedTimer(const HostedTimer &);
    /** The assignment operator
     *\note NOT implemented */
    HostedTimer &operator = (const HostedTimer &);

    //
    // IrqHandler interface
    //
    virtual bool irq(irq_id_t number, InterruptState &state);

    /** The current year */
    size_t m_Year;
    /** The current month */
    uint8_t m_Month;
    /** The current day of month */
    uint8_t m_DayOfMonth;
    /** The current day of week */
    uint8_t m_DayOfWeek;
    /** The current hour */
    uint8_t m_Hour;
    /** The current minute */
    uint8_t m_Minute;
    /** The current second */
    uint8_t m_Second;
    /** The current nanosecond */
    uint64_t m_Nanosecond;

    /** Tick source. */
    __pedigree_hosted::timer_t m_Timer;

    /** Registered handler. */
    irq_id_t m_IrqId;

    /** The HostedTimer class instance */
    static HostedTimer m_Instance;

    /** All timer handlers installed */
    TimerHandler* m_Handlers[MAX_TIMER_HANDLERS];

    /** Alarm structure. */
    class Alarm
    {
    public:
        Alarm(class Event *pEvent, size_t time, class Thread *pThread) :
            m_pEvent(pEvent), m_Time(time), m_pThread(pThread)
        {}
        class Event *m_pEvent;
        size_t m_Time;
        class Thread *m_pThread;
    private:
        Alarm(const Alarm &);
        Alarm &operator = (const Alarm &);
    };

    /** List of alarms. */
    List<Alarm*> m_Alarms;
};

/** @} */

#endif
