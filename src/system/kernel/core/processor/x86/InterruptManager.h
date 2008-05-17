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

#ifndef KERNEL_PROCESSOR_X86_INTERRUPTMANAGER_H
#define KERNEL_PROCESSOR_X86_INTERRUPTMANAGER_H

#include <compiler.h>
#include <Spinlock.h>
#include <processor/types.h>
#include <processor/SyscallManager.h>
#include <processor/InterruptManager.h>

/** @addtogroup kernelprocessorx86
 * @{ */

/** The interrupt handler on x86 processors */
class X86InterruptManager : public ::InterruptManager,
                            public ::SyscallManager
{
  public:
    /** Get the X86InterruptManager class instance
     *\return instance of the X86InterruptManager class */
    inline static X86InterruptManager &instance(){return m_Instance;}

    // InterruptManager Interface
    virtual bool registerInterruptHandler(size_t interruptNumber, InterruptHandler *handler);

    #ifdef DEBUGGER
      virtual bool registerInterruptHandlerDebugger(size_t interruptNumber, InterruptHandler *handler);
      virtual size_t getBreakpointInterruptNumber() PURE;
      virtual size_t getDebugInterruptNumber() PURE;
    #endif

    // SyscallManager Interface
    virtual bool registerSyscallHandler(Service_t Service, SyscallHandler *handler);

    /** Initialises this processors IDTR
     *\note This should only be called from initialiseProcessor()
     *\todo and some smp/acpi function */
    static void initialiseProcessor() INITIALISATION_ONLY;

  private:
    /** Sets up an interrupt gate
     *\param[in] interruptNumber the interrupt number
     *\param[in] interruptHandler address of the assembler interrupt handler stub
     *\param[in] userspace is the userspace allowed to call this callgate?
     *\note This function is defined in kernel/processor/ARCH/interrupt.cc */
    void setInterruptGate(size_t interruptNumber,
                          uintptr_t interruptHandler,
                          bool userspace) INITIALISATION_ONLY;
    /** Called when an interrupt was triggered
     *\param[in] interruptState reference to the usermode/kernel state before the interrupt */
    static void interrupt(InterruptState &interruptState);
    /** The constructor */
    X86InterruptManager() INITIALISATION_ONLY;
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
    /** The syscall handlers */
    SyscallHandler *m_SyscallHandler[SyscallManager::serviceEnd];

    /** Lock */
    Spinlock m_Lock;

    /** The instance of the interrupt manager  */
    static X86InterruptManager m_Instance;
};

/** @} */

#endif
