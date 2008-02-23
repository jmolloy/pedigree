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
#ifndef KERNEL_PROCESSOR_X86_GDT_H
#define KERNEL_PROCESSOR_X86_GDT_H

#include <compiler.h>
#include <processor/types.h>

/** @addtogroup kernelprocessorx86 x86
 * x86 processor-specific kernel
 *  @ingroup kernelprocessor
 * @{ */

/** The GDT manager on x86 processors */
class X86GdtManager
{
  public:
    /** Get the gdt manager instance
     *\return instance of the gdt manager */
    static X86GdtManager &instance();
    /** Initialises this processors GDTR
     *\note This should only be called from initialiseProcessor()
     *\todo and some smp/acpi function */
    static void initialiseProcessor();

  private:
    /** The constructor */
    X86GdtManager();
    /** Copy constructor
     *\note NOT implemented */
    X86GdtManager(const X86GdtManager &);
    /** Assignment operator
     *\note NOT implemented */
    X86GdtManager &operator = (const X86GdtManager &);
    /** The destructor */
    ~X86GdtManager();

    /** The instance of the gdt manager  */
    static X86GdtManager m_Instance;
};

/** @} */

#endif
