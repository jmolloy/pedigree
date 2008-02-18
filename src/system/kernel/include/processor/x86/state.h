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

#include <processor/types.h>

namespace x86
{
  /** @addtogroup kernelprocessorx86 x86
   * x86 processor-specific kernel
   *  @ingroup kernelprocessor
   * @{ */

  /** x86 Interrupt State */
  class InterruptState
  {
    public:
      // General Interface (both InterruptState and SyscallState)
      inline processor_register_t &stackPointer();
      inline processor_register_t stackPointer() const;
      inline processor_register_t &instructionPointer();
      inline processor_register_t instructionPointer() const;

      // InterruptState Interface
      inline size_t getInterruptNumber() const;

      // SyscallState Interface
      inline size_t getSyscallNumber() const;

    private:
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
  typedef InterruptState SyscallState;

  /** @} */
}

//
// Part of the Implementation
//
processor_register_t &x86::InterruptState::stackPointer()
{
  return m_Esp;
}
processor_register_t x86::InterruptState::stackPointer() const
{
  return m_Esp;
}
processor_register_t &x86::InterruptState::instructionPointer()
{
  return m_Eip;
}
processor_register_t x86::InterruptState::instructionPointer() const
{
  return m_Eip;
}

size_t x86::InterruptState::getInterruptNumber() const
{
  return m_IntNumber;
}

size_t x86::InterruptState::getSyscallNumber() const
{
  // TODO
  return 0;
}

#endif
