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

#ifndef KERNEL_PROCESSOR_MIPS32_STATE_H
#define KERNEL_PROCESSOR_MIPS32_STATE_H

#include <compiler.h>
#include <processor/types.h>

/** @addtogroup kernelprocessorPPC32
 * @{ */

typedef class PPC32InterruptState PPC32SyscallState;
typedef class PPC32InterruptState PPC32ProcessorState;

/** PPC32 Interrupt State */
class PPC32InterruptState
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
    
    //
    // SyscallState Interface
    //
    /** Get the syscall service number
     *\return the syscall service number */
    inline size_t getSyscallService() const;
    /** Get the syscall function number
     *\return the syscall function number */
    inline size_t getSyscallNumber() const;

    /** Construct a dummy interruptstate on the stack given in 'state', which when executed will 
     *  set the processor to 'state'. */
    static PPC32InterruptState *construct(PPC32ProcessorState &state, bool userMode);

  private:
    /** The default constructor
     *\note NOT implemented */
  public:
    PPC32InterruptState();

    /** The copy-constructor
     *\note NOT implemented */
    PPC32InterruptState(const PPC32InterruptState &);
    /** The assignement operator
     *\note NOT implemented */
    PPC32InterruptState &operator = (const PPC32InterruptState &);
    /** The destructor
     *\note NOT implemented */
    ~PPC32InterruptState() {}

    /** The interrupt/exception number. */
    uint32_t m_IntNumber;
    /** Fixed point exception register */
    uint32_t m_Xer;
    /** Loop counter */
    uint32_t m_Ctr;
    /** Link register */
    uint32_t m_Lr;
    /** Condition register */
    uint32_t m_Cr;
    /** SRR0 - Contains next program counter location */
    uint32_t m_Srr0;
    /** SRR1 - Contains most of the bits from the MSR register */
    uint32_t m_Srr1;
    /** DSISR - interrupt information */
    uint32_t m_Dsisr;
    /** DAR - interrupt information - data address register */
    uint32_t m_Dar;
    /** General purpose register 0 */
    uint32_t m_R0;
    /** General purpose register 1 */
    uint32_t m_R1;
    /** General purpose register 2 */
    uint32_t m_R2;
    /** General purpose register 3 */
    uint32_t m_R3;
    /** General purpose register 4 */
    uint32_t m_R4;
    /** General purpose register 5 */
    uint32_t m_R5;
    /** General purpose register 6 */
    uint32_t m_R6;
    /** General purpose register 7 */
    uint32_t m_R7;
    /** General purpose register 8 */
    uint32_t m_R8;
    /** General purpose register 9 */
    uint32_t m_R9;
    /** General purpose register 10 */
    uint32_t m_R10;
    /** General purpose register 11 */
    uint32_t m_R11;
    /** General purpose register 12 */
    uint32_t m_R12;
    /** General purpose register 13 */
    uint32_t m_R13;
    /** General purpose register 14 */
    uint32_t m_R14;
    /** General purpose register 15 */
    uint32_t m_R15;
    /** General purpose register 16 */
    uint32_t m_R16;
    /** General purpose register 17 */
    uint32_t m_R17;
    /** General purpose register 18 */
    uint32_t m_R18;
    /** General purpose register 19 */
    uint32_t m_R19;
    /** General purpose register 20 */
    uint32_t m_R20;
    /** General purpose register 21 */
    uint32_t m_R21;
    /** General purpose register 22 */
    uint32_t m_R22;
    /** General purpose register 23 */
    uint32_t m_R23;
    /** General purpose register 24 */
    uint32_t m_R24;
    /** General purpose register 25 */
    uint32_t m_R25;
    /** General purpose register 26 */
    uint32_t m_R26;
    /** General purpose register 27 */
    uint32_t m_R27;
    /** General purpose register 28 */
    uint32_t m_R28;
    /** General purpose register 29 */
    uint32_t m_R29;
    /** General purpose register 30 */
    uint32_t m_R30;
    /** General purpose register 31 */
    uint32_t m_R31;
} PACKED;

/** @} */

//
// Part of the Implementation
//

uintptr_t PPC32InterruptState::getStackPointer() const
{
  return m_R1;
}
void PPC32InterruptState::setStackPointer(uintptr_t stackPointer)
{
  m_R1 = stackPointer;
}
uintptr_t PPC32InterruptState::getInstructionPointer() const
{
  return m_Srr0;
}
void PPC32InterruptState::setInstructionPointer(uintptr_t instructionPointer)
{
  m_Srr0 = instructionPointer;
}
uintptr_t PPC32InterruptState::getBasePointer() const
{
  return 0xdeadbaba;
}
void PPC32InterruptState::setBasePointer(uintptr_t basePointer)
{
}
size_t PPC32InterruptState::getRegisterSize(size_t index) const
{
  return 4;
}

bool PPC32InterruptState::kernelMode() const
{
  return m_Srr1 & (1<<15);
}
size_t PPC32InterruptState::getInterruptNumber() const
{
  return m_IntNumber;
}

size_t PPC32InterruptState::getSyscallService() const
{
  return 0;
}
size_t PPC32InterruptState::getSyscallNumber() const
{
  return 0;
}

#endif
