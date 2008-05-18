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

#ifndef KERNEL_PROCESSOR_PPC32_INTERRUPTMANAGER_H
#define KERNEL_PROCESSOR_PPC32_INTERRUPTMANAGER_H

#include <compiler.h>
#include <processor/types.h>
#include <processor/SyscallManager.h>
#include <processor/InterruptManager.h>

/** @addtogroup kernelprocessorPPC32
 * @{ */

/** The interrupt handler on mips32 processors */
class PPC32InterruptManager : public ::InterruptManager,
                              public ::SyscallManager
{
  public:
    /** Get the PPC32InterruptManager class instance
     *\return instance of the PPC32InterruptManager class */
    inline static PPC32InterruptManager &instance(){return m_Instance;}

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
    static void initialiseProcessor();

  private:
    /** Called when an interrupt was triggered
     *\param[in] interruptState reference to the usermode/kernel state before the interrupt */
    static void interrupt(InterruptState &interruptState);
    /** The constructor */
    PPC32InterruptManager();
    /** Copy constructor
     *\note NOT implemented */
    PPC32InterruptManager(const PPC32InterruptManager &);
    /** Assignment operator
     *\note NOT implemented */
    PPC32InterruptManager &operator = (const PPC32InterruptManager &);
    /** The destructor */
    virtual ~PPC32InterruptManager();

    InterruptHandler *m_Handler[64];
    InterruptHandler *m_ExternalHandler[8];
#ifdef DEBUGGER
    /** The debugger interrupt handlers */
    InterruptHandler *m_DbgHandler[64];
#endif
    /** The syscall handlers */
    SyscallHandler *m_SyscallHandler[SyscallManager::serviceEnd];

    /** The instance of the interrupt manager  */
    static PPC32InterruptManager m_Instance;
};

/** @} */

#endif
