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
#ifndef KERNEL_PROCESSOR_INTERRUPT_H
#define KERNEL_PROCESSOR_INTERRUPT_H

#include <processor/types.h>
#include <processor/state.h>

/** @addtogroup kernelprocessor processor-specifc kernel
 * processor-specific kernel interface
 *  @ingroup kernel
 * @{ */

/** Abstract base class for all interrupt-handlers. All interrupt-handlers must
 * be derived from this class */
class InterruptHandler
{
  public:
    /** Called when the handler is registered with the interrupt manager and the interrupt occurred */
    virtual void interrupt(size_t interruptNumber, InterruptState &state) = 0;
};

/** The interrupt manager allows interrupt handler registrations and handles interrupts */
class InterruptManager
{
  public:
    /** Get the interrupt handler instance
     *\return instance of the interrupt handler */
    static InterruptManager &instance();
    /** Register an interrupt handler
     *\param[in] interruptNumber the interrupt's number
     *\param[in] handler the interrupt handler
     *\return true, if successfully registered, false otherwise */
    virtual bool registerInterruptHandler(size_t interruptNumber, InterruptHandler *handler) = 0;
    /** Register an interrupt handler (for the kernel debugger)
     *\param[in] interruptNumber the interrupt's number
     *\param[in] handler the interrupt handler
     *\return true, if successfully registered, false otherwise */
    virtual bool registerInterruptHandlerDebugger(size_t interruptNumber, InterruptHandler *handler) = 0;
    /** Get the interrupt number of the breakpoint exception
     *\return the interrupt number of the breakpoint exception */
    virtual size_t getBreakpointInterruptNumber() = 0;
    /** Get the interrupt number of the debug exception
     *\return the interrupt number of the debug exception */
    virtual size_t getDebugInterruptNumber() = 0;

  protected:
    /** The constructor */
    inline InterruptManager();
    /** The destructor */
    inline virtual ~InterruptManager();

  private:
    /** The copy-constructor
     *\note Not implemented (singleton) */
    InterruptManager(const InterruptManager &);
    /** The copy-constructor
     *\note Not implemented (singleton) */
    InterruptManager &operator = (const InterruptManager &);
};

/** @} */

//
// Part of the Implementation
//
InterruptManager::InterruptManager()
{
}
InterruptManager::~InterruptManager()
{
}

#endif
