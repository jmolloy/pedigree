/*
 * Copyright (c) 2010 Matthew Iselin
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
#include <machine/Timer.h>
#include <machine/SchedulerTimer.h>
#include <processor/MemoryRegion.h>
#include <processor/InterruptManager.h>
#include <utilities/List.h>

#define MAX_TIMER_HANDLERS    32

/** Driver for the 32 kHz "Sync Timer" on the system, which provides a nice
  * tick count mechanism similar to what we use for log timestamps on x86,
  * except in hardware.
  */
class SyncTimer
{
    public:
        SyncTimer() : m_MmioBase("SyncTimer")
        {
        }
        virtual ~SyncTimer()
        {
        }

        void initialise(uintptr_t base);

        uint32_t getTickCount();

    private:
        MemoryRegion m_MmioBase;
};

/** Driver for a general purpose timer - provide the MMIO base during
  * initialisation and it'll be able to generate IRQs and call handlers
  * and all that good stuff.
  */
class GPTimer : public Timer, public SchedulerTimer, public InterruptHandler
{
  public:

    /** The default constructor */
    inline GPTimer() :
        m_MmioBase("GPTimer"), m_bIrqInstalled(false), m_Irq(0),
        m_Handlers(), m_Alarms(), m_TickCount(0)
    {
        for(int i = 0; i < MAX_TIMER_HANDLERS; i++)
            m_Handlers[i] = 0;
    }
    /** The destructor */
    inline virtual ~GPTimer(){}

    /** Initialises this timer */
    void initialise(size_t timer, uintptr_t base);

    /** Get the current year
     *\return the current year */
    virtual size_t getYear()
    {
        return 0;
    }
    /** Get the current month
     *\return the current month */
    virtual uint8_t getMonth()
    {
        return 0;
    }
    /** Get the current day of month
     *\return the current day of month */
    virtual uint8_t getDayOfMonth()
    {
        return 0;
    }
    /** Get the current day of week
     *\return the current day of week */
    virtual uint8_t getDayOfWeek()
    {
        return 0;
    }
    /** Get the current hour
     *\return the current hour */
    virtual uint8_t getHour()
    {
        return 0;
    }
    /** Get the current minute
     *\return the current minute */
    virtual uint8_t getMinute()
    {
        return 0;
    }
    /** Get the current second
     *\return the current second */
    virtual uint8_t getSecond()
    {
        return 0;
    }

    /** Get the Tick count (time elapsed since system bootup)
     *\return the tick count in milliseconds */
    virtual uint64_t getTickCount()
    {
        extern SyncTimer g_SyncTimer;
        return g_SyncTimer.getTickCount();
    }

    virtual bool registerHandler(TimerHandler *handler);
    virtual bool unregisterHandler(TimerHandler *handler);

    /** Dispatches the Event \p pEvent to the current thread in \p alarmSecs time.
     *\param pEvent Event to dispatch.
     *\param alarmSecs Number of seconds to wait.
     */
    virtual void addAlarm(class Event *pEvent, size_t alarmSecs, size_t alarmUsecs=0);
    /** Removes the event \p pEvent from the alarm queue.
     *\param pEvent Event to remove alarm for. */
    virtual void removeAlarm(class Event *pEvent);
    /** Removes the event \p pEvent from the alarm queue.
     *\param pEvent Event to remove alarm for.
     *\param bRetZero If true, returns zero rather the time until firing
     *\return The number of seconds before the event would have fired,
     *        or zero if bRetZero is true. */
    virtual size_t removeAlarm(class Event *pEvent, bool bRetZero);

  private:
    /** The copy-constructor
     *\note NOT implemented */
    GPTimer(const Timer &);
    /** The assignment operator
     *\note NOT implemented */
    GPTimer &operator = (const Timer &);

    virtual void interrupt(size_t nInterruptnumber, InterruptState &state);

    /** MMIO base */
    MemoryRegion m_MmioBase;

    /** Whether the IRQ for this timer is installed. This allows lazy installation
      * of the IRQ handler - we only need to get IRQs for timers which have handlers
      * after all.
      */
    bool m_bIrqInstalled;

    /** Internal IRQ number */
    size_t m_Irq;

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

    /** Internal tick count - milliseconds, used for alarms and things */
    uint64_t m_TickCount;

    /** Register offsets */
    enum Registers
    {
        TIDR            = 0x00 / 4, // R
        TIOCP_CFG       = 0x10 / 4, // RW
        TISTAT          = 0x14 / 4, // R
        TISR            = 0x18 / 4, // RW
        TIER            = 0x1C / 4, // RW
        TWER            = 0x20 / 4, // RW
        TCLR            = 0x24 / 4, // RW
        TCRR            = 0x28 / 4, // RW
        TLDR            = 0x2C / 4, // RW
        TTGR            = 0x30 / 4, // RW
        TWPS            = 0x34 / 4, // R
        TMAR            = 0x38 / 4, // RW
        TCAR1           = 0x3C / 4, // R
        TSICR           = 0x40 / 4, // RW
        TCAR2           = 0x44 / 4, // R
        TPIR            = 0x48 / 4, // RW
        TNIR            = 0x4C / 4, // RW
        TCVR            = 0x50 / 4, // RW
        TOCR            = 0x54 / 4, // RW
        TOWR            = 0x58 / 4, // RW
    };
};
