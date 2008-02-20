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

/** @addtogroup kernelprocessorx64 x64
 * x64 processor-specific kernel
 *  @ingroup kernelprocessor
 * @{ */

/** x64 Interrupt State */
class X64InterruptState
{
  public:
    inline uintptr_t &stackPointer();
    inline uintptr_t stackPointer() const;
    inline uintptr_t &instructionPointer();
    inline uintptr_t instructionPointer() const;
    inline uintptr_t &basePointer();
    inline uintptr_t basePointer() const;

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

class X64SyscallState
{
  public:
    /// TODO
    inline uintptr_t &stackPointer();
    inline uintptr_t stackPointer() const;
    inline uintptr_t &instructionPointer();
    inline uintptr_t instructionPointer() const;
    inline uintptr_t &basePointer();
    inline uintptr_t basePointer() const;

    inline size_t getSyscallNumber() const;

  private:
    /// TODO
};

/** @} */

//
// Part of the Implementation
//
uintptr_t &X64InterruptState::stackPointer()
{
  return m_Rsp;
}
uintptr_t X64InterruptState::stackPointer() const
{
  return m_Rsp;
}
uintptr_t &X64InterruptState::instructionPointer()
{
  return m_Rip;
}
uintptr_t X64InterruptState::instructionPointer() const
{
  return m_Rip;
}
uintptr_t &X64InterruptState::basePointer()
{
  return m_Rbp;
}
uintptr_t X64InterruptState::basePointer() const
{
  return m_Rbp;
}

size_t X64InterruptState::getInterruptNumber() const
{
  return m_IntNumber;
}

#endif
