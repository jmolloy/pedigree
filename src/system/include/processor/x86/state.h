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

#ifndef KERNEL_PROCESSOR_X86_STATE_H
#define KERNEL_PROCESSOR_X86_STATE_H

#include <compiler.h>
#include <processor/types.h>

/** @addtogroup kernelprocessorx86
 * @{ */

/** x86 Interrupt State */
class X86InterruptState
{
  friend class X86ProcessorState;
  friend class X86InterruptManager;
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

    //
    // SyscallState Interface
    //
    /** Get the syscall service number
     *\return the syscall service number */
    inline size_t getSyscallService() const;
    /** Get the syscall function number
     *\return the syscall function number */
    inline size_t getSyscallNumber() const;


    /** Get the flags register
     *\return the flags register */
    inline uint32_t getFlags() const;
    /** Set the flags register
     *\param[in] flags the new flags */
    inline void setFlags(uint32_t flags);

    /** Construct a dummy interruptstate on the stack given in 'state', which when executed will 
     *  set the processor to 'state'. */
    static X86InterruptState *construct(class X86ProcessorState &state, bool userMode);

  private:
    /** The default constructor
     *\note NOT implemented */
    X86InterruptState();
    /** The copy-constructor
     *\note NOT implemented */
    X86InterruptState(const X86InterruptState &);
    /** The assignement operator
     *\note NOT implemented */
    X86InterruptState &operator = (const X86InterruptState &);
    /** The destructor
     *\note NOT implemented */
    ~X86InterruptState();

    /** The DS segment register (zero-extended to 32bit) */
    uint32_t m_Ds;
    /** The EDI general purpose register */
    uint32_t m_Edi;
    /** The ESI general purpose register */
    uint32_t m_Esi;
    /** The base-pointer register */
    uint32_t m_Ebp;
    /** Reserved/Unused (ESP) */
    uint32_t m_Res;
    /** The EBX general purpose register */
    uint32_t m_Ebx;
    /** The EDX general purpose register */
    uint32_t m_Edx;
    /** The ECX general purpose register */
    uint32_t m_Ecx;
    /** The EAX general purpose register */
    uint32_t m_Eax;
    /** The interrupt number */
    uint32_t m_IntNumber;
    /** The error-code (if any) */
    uint32_t m_Errorcode;
    /** The instruction pointer */
    uint32_t m_Eip;
    /** The CS segment register (zero-extended to 32bit) */
    uint32_t m_Cs;
    /** The extended flags (EFLAGS) */
    uint32_t m_Eflags;
    /** The stack-pointer */
    uint32_t m_Esp;
    /** The SS segment register */
    uint32_t m_Ss;
} PACKED;

/** x86 SyscallState */
typedef X86InterruptState X86SyscallState;

/** x86 ProcessorState */
class X86ProcessorState
{
  public:
    /** Default constructor initializes everything with 0 */
    inline X86ProcessorState();
    /** Copy-constructor */
    inline X86ProcessorState(const X86ProcessorState &x);
    /** Construct a ProcessorState object from an InterruptState object
     *\param[in] x reference to the InterruptState object */
    inline X86ProcessorState(const X86InterruptState &x);
    /** Assignment operator */
    inline X86ProcessorState &operator = (const X86ProcessorState &x);
    /** Assignment from InterruptState
     *\param[in] reference to the InterruptState */
    inline X86ProcessorState &operator = (const X86InterruptState &x);
    /** Destructor does nothing */
    inline ~X86ProcessorState();

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

    /** The EDI general purpose register */
    uint32_t edi;
    /** The ESI general purpose register */
    uint32_t esi;
    /** The base-pointer register */
    uint32_t ebp;
    /** The EBX general purpose register */
    uint32_t ebx;
    /** The EDX general purpose register */
    uint32_t edx;
    /** The ECX general purpose register */
    uint32_t ecx;
    /** The EAX general purpose register */
    uint32_t eax;
    /** The instruction pointer */
    uint32_t eip;
    /** The extended flags (EFLAGS) */
    uint32_t eflags;
    /** The stack-pointer */
    uint32_t esp;
};

/** @} */

//
// Part of the Implementation
//
uintptr_t X86InterruptState::getStackPointer() const
{
  if (kernelMode())
    return (m_Res + 20);
  return m_Esp;
}
void X86InterruptState::setStackPointer(uintptr_t stackPointer)
{
  if (!kernelMode())
    m_Esp = stackPointer;
}
uintptr_t X86InterruptState::getInstructionPointer() const
{
  return m_Eip;
}
void X86InterruptState::setInstructionPointer(uintptr_t instructionPointer)
{
  m_Eip = instructionPointer;
}
uintptr_t X86InterruptState::getBasePointer() const
{
  return m_Ebp;
}
void X86InterruptState::setBasePointer(uintptr_t basePointer)
{
  m_Ebp = basePointer;
}
size_t X86InterruptState::getRegisterSize(size_t index) const
{
  return 4;
}

bool X86InterruptState::kernelMode() const
{
  return (m_Cs == 0x08);
}
size_t X86InterruptState::getInterruptNumber() const
{
  return m_IntNumber;
}

size_t X86InterruptState::getSyscallService() const
{
  return ((m_Eax >> 16) & 0xFFFF);
}
size_t X86InterruptState::getSyscallNumber() const
{
  return (m_Eax & 0xFFFF);
}

uint32_t X86InterruptState::getFlags() const
{
  return m_Eflags;
}
void X86InterruptState::setFlags(uint32_t flags)
{
  m_Eflags = flags;
}

X86ProcessorState::X86ProcessorState()
  : edi(), esi(), ebp(), ebx(), edx(), ecx(), eax(), eip(), eflags(), esp()
{
}
X86ProcessorState::X86ProcessorState(const X86ProcessorState &x)
  : edi(x.edi), esi(x.esi), ebp(x.ebp), ebx(x.ebx), edx(x.edx), ecx(x.ecx), eax(x.eax),
    eip(x.eip), eflags(x.eflags), esp(x.esp)
{
}
X86ProcessorState::X86ProcessorState(const X86InterruptState &x)
  : edi(x.m_Edi), esi(x.m_Esi), ebp(x.m_Ebp), ebx(x.m_Ebx), edx(x.m_Edx), ecx(x.m_Ecx), eax(x.m_Eax),
    eip(x.m_Eip), eflags(x.m_Eflags), esp(x.getStackPointer())
{
}
X86ProcessorState &X86ProcessorState::operator = (const X86ProcessorState &x)
{
  edi = x.edi;
  esi = x.esi;
  ebp = x.ebp;
  ebx = x.ebx;
  edx = x.edx;
  ecx = x.ecx;
  eax = x.eax;
  eip = x.eip;
  eflags = x.eflags;
  esp = x.esp;
  return *this;
}
X86ProcessorState &X86ProcessorState::operator = (const X86InterruptState &x)
{
  edi = x.m_Edi;
  esi = x.m_Esi;
  ebp = x.m_Ebp;
  ebx = x.m_Ebx;
  edx = x.m_Edx;
  ecx = x.m_Ecx;
  eax = x.m_Eax;
  eip = x.m_Eip;
  eflags = x.m_Eflags;
  esp = x.getStackPointer();
  return *this;
}
X86ProcessorState::~X86ProcessorState()
{
}

uintptr_t X86ProcessorState::getStackPointer() const
{
  return esp;
}
void X86ProcessorState::setStackPointer(uintptr_t stackPointer)
{
  esp = stackPointer;
}
uintptr_t X86ProcessorState::getInstructionPointer() const
{
  return eip;
}
void X86ProcessorState::setInstructionPointer(uintptr_t instructionPointer)
{
  eip = instructionPointer;
}
uintptr_t X86ProcessorState::getBasePointer() const
{
  return ebp;
}
void X86ProcessorState::setBasePointer(uintptr_t basePointer)
{
  ebp = basePointer;
}

#endif
