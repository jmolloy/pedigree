/*
 * Copyright (c) 2008-2014, Pedigree Developers
 *
 * Please see the CONTRIB file in the root of the source tree for a full
 * list of contributors.
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

#ifndef KERNEL_PROCESSOR_X64_SYSCALLMANAGER_H
#define KERNEL_PROCESSOR_X64_SYSCALLMANAGER_H

#include <Spinlock.h>
#include <processor/types.h>
#include <processor/SyscallManager.h>

/** @addtogroup kernelprocessorx64
 * @{ */

/** The syscall manager on x64 processors */
class X64SyscallManager : public ::SyscallManager
{
  friend class Processor;
  public:
    /** Get the X64SyscallManager class instance
     *\return instance of the X64SyscallManager class */
    inline static X64SyscallManager &instance(){return m_Instance;}

    virtual bool registerSyscallHandler(Service_t Service, SyscallHandler *pHandler);

    /** Initialises this processors syscall handling
     *\note This should only be called from Processor::initialise1() and
     *      Multiprocessor::applicationProcessorStartup() */
    static void initialiseProcessor() INITIALISATION_ONLY;

    /** Called to execute a syscall. */
    uintptr_t syscall(Service_t service, uintptr_t function, uintptr_t p1, uintptr_t p2, uintptr_t p3, uintptr_t p4, uintptr_t p5);

  private:
    /** Called when a syscall was called
     *\param[in] syscallState reference to the usermode state before the syscall */
    static void syscall(SyscallState &syscallState);

    /** The constructor */
    X64SyscallManager() INITIALISATION_ONLY;
    /** Copy constructor
     *\note NOT implemented */
    X64SyscallManager(const X64SyscallManager &);
    /** Assignment operator
     *\note NOT implemented */
    X64SyscallManager &operator = (const X64SyscallManager &);
    /** The destructor */
    virtual ~X64SyscallManager();

    /** Spinlock protecting the member variables */
    Spinlock m_Lock;

    /** The syscall handlers */
    SyscallHandler *m_pHandler[serviceEnd];

    /** The instance of the syscall manager  */
    static X64SyscallManager m_Instance;
};

/** @} */

#endif
