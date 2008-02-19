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
#ifndef KERNEL_PROCESSOR_X86_INTERRUPT_H
#define KERNEL_PROCESSOR_X86_INTERRUPT_H

#include <compiler.h>
#include <processor/types.h>
#include <processor/syscall.h>
#include <processor/interrupt.h>

/** @addtogroup kernelprocessorx86 x86
 * x86 processor-specific kernel
 *  @ingroup kernelprocessor
 * @{ */

/** The interrupt handler on x86 processors */
class X86InterruptManager : public ::InterruptManager,
                            public ::SyscallManager
{
  friend ::SyscallManager &::SyscallManager::instance();
  friend ::InterruptManager &::InterruptManager::instance();
  public:
    // InterruptManager Interface
    virtual bool registerInterruptHandler(size_t interruptNumber, InterruptHandler *handler);

    #ifdef DEBUGGER
      virtual bool registerInterruptHandlerDebugger(size_t interruptNumber, InterruptHandler *handler);
      virtual size_t getBreakpointInterruptNumber() PURE;
      virtual size_t getDebugInterruptNumber() PURE;
    #endif

    // SyscallManager Interface
    virtual bool registerSyscallHandler(Service_t Service, SyscallHandler *handler);

    /** Initialises the InterruptManager
     *\note This should only be called from initialiseProcessor() */
    static void initialise();
    /** Initialises this processors IDTR
     *\note This should only be called from initialiseProcessor()
     *\todo and some smp/acpi function */
    static void initialiseProcessor();

  private:
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
    X86InterruptManager();
    /** Copy constructor
     *\note NOT implemented */
    X86InterruptManager(const X86InterruptManager &);
    /** Assignment operator
     *\note NOT implemented */
    X86InterruptManager &operator = (const X86InterruptManager &);
    /** The destructor */
    virtual ~X86InterruptManager();

    /** Structure of a x86 protected-mode gate descriptor */
    struct gate_descriptor
    {
      /** Bits 0-15 of the offset */
      uint16_t offset0;
      /** The segment selector */
      uint16_t selector;
      /** Reserved, must be 0 */
      uint8_t res;
      /** Flags */
      uint8_t flags;
      /** Bits 16-31 of the offset */
      uint16_t offset1;
    } PACKED;

    /** The interrupt descriptor table (IDT) */
    gate_descriptor m_IDT[256];
    /** The normal interrupt handlers */
    InterruptHandler *m_Handler[256];
    #ifdef DEBUGGER
      /** The debugger interrupt handlers */
      InterruptHandler *m_DbgHandler[256];
    #endif

    /** The instance of the interrupt manager  */
    static X86InterruptManager m_Instance;
};

/** @} */

#endif
