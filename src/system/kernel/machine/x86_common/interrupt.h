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
#ifndef KERNEL_MACHINE_X86_COMMON_INTERRUPT_H
#define KERNEL_MACHINE_X86_COMMON_INTERRUPT_H

#include <machine/types.h>
#include <machine/interrupt.h>
#ifdef X86
  #include "../x86/interrupt.h"
#endif
#ifdef X86_64
  #include "../x86_64/interrupt.h"
#endif

/** @addtogroup kernelmachinex86common x86-common
 * common-x86 machine-specific kernel
 *  @ingroup kernelmachine
 * @{ */

namespace x86_common
{
  /** The interrupt handler on x86 and x86-64 machines */
  class InterruptManager : public ::InterruptManager
  {
    friend ::InterruptManager &::InterruptManager::instance();
    public:
      virtual void initialise();
      virtual bool registerInterruptHandler(size_t interruptNumber, InterruptHandler *handler);
      virtual bool registerInterruptHandlerDebugger(size_t interruptNumber, InterruptHandler *handler);
      virtual size_t getBreakpointInterruptNumber();
      virtual size_t getDebugInterruptNumber();
    
    private:
      /** Called when an interrupt was triggered
       *\param[in] interruptNumber the interrupt's number
       *\param[in] interruptState reference to the usermode/kernel state before the interrupt */
      static void interrupt(size_t interruptNumber, InterruptState &interruptState);
      /** The constructor */
      InterruptManager();
      /** The destructor */
      virtual ~InterruptManager();
      
      /** Typedef the correct gate descriptor structure */
      #ifdef X86
        typedef x86::gate_descriptor gate_descriptor;
      #endif
      #ifdef X86_64
        typedef x86_64::gate_descriptor gate_descriptor;
      #endif
      
      /** The interrupt descriptor table (IDT) */
      gate_descriptor m_IDT[256];
      /** The interrupt handlers (normal at index 0, kernel debugger at index 1) */
      InterruptHandler *m_Handler[256][2];
      
      /** The instance of the interrupt manager  */
      static InterruptManager m_Instance;
  };
}

/** @} */

#endif
