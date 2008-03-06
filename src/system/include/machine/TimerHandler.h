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
#ifndef KERNEL_MACHINE_TIMERHANDLER_H
#define KERNEL_MACHINE_TIMERHANDLER_H

#include <processor/types.h>

/** @addtogroup kernelmachine
 * @{ */

/** Abstract base class for all timer handlers. All those handlers must
 * be derived from this class */
class TimerHandler
{
  public:
    /** Called when the handler is registered with the Timer/SchedulerTimer class
     * and a timer event occured
     *\param[in] delta time elapsed since the last event
     *\todo which unit for delta? ns? ms? */
    virtual void timer(uint64_t delta) = 0;

  protected:
    /** Virtual destructor */
    inline virtual ~TimerHandler(){}
};

/** @} */

#endif
