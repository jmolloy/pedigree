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

/** @addtogroup kernelprocessorMIPS32
 * @{ */

/** MIPS32 Interrupt State */
class MIPS32InterruptState
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

    //
    // MIPS-specific interface.
    //
    /** Was the exception in a branch delay slot?
     *\return True if the exception occurred in a branch delay slot. */
    inline bool branchDelay() const;
  private:
    /** The default constructor
     *\note NOT implemented */
  public:
    MIPS32InterruptState();

    /** The copy-constructor
     *\note NOT implemented */
    MIPS32InterruptState(const MIPS32InterruptState &);
    /** The assignement operator
     *\note NOT implemented */
    MIPS32InterruptState &operator = (const MIPS32InterruptState &);
    /** The destructor
     *\note NOT implemented */
    ~MIPS32InterruptState() {}
    
    /** Assembler temporary register */
    uint32_t m_At;
    /** Value returned by subroutine 0 */
    uint32_t m_V0;
    /** Value returned by subroutine 1 */
    uint32_t m_V1;
    /** Function argument 0 */
    uint32_t m_A0;
    /** Function argument 1 */
    uint32_t m_A1;
    /** Function argument 2 */
    uint32_t m_A2;
    /** Function argument 3 */
    uint32_t m_A3;
    /** Subroutine temporary 0 */
    uint32_t m_T0;
    /** Subroutine temporary 1 */
    uint32_t m_T1;
    /** Subroutine temporary 2 */
    uint32_t m_T2;
    /** Subroutine temporary 3 */
    uint32_t m_T3;
    /** Subroutine temporary 4 */
    uint32_t m_T4;
    /** Subroutine temporary 5 */
    uint32_t m_T5;
    /** Subroutine temporary 6 */
    uint32_t m_T6;
    /** Subroutine temporary 7 */
    uint32_t m_T7;
    /** Subroutine local 0 */
    uint32_t m_S0;
    /** Subroutine local 1 */
    uint32_t m_S1;
    /** Subroutine local 2 */
    uint32_t m_S2;
    /** Subroutine local 3 */
    uint32_t m_S3;
    /** Subroutine local 4 */
    uint32_t m_S4;
    /** Subroutine local 5 */
    uint32_t m_S5;
    /** Subroutine local 6 */
    uint32_t m_S6;
    /** Subroutine local 7 */
    uint32_t m_S7;
    /** Subroutine temporary 8 */
    uint32_t m_T8;
    /** Subroutine temporary 9 */
    uint32_t m_T9;
    /** Global pointer */
    uint32_t m_Gp;
    /** Stack pointer */
    uint32_t m_Sp;
    /** Frame pointer */
    uint32_t m_Fp;
    /** Subroutine return address */
    uint32_t m_Ra;
    /** Status register */
    uint32_t m_Sr;
    /** Exception cause */
    uint32_t m_Cause;
    /** Exception restart address */
    uint32_t m_Epc;
    /** Bad virtual address */
    uint32_t m_BadVAddr;
    /** TLB miss context */
    uint32_t m_Context;
} PACKED;

typedef MIPS32InterruptState MIPS32SyscallState;
typedef MIPS32InterruptState MIPS32ProcessorState;

/** @} */

//
// Part of the Implementation
//

uintptr_t MIPS32InterruptState::getStackPointer() const
{
  return m_Sp;
}
void MIPS32InterruptState::setStackPointer(uintptr_t stackPointer)
{
}
uintptr_t MIPS32InterruptState::getInstructionPointer() const
{
  return m_Epc;
}
void MIPS32InterruptState::setInstructionPointer(uintptr_t instructionPointer)
{
}
uintptr_t MIPS32InterruptState::getBasePointer() const
{
  return 0xdeadbaba;
}
void MIPS32InterruptState::setBasePointer(uintptr_t basePointer)
{
}
size_t MIPS32InterruptState::getRegisterSize(size_t index) const
{
  return 4;
}

bool MIPS32InterruptState::kernelMode() const
{
  return true;
}
size_t MIPS32InterruptState::getInterruptNumber() const
{
  return (m_Cause>>2)&0x1F; // Return the ExcCode field of the Cause register.
}

size_t MIPS32InterruptState::getSyscallService() const
{
  return 0;
}
size_t MIPS32InterruptState::getSyscallNumber() const
{
  return 0;
}

bool MIPS32InterruptState::branchDelay() const
{
  return (m_Cause&0x80000000); // Test bit 31 of Cause.
}

#endif
