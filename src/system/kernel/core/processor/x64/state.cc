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

#include <processor/state.h>

const char *X64InterruptStateRegisterName[18] =
{
  "rax",
  "rbx",
  "rcx",
  "rdx",
  "rdi",
  "rsi",
  "rbp",
  "r8",
  "r9",
  "r10",
  "r11",
  "r12",
  "r13",
  "r14",
  "r15",
  "rsp",
  "rip",
  "rflags"
};

const char *X64SyscallStateRegisterName[16] =
{
  "rax",
  "rbx",
  "rdx",
  "rdi",
  "rsi",
  "rbp",
  "r8",
  "r9",
  "r10",
  "r12",
  "r13",
  "r14",
  "r15",
  "rsp",
  "rip",
  "rflags"
};

size_t X64InterruptState::getRegisterCount() const
{
  return 18;
}
processor_register_t X64InterruptState::getRegister(size_t index) const
{
  if (index == 0)return m_Rax;
  if (index == 1)return m_Rbx;
  if (index == 2)return m_Rcx;
  if (index == 3)return m_Rdx;
  if (index == 4)return m_Rdi;
  if (index == 5)return m_Rsi;
  if (index == 6)return m_Rbp;
  if (index == 7)return m_R8;
  if (index == 8)return m_R9;
  if (index == 9)return m_R10;
  if (index == 10)return m_R11;
  if (index == 11)return m_R12;
  if (index == 12)return m_R13;
  if (index == 13)return m_R14;
  if (index == 14)return m_R15;
  if (index == 15)return getStackPointer();
  if (index == 16)return m_Rip;
  if (index == 17)return m_Rflags;
  return 0;
}
const char *X64InterruptState::getRegisterName(size_t index) const
{
  return X64InterruptStateRegisterName[index];
}

size_t X64SyscallState::getRegisterCount() const
{
  return 16;
}
processor_register_t X64SyscallState::getRegister(size_t index) const
{
  if (index == 0)return m_Rax;
  if (index == 1)return m_Rbx;
  if (index == 2)return m_Rdx;
  if (index == 3)return m_Rdi;
  if (index == 4)return m_Rsi;
  if (index == 5)return m_Rbp;
  if (index == 6)return m_R8;
  if (index == 7)return m_R9;
  if (index == 8)return m_R10;
  if (index == 9)return m_R12;
  if (index == 10)return m_R13;
  if (index == 11)return m_R14;
  if (index == 12)return m_R15;
  if (index == 13)return m_Rsp;
  if (index == 14)return m_RipRcx;
  if (index == 15)return m_RFlagsR11;
  return 0;
}
const char *X64SyscallState::getRegisterName(size_t index) const
{
  return X64SyscallStateRegisterName[index];
}

X64InterruptState *X64InterruptState::construct(X64ProcessorState &state, bool userMode)
{
  // Obtain the stack pointer.
  uintptr_t *pStack = reinterpret_cast<uintptr_t*> (state.getStackPointer());

  if (userMode)
  {
    *--pStack = (userMode) ? 0x23 : 0x10; // SS
    *--pStack = state.rsp; // RSP
  }
  *--pStack = 0x200; // RFLAGS - IF enabled.
  *--pStack = (userMode) ? 0x1b : 0x08; // CS
  *--pStack = state.rip; // RIP
  *--pStack = 0; // Error code
  *--pStack = 0; // Interrupt number
  *--pStack = state.rax; // RAX
  *--pStack = state.rbx; // RBX
  *--pStack = state.rcx; // RCX
  *--pStack = state.rdx; // RDX
  *--pStack = state.rdi; // RDI
  *--pStack = state.rsi; // RSI
  *--pStack = state.rbp; // RBP
  *--pStack = state.r8;
  *--pStack = state.r9;
  *--pStack = state.r10;
  *--pStack = state.r11;
  *--pStack = state.r12;
  *--pStack = state.r13;
  *--pStack = state.r14;
  *--pStack = state.r15;
  
  X64InterruptState *toRet = reinterpret_cast<X64InterruptState*> (pStack);

  return toRet;
}