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
#ifndef KERNEL_PROCESSOR_X86_COMMON_PHYSICALMEMORYMANAGER_H
#define KERNEL_PROCESSOR_X86_COMMON_PHYSICALMEMORYMANAGER_H

#include <processor/PhysicalMemoryManager.h>
#include <BootstrapInfo.h>

/** @addtogroup kernelprocessorx86common
 * @{ */

class X86CommonPhysicalMemoryManager : public PhysicalMemoryManager
{
  public:
    /** Get the X86CommonPhysicalMemoryManager instance
     *\return instance of the X86CommonPhysicalMemoryManager */
    inline static X86CommonPhysicalMemoryManager &instance(){return m_Instance;}

    void initialise(BootstrapInfo_t &Info);

  protected:
    /** The constructor */
    X86CommonPhysicalMemoryManager();
    /** The destructor */
    virtual ~X86CommonPhysicalMemoryManager();

  private:
    /** The copy-constructor
     *\note Not implemented (singleton) */
    X86CommonPhysicalMemoryManager(const X86CommonPhysicalMemoryManager &);
    /** The copy-constructor
     *\note Not implemented (singleton) */
    X86CommonPhysicalMemoryManager &operator = (const X86CommonPhysicalMemoryManager &);

    static X86CommonPhysicalMemoryManager m_Instance;
};

/** @} */

#endif
