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

#include <compiler.h>
#include <processor/types.h>

/** @addtogroup kernelprocessorx64 x64
 * x64 processor-specific kernel
 *  @ingroup kernelprocessor
 * @{ */

/** x64 Interrupt State */
class X64InterruptState
{
  public:
    //
    // General Interface (both InterruptState and SyscallState)
    //
    /** Get the stack-pointer before the interrupt occured
     *\return the stack-pointer before the interrupt */
    inline uintptr_t getStackPointer() const;
    /** Set the userspace stack-pointer
     *\param[in] stackPointer the new stack-pointer */
    inline void setStackPointer(uintptr_t stackPointer);
    /** Get the instruction-pointer of the next instruction that is executed
     * after the interrupt is processed
     *\return the instruction-pointer */
    inline uintptr_t getInstructionPointer() const;
    /** Set the instruction-pointer
     *\param[in] instructionPointer the new instruction-pointer */
    inline void setInstructionPointer(uintptr_t instructionPointer);
    /** Get the base-pointer
     *\return the base-pointer */
    inline uintptr_t getBasePointer() const;
    /** Set the base-pointer
     *\param[in] basePointer the new base-pointer */
    inline void setBasePointer(uintptr_t basePointer);
    /** Get the number of registers
     *\return the number of registers */
    size_t getRegisterCount() const;
    /** Get a specific register
     *\param[in] index the index of the register (from 0 to getRegisterCount() - 1)
     *\return the value of the register */
    processor_register_t getRegister(size_t index) const;
    /** Get the name of a specific register
     *\param[in] index the index of the register (from 0 to getRegisterCount() - 1)
     *\return the name of the register */
    const char *getRegisterName(size_t index) const;
    /** Get the register's size in bytes
     *\param[in] index the index of the register (from 0 to getRegisterCount() - 1)
     *\return the register size in bytes */
    inline size_t getRegisterSize(size_t index) const;

    //
    // InterruptState Interface
    //
    /** Did the interrupt happen in kernel-mode?
     *\return true, if the interrupt happened in kernel-mode, false otherwise */
    inline bool kernelMode() const;
    /** Get the interrupt number
     *\return the interrupt number */
    inline size_t getInterruptNumber() const;

  private:
    /** The default constructor
     *\note NOT implemented */
    X64InterruptState();
    /** The copy-constructor
     *\note NOT implemented */
    X64InterruptState(const X64InterruptState &);
    /** The assignement operator
     *\note NOT implemented */
    X64InterruptState &operator = (const X64InterruptState &);
    /** The destructor
     *\note NOT implemented */
    ~X64InterruptState();

    /** The R15 general purpose register */
    uint64_t m_R15;
    /** The R14 general purpose register */
    uint64_t m_R14;
    /** The R13 general purpose register */
    uint64_t m_R13;
    /** The R12 general purpose register */
    uint64_t m_R12;
    /** The R11 general purpose register */
    uint64_t m_R11;
    /** The R10 general purpose register */
    uint64_t m_R10;
    /** The R9 general purpose register */
    uint64_t m_R9;
    /** The R8 general purpose register */
    uint64_t m_R8;
    /** The base-pointer */
    uint64_t m_Rbp;
    /** The RSI general purpose register */
    uint64_t m_Rsi;
    /** The RDI general purpose register */
    uint64_t m_Rdi;
    /** The RDX general purpose register */
    uint64_t m_Rdx;
    /** The RCX general purpose register */
    uint64_t m_Rcx;
    /** The RBX general purpose register */
    uint64_t m_Rbx;
    /** The RAX general purpose register */
    uint64_t m_Rax;
    /** The interrupt number */
    uint64_t m_IntNumber;
    /** The error-code (if any) */
    uint64_t m_Errorcode;
    /** The instruction-pointer */
    uint64_t m_Rip;
    /** The CS segment register */
    uint64_t m_Cs;
    /** The RFlags register */
    uint64_t m_Rflags;
    /** The stack-pointer */
    uint64_t m_Rsp;
    /** The SS segment register */
    uint64_t m_Ss;
} PACKED;

class X64SyscallState
{
  public:
    //
    // General Interface (both InterruptState and SyscallState)
    //
    /** Get the stack-pointer before the syscall occured
     *\return the stack-pointer before the syscall */
    inline uintptr_t getStackPointer() const;
    /** Set the userspace stack-pointer
     *\param[in] stackPointer the new stack-pointer */
    inline void setStackPointer(uintptr_t stackPointer);
    /** Get the instruction-pointer of the next instruction that is executed
     * after the syscall is processed
     *\return the instruction-pointer */
    inline uintptr_t getInstructionPointer() const;
    /** Set the instruction-pointer
     *\param[in] instructionPointer the new instruction-pointer */
    inline void setInstructionPointer(uintptr_t instructionPointer);
    /** Get the base-pointer
     *\return the base-pointer */
    inline uintptr_t getBasePointer() const;
    /** Set the base-pointer
     *\param[in] basePointer the new base-pointer */
    inline void setBasePointer(uintptr_t basePointer);
    /** Get the number of registers
     *\return the number of registers */
    size_t getRegisterCount() const;
    /** Get a specific register
     *\param[in] index the index of the register (from 0 to getRegisterCount() - 1)
     *\return the value of the register */
    processor_register_t getRegister(size_t index) const;
    /** Get the name of a specific register
     *\param[in] index the index of the register (from 0 to getRegisterCount() - 1)
     *\return the name of the register */
    const char *getRegisterName(size_t index) const;
    /** Get the register's size in bytes
     *\param[in] index the index of the register (from 0 to getRegisterCount() - 1)
     *\return the register size in bytes */
    inline size_t getRegisterSize(size_t index) const;

    //
    // SyscallState Interface
    //
    /** Get the syscall service number
     *\return the syscall service number */
    inline size_t getSyscallService() const;
    /** Get the syscall function number
     *\return the syscall function number */
    inline size_t getSyscallNumber() const;

  private:
    /** The R15 general purpose register */
    uint64_t m_R15;
    /** The R14 general purpose register */
    uint64_t m_R14;
    /** The R14 general purpose register */
    uint64_t m_R13;
    /** The R12 general purpose register */
    uint64_t m_R12;
    /** The R10 general purpose register */
    uint64_t m_R10;
    /** The R9 general purpose register */
    uint64_t m_R9;
    /** The R8 general purpose register */
    uint64_t m_R8;
    /** The base-pointer */
    uint64_t m_Rbp;
    /** The RSI general purpose register */
    uint64_t m_Rsi;
    /** The RDI general purpose register */
    uint64_t m_Rdi;
    /** The RDX general purpose register */
    uint64_t m_Rdx;
    /** The RBX general purpose register */
    uint64_t m_Rbx;
    /** The RAX general purpose register */
    uint64_t m_Rax;
    /** The R11/RFlags register */
    uint64_t m_RFlagsR11;
    /** The RIP/RCX register */
    uint64_t m_RipRcx;
    /** The stack-pointer */
    uint64_t m_Rsp;
} PACKED;

/** @} */

//
// Part of the Implementation
//
uintptr_t X64InterruptState::getStackPointer() const
{
  return m_Rsp;
}
void X64InterruptState::setStackPointer(uintptr_t stackPointer)
{
  m_Rsp = stackPointer;
}
uintptr_t X64InterruptState::getInstructionPointer() const
{
  return m_Rip;
}
void X64InterruptState::setInstructionPointer(uintptr_t instructionPointer)
{
  m_Rip = instructionPointer;
}
uintptr_t X64InterruptState::getBasePointer() const
{
  return m_Rbp;
}
void X64InterruptState::setBasePointer(uintptr_t basePointer)
{
  m_Rbp = basePointer;
}
size_t X64InterruptState::getRegisterSize(size_t index) const
{
  return 8;
}

bool X64InterruptState::kernelMode() const
{
  return (m_Cs == 0x08);
}
size_t X64InterruptState::getInterruptNumber() const
{
  return m_IntNumber;
}

uintptr_t X64SyscallState::getStackPointer() const
{
  return m_Rsp;
}
void X64SyscallState::setStackPointer(uintptr_t stackPointer)
{
  m_Rsp = stackPointer;
}
uintptr_t X64SyscallState::getInstructionPointer() const
{
  return m_RipRcx;
}
void X64SyscallState::setInstructionPointer(uintptr_t instructionPointer)
{
  m_RipRcx = instructionPointer;
}
uintptr_t X64SyscallState::getBasePointer() const
{
  return m_Rbp;
}
void X64SyscallState::setBasePointer(uintptr_t basePointer)
{
  m_Rbp = basePointer;
}
size_t X64SyscallState::getRegisterSize(size_t index) const
{
  return 8;
}

size_t X64SyscallState::getSyscallService() const
{
  // TODO: Is this a wise decision?
  return ((m_Rax >> 32) & 0xFFFFFFFF);
}
size_t X64SyscallState::getSyscallNumber() const
{
  // TODO: Is this a wise decision?
  return (m_Rax & 0xFFFFFFFF);
}

#endif
