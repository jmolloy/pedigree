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

#ifndef KERNEL_PROCESSOR_ARMV7_INTERRUPTMANAGER_H
#define KERNEL_PROCESSOR_ARMV7_INTERRUPTMANAGER_H

#include <compiler.h>
#include <processor/types.h>
#include <processor/SyscallManager.h>
#include <processor/InterruptManager.h>
#include <processor/MemoryRegion.h>

/** @addtogroup kernelprocessorARMV7
 * @{ */

/** The interrupt handler on mips32 processors */
class ARMV7InterruptManager : public ::InterruptManager,
                               public ::SyscallManager
{
  public:
    /** Get the ARMV7InterruptManager class instance
     *\return instance of the ARMV7InterruptManager class */
    inline static ARMV7InterruptManager &instance(){return m_Instance;}

    // InterruptManager Interface
    virtual bool registerInterruptHandler(size_t interruptNumber, InterruptHandler *handler);

#ifdef DEBUGGER
    virtual bool registerInterruptHandlerDebugger(size_t interruptNumber, InterruptHandler *handler);
    virtual size_t getBreakpointInterruptNumber() PURE;
    virtual size_t getDebugInterruptNumber() PURE;
#endif

    // SyscallManager Interface
    virtual bool registerSyscallHandler(Service_t Service, SyscallHandler *handler);
    
    virtual uintptr_t syscall(Service_t service, uintptr_t function, uintptr_t p1=0, uintptr_t p2=0, uintptr_t p3=0, uintptr_t p4=0,
                              uintptr_t p5=0);

    /** Initialises this processors IDTR
     *\note This should only be called from initialiseProcessor()
     *\todo and some smp/acpi function */
    static void initialiseProcessor();

    /** Called when an interrupt was triggered
     *\param[in] interruptState reference to the usermode/kernel state before the interrupt */
    static void interrupt(InterruptState &interruptState);

  private:
    /** The constructor */
    ARMV7InterruptManager();
    /** Copy constructor
     *\note NOT implemented */
    ARMV7InterruptManager(const ARMV7InterruptManager &);
    /** Assignment operator
     *\note NOT implemented */
    ARMV7InterruptManager &operator = (const ARMV7InterruptManager &);
    /** The destructor */
    virtual ~ARMV7InterruptManager();

    InterruptHandler *m_Handler[256];
#ifdef DEBUGGER
    /** The debugger interrupt handlers */
    InterruptHandler *m_DbgHandler[256];
#endif
    /** The syscall handlers */
    SyscallHandler *m_SyscallHandler[/*SyscallManager::*/serviceEnd];

    /** MemoryRegion for the MPU MMIO base */
    static MemoryRegion m_MPUINTCRegion;

    /** The instance of the interrupt manager  */
    static ARMV7InterruptManager m_Instance;

    /** Interrupt controller registers */
    enum Registers
    {
        INTCPS_REVISION         = 0x00, // R
        INTCPS_SYSCONFIG        = 0x10 / 4, // RW
        INTCPS_SYSSTATUS        = 0x14 / 4, // R
        INTCPS_SIR_IRQ          = 0x40 / 4, // R
        INTCPS_SIR_FIQ          = 0x44 / 4, // R
        INTCPS_CONTROL          = 0x48 / 4, // RW
        INTCPS_PROTECTION       = 0x4C / 4, // RW
        INTCPS_IDLE             = 0x50 / 4, // RW
        INTCPS_IRQ_PRIORITY     = 0x60 / 4, // RW
        INTCPS_FIQ_PRIORITY     = 0x64 / 4, // RW
        INTCPS_THRESHOLD        = 0x68 / 4, // RW
        INTCPS_ITR              = 0x80 / 4, // R, multiple entries
        INTCPS_MIR              = 0x84 / 4, // RW, as above
        INTCPS_MIR_CLEAR        = 0x88 / 4, // W, as above
        INTCPS_MIR_SET          = 0x8C / 4, // W, as above
        INTCPS_ISR_SET          = 0x90 / 4, // RW, as above
        INTCPS_ISR_CLEAR        = 0x94 / 4, // W, as above
        INTCPS_PENDING_IRQ      = 0x98 / 4, // R, as above
        INTCPS_PENDING_FIQ      = 0x9C / 4, // R, as above
        INTCPS_ILR              = 0x100 / 4, // RW, multiple entries
    };
};

/** @} */

#endif
