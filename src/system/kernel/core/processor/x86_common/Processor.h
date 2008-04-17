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
#ifndef KERNEL_PROCESSOR_X86_COMMON_PROCESSOR_H
#define KERNEL_PROCESSOR_X86_COMMON_PROCESSOR_H

#include <processor/types.h>
#include <processor/Processor.h>

/** @addtogroup kernelprocessorx86common
 * @{ */

/** Common x86 processor information structure
 *\todo Local APIC Id, pointer to the TSS */
class X86CommonProcessor : public Processor::Information
{
  public:
    /** Construct a X86CommonProcessor object
     *\param[in] ProcessorId Identifier of the processor
     *\todo the local APIC id */
    inline X86CommonProcessor(Processor::Id ProcessorId)
      : Information(ProcessorId){}
    /** The destructor does nothing */
    inline virtual ~X86CommonProcessor(){}

  private:
    /** Default constructor
     *\note NOT implemented */
    X86CommonProcessor();
    /** Copy-constructor
     *\note NOT implemented */
    X86CommonProcessor(const X86CommonProcessor &);
    /** Assignment operator
     *\note NOT implemented */
    X86CommonProcessor &operator = (const X86CommonProcessor &);
};

/** @} */

#endif
