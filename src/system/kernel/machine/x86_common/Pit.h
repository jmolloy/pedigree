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
#ifndef KERNEL_MACHINE_X86_COMMON_PIT_H
#define KERNEL_MACHINE_X86_COMMON_PIT_H

#include <processor/io.h>
#include <machine/IrqManager.h>
#include <machine/SchedulerTimer.h>

/** @ingroup kernelmachinex86common
 * @{ */

/** The programmable intervall timer implements the SchedulerTimer interface */
class Pit : public SchedulerTimer,
            private IrqHandler
{
  public:
    /** Get the Pit class instance */
    inline static Pit &instance(){return m_Instance;}

    //
    // SchedulerTimer interface
    //
    virtual bool registerHandler(TimerHandler *handler);

    /** Initialises the class
     *\return true, if successfull, false otherwise */
    bool initialise();
     /** Uninitialises the class */
    void uninitialise();

  protected:
    /** The default constructor */
    Pit();
    /** The destructor */
    inline virtual ~Pit(){}

  private:
    /** The copy-constructor
     *\note NOT implemented */
    Pit(const Pit &);
    /** The assignment operator
     *\note NOT implemented */
    Pit &operator = (const Pit &);

    //
    // IrqHandler interface
    //
    virtual bool irq(irq_id_t number);

    /** The PIT I/O port range */
    IoPort m_IoPort;
    /** The PIT IRQ Id */
    irq_id_t m_IrqId;
    /** The scheduler */
    TimerHandler *m_Handler;

    /** The Pit class instance */
    static Pit m_Instance;
};

/** @} */

#endif
