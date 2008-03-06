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
#ifndef KERNEL_PROCESSOR_X64_SYSCALLMANAGER_H
#define KERNEL_PROCESSOR_X64_SYSCALLMANAGER_H

#include <processor/types.h>
#include <processor/SyscallManager.h>

/** @addtogroup kernelprocessorx64
 * @{ */

/** The syscall handler on x64 processors */
class X64SyscallManager : public ::SyscallManager
{
  public:
    /** Get the X64SyscallManager class instance
     *\return instance of the X64SyscallManager class */
    inline static X64SyscallManager &instance(){return m_Instance;}

    virtual bool registerSyscallHandler(Service_t Service, SyscallHandler *handler);

    /** Initialises this processors syscall handling
     *\note This should only be called from initialiseProcessor()
     *\todo and some smp/acpi function */
    static void initialiseProcessor();

  private:
    /** Called when a syscall was called
     *\param[in] syscallState reference to the usermode state before the syscall */
    static void syscall(SyscallState &syscallState);
    /** The constructor */
    X64SyscallManager();
    /** Copy constructor
     *\note NOT implemented */
    X64SyscallManager(const X64SyscallManager &);
    /** Assignment operator
     *\note NOT implemented */
    X64SyscallManager &operator = (const X64SyscallManager &);
    /** The destructor */
    virtual ~X64SyscallManager();

    /** The syscall handlers */
    SyscallHandler *m_Handler[SyscallManager::serviceEnd];

    /** The instance of the syscall manager  */
    static X64SyscallManager m_Instance;
};

/** @} */

#endif
