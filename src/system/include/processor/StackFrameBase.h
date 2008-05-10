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

#ifndef KERNEL_PROCESSOR_STACKFRAMEBASE_H
#define KERNEL_PROCESSOR_STACKFRAMEBASE_H

#if defined(DEBUGGER)

#include <processor/types.h>
#include <processor/state.h>
#include <utilities/demangle.h>
#include <utilities/StaticString.h>

/** @addtogroup kernelprocessor
 * @{ */

/** Base class for all processor-specific StackFrame classes */
class StackFrameBase
{
  public:
    /** Creates a stack frame based on the given processor state and also the given
     *  symbol name (mangled). */
    StackFrameBase(const ProcessorState &State, uintptr_t basePointer, LargeStaticString mangledSymbol);

    /** The destructor does nothing */
    inline virtual ~StackFrameBase(){}

    /** Returns a pretty printed string containing the function name and each parameter with
     *  its value (hopefully). */
    void prettyPrint(HugeStaticString &buf);

    /** Construct a stack frame, given a ProcessorState. The stack frame should be constructed
     *  to comply with the default ABI for the current architecture - that implies the stack
     *  may be changed, so the getStackPointer() member of ProcessorState must be valid.
     *  \param[out] state The state to modify (construct a stack frame in).
     *  \param returnAddress The return address of the stack frame.
     *  \param nParams The number of parameters to the stack frame.
     *  \param ... The parameters, each sizeof(uintptr_t). */
    static void construct(ProcessorState &state,
                          uintptr_t returnAddress,
                          unsigned int nParams,
                          ...);
    
  protected:
    /** The symbol */
    symbol_t m_Symbol;
    /** The processor state */
    const ProcessorState &m_State;
    /** The base pointer for this frame. */
    uintptr_t m_BasePointer;

  private:
    /** Returns the n'th 32/64-bit parameter in the stack frame. */
    virtual uintptr_t getParameter(size_t n) = 0;

    /** Returns whether the symbol is a class member function.
     *  This is calculated by a simple algorithm:
     *   * If the symbol name contains no '::'s, return false.
     *   * If the letter directly after the last '::' is a capital, return true, else return false. */
    bool isClassMember();

    /** Formats a number, given the 'type' of that number. */
    void format(uintptr_t n, const LargeStaticString &type, HugeStaticString &dest);
};

/** @} */

#endif

#endif
