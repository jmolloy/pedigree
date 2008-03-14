/*
 * Copyright (c) 2008 James Molloy, James Pritchett, Jörg Pfähler, Matthew Iselin
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
#ifndef KERNEL_MACHINE_X86_COMMON_RTC_H
#define KERNEL_MACHINE_X86_COMMON_RTC_H

#include <processor/IoPort.h>
#include <machine/IrqManager.h>
#include <machine/Timer.h>

/** @addtogroup kernelmachinex86common
 * @{ */

/** Class for the Real-time clock / CMOS implementing the Timer interface */
class Rtc : public Timer,
            private IrqHandler
{
  public:
    inline static Rtc &instance(){return m_Instance;}

    //
    // Timer interface
    //
    virtual bool registerHandler(TimerHandler *handler);
    virtual size_t getYear();
    virtual uint8_t getMonth();
    virtual uint8_t getDayOfMonth();
    virtual uint8_t getDayOfWeek();
    virtual uint8_t getHour();
    virtual uint8_t getMinute();
    virtual uint8_t getSecond();
    virtual uint64_t getTickCount();

    /** Initialises the class
     *\return true, if successfull, false otherwise */
    bool initialise();
    /** Synchronise the time/date with the hardware
     *\todo We might want to add this to the Timer interface */
    void synchronise();
    /** Uninitialises the class */
    void uninitialise();

  protected:
    /** The default constructor */
    Rtc();
    /** The destructor */
    inline virtual ~Rtc(){}

  private:
    /** The copy-constructor
     *\note NOT implemented */
    Rtc(const Rtc &);
    /** The assignment operator
     *\note NOT implemented */
    Rtc &operator = (const Rtc &);

    //
    // IrqHandler interface
    //
    virtual bool irq(irq_id_t number);

    /** Set the index register
     *\param[in] index the new index */
    void setIndex(uint8_t index);
    /** Wait until the Update of the RTC entries in the CMOS is complete,
     * if we want to access one of those entries
     *\param[in] index the index we want to access afterwards */
    void waitForUpdateCompletion(uint8_t index);
    /** Dis/Enable the RTC updates
     *\param[in] index the index we want to access / have accessed
     *\param[in] enable Do we want to enable or disable? */
    void enableRtcUpdates(bool enable);
    /** Read the value in the CMOS at a specific index
     *\param[in] index the index
     *\return the value at the index */
    uint8_t read(uint8_t index);
    /** Write the value to the CMOS at a specific index
     *\param[in] index the index
     *\param[in] value the value */
    void write(uint8_t index, uint8_t value);

    /** The CMOS/Real-time Clock I/O port range */
    IoPort m_IoPort;

    /** The IRQ Identifier */
    irq_id_t m_IrqId;

    /** Index into the periodicIrqInfo table */
    size_t m_PeriodicIrqInfoIndex;

    /** BCD mode? (otherwise in binary mode) */
    bool m_bBCD;

    /** The current year */
    size_t m_Year;
    /** The current month */
    uint8_t m_Month;
    /** The current day of month */
    uint8_t m_DayOfMonth;
    /** The current hour */
    uint8_t m_Hour;
    /** The current minute */
    uint8_t m_Minute;
    /** The current second */
    uint8_t m_Second;
    /** The current nanosecond */
    uint64_t m_Nanosecond;

    /** The current tick count in nanoseconds */
    uint64_t m_TickCount;

    /** Holds information about the RTC periodic irq */
    struct periodicIrqInfo_t
    {
      /** The frequency */
      size_t Hz;
      /** Value that needs to be written to the CMOS register */
      uint8_t rateBits;
      /** Nanoseconds between two ticks */
      uint64_t ns[2];
    };

    /** Information about the RTC's periodic irq */
    static periodicIrqInfo_t periodicIrqInfo[6];

    /** The Rtc class instance */
    static Rtc m_Instance;
};

/** @} */

#endif
