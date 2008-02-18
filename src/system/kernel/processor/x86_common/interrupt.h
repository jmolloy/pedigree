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
#ifndef KERNEL_PROCESSOR_X86_COMMON_INTERRUPT_H
#define KERNEL_PROCESSOR_X86_COMMON_INTERRUPT_H

#include <processor/types.h>
#include <processor/syscall.h>
#include <processor/interrupt.h>
#ifdef X86
  #include "../x86/interrupt.h"
#endif
#ifdef X86_64
  #include "../x86_64/interrupt.h"
#endif

namespace x86_common
{
  /** @addtogroup kernelprocessorx86common x86-common
   * common-x86 processor-specific kernel
   *  @ingroup kernelprocessor
   * @{ */

  /** The interrupt handler on x86 and x64 processors */
  class InterruptManager : public ::InterruptManager, public ::SyscallManager
  {
    friend ::InterruptManager &::InterruptManager::instance();
    public:
      virtual bool registerInterruptHandler(size_t interruptNumber, InterruptHandler *handler);
      virtual bool registerInterruptHandlerDebugger(size_t interruptNumber, InterruptHandler *handler);
      virtual size_t getBreakpointInterruptNumber();
      virtual size_t getDebugInterruptNumber();

      /** Initialises the InterruptManager
       *\note This should only be called from initialiseProcessor() */
      static void initialise();
      /** Initialises this processors IDTR
       *\note This should only be called from initialiseProcessor()
       *\todo and some smp/acpi function */
      static void initialiseProcessor();

    private:
      /** Load the IDT
       *\note This function is defined in kernel/processor/ARCH/interrupt.cc */
      void loadIDT();
      /** Sets up an interrupt gate
       *\param[in] interruptNumber the interrupt number
       *\param[in] interruptHandler address of the assembler interrupt handler stub
       *\param[in] userspace is the userspace allowed to call this callgate?
       *\note This function is defined in kernel/processor/ARCH/interrupt.cc */
      void setInterruptGate(size_t interruptNumber, uintptr_t interruptHandler, bool userspace);
      /** Called when an interrupt was triggered
       *\param[in] interruptNumber the interrupt's number
       *\param[in] interruptState reference to the usermode/kernel state before the interrupt */
      static void interrupt(InterruptState &interruptState);
      /** The constructor */
      InterruptManager();
      /** The destructor */
      virtual ~InterruptManager();

      /** Typedef the correct gate descriptor structure */
      #ifdef X86
        typedef x86::gate_descriptor gate_descriptor;
      #endif
      #ifdef X86_64
        typedef x64::gate_descriptor gate_descriptor;
      #endif

      /** The interrupt descriptor table (IDT) */
      gate_descriptor m_IDT[256];
      /** The interrupt handlers (normal at index 0, kernel debugger at index 1) */
      InterruptHandler *m_Handler[256][2];

      /** The instance of the interrupt manager  */
      static InterruptManager m_Instance;
  };

  /** @} */
}

#endif
