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

/** @addtogroup kernelprocessorx64
 * @{ */

/** x64 Interrupt State */
class X64InterruptState
{
  friend class X64ProcessorState;
  friend class X64InterruptManager;
  public:
    //
    // General Interface (InterruptState, SyscallState & ProcessorState)
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

    //
    // General Interface (InterruptState & SyscallState)
    //
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

    /** Get the flags register
     *\return the flags register */
    inline uint64_t getFlags() const;
    /** Set the flags register
     *\param[in] flags the new flags */
    inline void setFlags(uint64_t flags);

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

/** x64 Syscall State */
class X64SyscallState
{
  friend class X64ProcessorState;
  public:
    //
    // General Interface (InterruptState, SyscallState & ProcessorState)
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

    //
    // General Interface (InterruptState & SyscallState)
    //
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

/** x64 ProcessorState */
class X64ProcessorState
{
  public:
    /** Default constructor initializes everything with 0 */
    inline X64ProcessorState();
    /** Copy-constructor */
    inline X64ProcessorState(const X64ProcessorState &);
    /** Construct a ProcessorState object from an InterruptState object
     *\param[in] x reference to the InterruptState object */
    inline X64ProcessorState(const X64InterruptState &);
    /** Construct a ProcessorState object from an SyscallState object
     *\param[in] x reference to the SyscallState object */
    inline X64ProcessorState(const X64SyscallState &);
    /** Assignment operator */
    inline X64ProcessorState &operator = (const X64ProcessorState &);
    /** Assignment from InterruptState
     *\param[in] reference to the InterruptState */
    inline X64ProcessorState &operator = (const X64InterruptState &);
    /** Assignment from SyscallState
     *\param[in] reference to the SyscallState */
    inline X64ProcessorState &operator = (const X64SyscallState &);
    /** Destructor does nothing */
    inline ~X64ProcessorState();

    //
    // General Interface (InterruptState, SyscallState & ProcessorState)
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

    /** The R15 general purpose register */
    uint64_t r15;
    /** The R14 general purpose register */
    uint64_t r14;
    /** The R13 general purpose register */
    uint64_t r13;
    /** The R12 general purpose register */
    uint64_t r12;
    /** The R11 general purpose register */
    uint64_t r11;
    /** The R10 general purpose register */
    uint64_t r10;
    /** The R9 general purpose register */
    uint64_t r9;
    /** The R8 general purpose register */
    uint64_t r8;
    /** The base-pointer */
    uint64_t rbp;
    /** The RSI general purpose register */
    uint64_t rsi;
    /** The RDI general purpose register */
    uint64_t rdi;
    /** The RDX general purpose register */
    uint64_t rdx;
    /** The RCX general purpose register */
    uint64_t rcx;
    /** The RBX general purpose register */
    uint64_t rbx;
    /** The RAX general purpose register */
    uint64_t rax;
    /** The instruction-pointer */
    uint64_t rip;
    /** The RFlags register */
    uint64_t rflags;
    /** The stack-pointer */
    uint64_t rsp;
};

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

uint64_t X64InterruptState::getFlags() const
{
  return m_Rflags;
}
void X64InterruptState::setFlags(uint64_t flags)
{
  m_Rflags = flags;
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

X64ProcessorState::X64ProcessorState()
  : r15(), r14(), r13(), r12(), r11(), r10(), r9(), r8(), rbp(), rsi(), rdi(),
    rdx(), rcx(), rbx(), rax(), rip(), rflags(), rsp()
{
}
X64ProcessorState::X64ProcessorState(const X64ProcessorState &x)
  : r15(x.r15), r14(x.r14), r13(x.r13), r12(x.r12), r11(x.r11), r10(x.r10), r9(x.r9), r8(x.r8),
    rbp(x.rbp), rsi(x.rsi), rdi(x.rdi), rdx(x.rdx), rcx(x.rcx), rbx(x.rbx), rax(x.rax), rip(x.rip),
    rflags(x.rflags), rsp(x.rsp)
{
}
X64ProcessorState::X64ProcessorState(const X64InterruptState &x)
  : r15(x.m_R15), r14(x.m_R14), r13(x.m_R13), r12(x.m_R12), r11(x.m_R11), r10(x.m_R10), r9(x.m_R9),
    r8(x.m_R8), rbp(x.m_Rbp), rsi(x.m_Rsi), rdi(x.m_Rdi), rdx(x.m_Rdx), rcx(x.m_Rcx), rbx(x.m_Rbx),
    rax(x.m_Rax), rip(x.m_Rip), rflags(x.m_Rflags), rsp(x.m_Rsp)
{
}
X64ProcessorState::X64ProcessorState(const X64SyscallState &x)
  : r15(x.m_R15), r14(x.m_R14), r13(x.m_R13), r12(x.m_R12), r11(x.m_RFlagsR11), r10(x.m_R10), r9(x.m_R9),
    r8(x.m_R8), rbp(x.m_Rbp), rsi(x.m_Rsi), rdi(x.m_Rdi), rdx(x.m_Rdx), rcx(x.m_RipRcx), rbx(x.m_Rbx),
    rax(x.m_Rax), rip(x.m_RipRcx), rflags(x.m_RFlagsR11), rsp(x.m_Rsp)
{
}
X64ProcessorState &X64ProcessorState::operator = (const X64ProcessorState &x)
{
  r15 = x.r15;
  r14 = x.r14;
  r13 = x.r13;
  r12 = x.r12;
  r11 = x.r11;
  r10 = x.r10;
  r9 = x.r9;
  r8 = x.r8;
  rbp = x.rbp;
  rsi = x.rsi;
  rdi = x.rdi;
  rdx = x.rdx;
  rcx = x.rcx;
  rbx = x.rbx;
  rax = x.rax;
  rip = x.rip;
  rflags = x.rflags;
  rsp = x.rsp;
  return *this;
}
X64ProcessorState &X64ProcessorState::operator = (const X64InterruptState &x)
{
  r15 = x.m_R15;
  r14 = x.m_R14;
  r13 = x.m_R13;
  r12 = x.m_R12;
  r11 = x.m_R11;
  r10 = x.m_R10;
  r9 = x.m_R9;
  r8 = x.m_R8;
  rbp = x.m_Rbp;
  rsi = x.m_Rsi;
  rdi = x.m_Rdi;
  rdx = x.m_Rdx;
  rcx = x.m_Rcx;
  rbx = x.m_Rbx;
  rax = x.m_Rax;
  rip = x.m_Rip;
  rflags = x.m_Rflags;
  rsp = x.m_Rsp;
  return *this;
}
X64ProcessorState &X64ProcessorState::operator = (const X64SyscallState &x)
{
  r15 = x.m_R15;
  r14 = x.m_R14;
  r13 = x.m_R13;
  r12 = x.m_R12;
  r11 = x.m_RFlagsR11;
  r10 = x.m_R10;
  r9 = x.m_R9;
  r8 = x.m_R8;
  rbp = x.m_Rbp;
  rsi = x.m_Rsi;
  rdi = x.m_Rdi;
  rdx = x.m_Rdx;
  rcx = x.m_RipRcx;
  rbx = x.m_Rbx;
  rax = x.m_Rax;
  rip = x.m_RipRcx;
  rflags = x.m_RFlagsR11;
  rsp = x.m_Rsp;
  return *this;
}
X64ProcessorState::~X64ProcessorState()
{
}

uintptr_t X64ProcessorState::getStackPointer() const
{
  return rsp;
}
void X64ProcessorState::setStackPointer(uintptr_t stackPointer)
{
  rsp = stackPointer;
}
uintptr_t X64ProcessorState::getInstructionPointer() const
{
  return rip;
}
void X64ProcessorState::setInstructionPointer(uintptr_t instructionPointer)
{
  rip = instructionPointer;
}
uintptr_t X64ProcessorState::getBasePointer() const
{
  return rbp;
}
void X64ProcessorState::setBasePointer(uintptr_t basePointer)
{
  rbp = basePointer;
}

#endif
