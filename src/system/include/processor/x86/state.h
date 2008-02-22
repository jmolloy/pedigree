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
#ifndef KERNEL_PROCESSOR_X86_STATE_H
#define KERNEL_PROCESSOR_X86_STATE_H

#include <processor/processor.h>
#include <processor/types.h>

/** @addtogroup kernelprocessorx86 x86
 * x86 processor-specific kernel
 *  @ingroup kernelprocessor
 * @{ */

/** x86 Interrupt State */
class X86InterruptState
{
  public:
    // Default constructor creates its contents from the current state.
	X86InterruptState() :
      m_Ds(0), m_Edi(0), m_Esi(0),
      m_Ebp(processor::getBasePointer()),
      m_Res(0), m_Ebx(0), m_Edx(0), m_Ecx(0), m_Eax(0),
      m_IntNumber(0), m_Errorcode(0),
      m_Eip(processor::getInstructionPointer()),
      m_Cs(0), m_Eflags(0),
      m_Esp(processor::getStackPointer()),
      m_Ss(0)
    {
    }
	
    // General Interface (both InterruptState and SyscallState)
    inline uintptr_t &stackPointer();
    inline uintptr_t stackPointer() const;
    inline uintptr_t &instructionPointer();
    inline uintptr_t instructionPointer() const;
    inline uintptr_t &basePointer();
    inline uintptr_t basePointer() const;

    // InterruptState Interface
    inline size_t getInterruptNumber() const;

    // SyscallState Interface
    inline size_t getSyscallNumber() const;

  private:
    uint32_t m_Ds;
    uint32_t m_Edi;
    uint32_t m_Esi;
    uint32_t m_Ebp;
    uint32_t m_Res;
    uint32_t m_Ebx;
    uint32_t m_Edx;
    uint32_t m_Ecx;
    uint32_t m_Eax;
    uint32_t m_IntNumber;
    uint32_t m_Errorcode;
    uint32_t m_Eip;
    uint32_t m_Cs;
    uint32_t m_Eflags;
    uint32_t m_Esp;
    uint32_t m_Ss;
};

/** x86 SyscallState */
typedef X86InterruptState X86SyscallState;

/** @} */

//
// Part of the Implementation
//
uintptr_t &X86InterruptState::stackPointer()
{
  return m_Esp;
}
uintptr_t X86InterruptState::stackPointer() const
{
  return m_Esp;
}
uintptr_t &X86InterruptState::instructionPointer()
{
  return m_Eip;
}
uintptr_t X86InterruptState::instructionPointer() const
{
  return m_Eip;
}
uintptr_t &X86InterruptState::basePointer()
{
  return m_Ebp;
}
uintptr_t X86InterruptState::basePointer() const
{
  return m_Ebp;
}

size_t X86InterruptState::getInterruptNumber() const
{
  return m_IntNumber;
}

size_t X86InterruptState::getSyscallNumber() const
{
  // TODO
  return 0;
}

#endif
