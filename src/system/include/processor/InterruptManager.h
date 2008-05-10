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

#ifndef KERNEL_PROCESSOR_INTERRUPTMANAGER_H
#define KERNEL_PROCESSOR_INTERRUPTMANAGER_H

#include <processor/types.h>
#include <processor/state.h>
#include <processor/InterruptHandler.h>

/** @addtogroup kernelprocessor
 * @{ */

/** The interrupt manager allows interrupt handler registrations and handles interrupts.
 *\brief Handles interrupts and interrupt registrations from kernel components */
class InterruptManager
{
  public:
    /** Get the interrupt manager instance
     *\return instance of the interrupt manager */
    static InterruptManager &instance();
    /** Register an interrupt handler
     *\param[in] interruptNumber the interrupt's number
     *\param[in] handler the interrupt handler
     *\return true, if successfully registered, false otherwise */
    virtual bool registerInterruptHandler(size_t interruptNumber, InterruptHandler *handler) = 0;

    #if defined(DEBUGGER)
      /** Register an interrupt handler (for the kernel debugger)
       *\param[in] interruptNumber the interrupt's number
       *\param[in] handler the interrupt handler
       *\return true, if successfully registered, false otherwise */
      virtual bool registerInterruptHandlerDebugger(size_t interruptNumber, InterruptHandler *handler) = 0;
      /** Get the interrupt number of the breakpoint exception
       *\return the interrupt number of the breakpoint exception */
      virtual size_t getBreakpointInterruptNumber() PURE = 0;
      /** Get the interrupt number of the debug exception
       *\return the interrupt number of the debug exception */
      virtual size_t getDebugInterruptNumber() PURE = 0;
    #endif

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
