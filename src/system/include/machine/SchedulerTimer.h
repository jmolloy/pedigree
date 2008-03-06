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
#ifndef KERNEL_MACHINE_SCHEDULERTIMER_H
#define KERNEL_MACHINE_SCHEDULERTIMER_H

#include <processor/types.h>
#include <machine/TimerHandler.h>

/** @addtogroup kernelmachine
 * @{ */

/** Timer for scheduling */
class SchedulerTimer
{
  public:
    /** Get the SchedulerTimer class instance */
    static SchedulerTimer &instance();

    virtual bool registerHandler(TimerHandler *handler) = 0;

  protected:
    /** The default constructor */
    inline SchedulerTimer(){}
    /** The destructor */
    inline virtual ~SchedulerTimer(){}
  private:
    /** The copy-constructor
     *\note NOT implemented */
    SchedulerTimer(const SchedulerTimer &);
    /** The assignment operator
     *\note NOT implemented */
    SchedulerTimer &operator = (const SchedulerTimer &);
};

/** @} */

#endif
