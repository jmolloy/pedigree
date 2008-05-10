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

#ifndef KERNEL_MACHINE_MIPS_COMMON_TIMER_H
#define KERNEL_MACHINE_MIPS_COMMON_TIMER_H

#include <processor/InterruptHandler.h>
#include <machine/SchedulerTimer.h>
#include <processor/state.h>

/** @addtogroup kernelmachinex86common
 * @{ */

/** The programmable interval timer implements the SchedulerTimer interface */
class CountCompareTimer : public SchedulerTimer,
                          private InterruptHandler
{
  public:
    /** Get the CountCompareTimer class instance */
    inline static CountCompareTimer &instance(){return m_Instance;}

    //
    // SchedulerTimer interface
    //
    virtual bool registerHandler(TimerHandler *handler);

    /** Initialises the class
     *\return true, if successful, false otherwise */
    bool initialise() INITIALISATION_ONLY;
     /** Uninitialises the class */
    void uninitialise();

  protected:
    /** The default constructor */
    CountCompareTimer() INITIALISATION_ONLY;
    /** The destructor */
    inline virtual ~CountCompareTimer(){}

  private:
    /** The copy-constructor
     *\note NOT implemented */
    CountCompareTimer(const CountCompareTimer &);
    /** The assignment operator
     *\note NOT implemented */
    CountCompareTimer &operator = (const CountCompareTimer &);

    //
    // InterruptHandler interface
    //
    virtual void interrupt(size_t interruptNumber, InterruptState &state);

    /** The scheduler */
    TimerHandler *m_Handler;

    /** The CountCompareTimer class instance */
    static CountCompareTimer m_Instance;

    uint32_t m_Compare;
};

/** @} */

#endif
