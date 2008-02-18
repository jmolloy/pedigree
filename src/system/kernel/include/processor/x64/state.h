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
#ifndef KERNEL_PROCESSOR_X64_STATE_H
#define KERNEL_PROCESSOR_X64_STATE_H

#include <processor/types.h>

namespace x64
{
  /** @addtogroup kernelprocessorx64 x64
   * x64 processor-specific kernel
   *  @ingroup kernelprocessor
   * @{ */

  /** x64 Interrupt State */
  class InterruptState
  {
    public:
      inline processor_register_t &stackPointer();
      inline processor_register_t stackPointer() const;
      inline processor_register_t &instructionPointer();
      inline processor_register_t instructionPointer() const;

      inline size_t getInterruptNumber() const;

    private:
      uint64_t m_R15;
      uint64_t m_R14;
      uint64_t m_R13;
      uint64_t m_R12;
      uint64_t m_R11;
      uint64_t m_R10;
      uint64_t m_R9;
      uint64_t m_R8;
      uint64_t m_Rbp;
      uint64_t m_Rsi;
      uint64_t m_Rdi;
      uint64_t m_Rdx;
      uint64_t m_Rcx;
      uint64_t m_Rbx;
      uint64_t m_Rax;
      uint64_t m_IntNumber;
      uint64_t m_Errorcode;
      uint64_t m_Rip;
      uint64_t m_Cs;
      uint64_t m_Rflags;
      uint64_t m_Rsp;
      uint64_t m_Ss;
  };

  class SyscallState
  {
    public:
      /// TODO
      inline processor_register_t &stackPointer();
      inline processor_register_t &instructionPointer();
      inline processor_register_t &instructionPointer();
      inline processor_register_t instructionPointer() const;

      inline size_t getSyscallNumber() const;

    private:
      /// TODO
  };

  /** @} */
}

//
// Part of the Implementation
//
processor_register_t &x64::InterruptState::stackPointer()
{
  return m_Rsp;
}
processor_register_t x64::InterruptState::stackPointer() const
{
  return m_Rsp;
}
processor_register_t &x64::InterruptState::instructionPointer()
{
  return m_Rip;
}
processor_register_t x64::InterruptState::instructionPointer() const
{
  return m_Rip;
}
size_t x64::InterruptState::getInterruptNumber() const
{
  return m_IntNumber;
}

#endif
