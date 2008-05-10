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

#ifndef KERNEL_MACHINE_TIMER_H
#define KERNEL_MACHINE_TIMER_H

#include <processor/types.h>
#include <machine/TimerHandler.h>

/** @addtogroup kernelmachine
 * @{ */

/** Timer for the time-keeping
 *\todo I don't know yet how to register handlers and how they should
 *      be called back */
class Timer
{
  public:
    /** Get the current year
     *\return the current year */
    virtual size_t getYear() = 0;
    /** Get the current month
     *\return the current month */
    virtual uint8_t getMonth() = 0;
    /** Get the current day of month
     *\return the current day of month */
    virtual uint8_t getDayOfMonth() = 0;
    /** Get the current day of week
     *\return the current day of week */
    virtual uint8_t getDayOfWeek() = 0;
    /** Get the current hour
     *\return the current hour */
    virtual uint8_t getHour() = 0;
    /** Get the current minute
     *\return the current minute */
    virtual uint8_t getMinute() = 0;
    /** Get the current second
     *\return the current second */
    virtual uint8_t getSecond() = 0;

    /** Get the Tick count (time elapsed since system bootup)
     *\return the tick count in milliseconds */
    virtual uint64_t getTickCount() = 0;

    virtual bool registerHandler(TimerHandler *handler) = 0;

  protected:
    /** The default constructor */
    inline Timer(){}
    /** The destructor */
    inline virtual ~Timer(){}
  private:
    /** The copy-constructor
     *\note NOT implemented */
    Timer(const Timer &);
    /** The assignment operator
     *\note NOT implemented */
    Timer &operator = (const Timer &);
};

/**@}*/

#endif
