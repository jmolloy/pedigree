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
#ifndef KERNEL_PROCESSOR_X64_INTERRUPTMANAGER_H
#define KERNEL_PROCESSOR_X64_INTERRUPTMANAGER_H

#include <compiler.h>
#include <processor/types.h>
#include <processor/InterruptManager.h>

/** @addtogroup kernelprocessorx64 x64
 * x64 processor-specific kernel
 *  @ingroup kernelprocessor
 * @{ */

/** The interrupt handler on x64 processors */
class X64InterruptManager : public ::InterruptManager
{
  public:
    /** Get the X64InterruptManager class instance
     *\return instance of the X64InterruptManager class */
    inline static X64InterruptManager &instance(){return m_Instance;}

    virtual bool registerInterruptHandler(size_t interruptNumber, InterruptHandler *handler);

    #ifdef DEBUGGER
      virtual bool registerInterruptHandlerDebugger(size_t interruptNumber, InterruptHandler *handler);
      virtual size_t getBreakpointInterruptNumber() PURE;
      virtual size_t getDebugInterruptNumber() PURE;
    #endif

    /** Initialises this processors IDTR
     *\note This should only be called from initialiseProcessor()
     *\todo and some smp/acpi function */
    static void initialiseProcessor();

  private:
    /** Sets up an interrupt gate
     *\param[in] interruptNumber the interrupt number
     *\param[in] interruptHandler address of the assembler interrupt handler stub
     *\note This function is defined in kernel/processor/ARCH/interrupt.cc */
    void setInterruptGate(size_t interruptNumber, uintptr_t interruptHandler);
    /** Called when an interrupt was triggered
     *\param[in] interruptState reference to the usermode/kernel state before the interrupt */
    static void interrupt(InterruptState &interruptState);
    /** The constructor */
    X64InterruptManager();
    /** Copy constructor
     *\note NOT implemented */
    X64InterruptManager(const X64InterruptManager &);
    /** Assignment operator
     *\note NOT implemented */
    X64InterruptManager &operator = (const X64InterruptManager &);
    /** The destructor */
    virtual ~X64InterruptManager();

    /** Structure of a x64 long-mode gate descriptor */
    struct gate_descriptor
    {
      /** Bits 0-15 of the offset */
      uint16_t offset0;
      /** The segment selector */
      uint16_t selector;
      /** Entry number in the interrupt-service-table */
      uint8_t ist;
      /** Flags */
      uint8_t flags;
      /** Bits 16-31 of the offset */
      uint16_t offset1;
      /** Bits 32-63 of the offset */
      uint32_t offset2;
      /** Reserved, must be 0 */
      uint32_t res;
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
    static X64InterruptManager m_Instance;
};

/** @} */

#endif
