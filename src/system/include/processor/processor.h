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
#ifndef KERNEL_PROCESSOR_PROCESSOR_H
#define KERNEL_PROCESSOR_PROCESSOR_H

#include <processor/types.h>
#include <processor/state.h>

/** @addtogroup kernelprocessor processor-specifc kernel
 * processor-specific kernel interface
 *  @ingroup kernel
 * @{ */

/** Interface to the processor */
class Processor
{
  friend class ProcessorState;
  public:
    /** Get the base-pointer of the calling function
     *\return base-pointer of the calling function */
    static uintptr_t getBasePointer();
    /** Get the stack-pointer of the calling function
     *\return stack-pointer of the calling function */
    static uintptr_t getStackPointer();
    /** Get the instruction-pointer of the calling function
     *\return instruction-pointer of the calling function */
    static uintptr_t getInstructionPointer();

    #ifdef X86_COMMON
      /** Read a Machine/Model-specific register
       *\param[in] index the register index
       *\return the value of the register */
      static uint64_t readMachineSpecificRegister(uint32_t index);
      /** Write a Machine/Model-specific register
       *\param[in] index the register index
       *\param[in] value the new value of the register */
      static void writeMachineSpecificRegister(uint32_t index, uint64_t value);
    #endif
};

/** @} */

#endif
