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
#ifndef KERNEL_PROCESSOR_ARM926E_STATE_H
#define KERNEL_PROCESSOR_ARM926E_STATE_H

#include <compiler.h>
#include <processor/types.h>

/** @addtogroup kernelprocessorARM926E
 * @{ */

/** ARM926E Interrupt State */
class ARM926EInterruptState
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

  private:
    /** The default constructor
     *\note NOT implemented */
  public:
    ARM926EInterruptState();

    /** The copy-constructor
     *\note NOT implemented */
    ARM926EInterruptState(const ARM926EInterruptState &);
    /** The assignement operator
     *\note NOT implemented */
    ARM926EInterruptState &operator = (const ARM926EInterruptState &);
    /** The destructor
     *\note NOT implemented */
    ~ARM926EInterruptState() {}
    
    /** ARM926E Registers and State **/
    uint32_t m_r0;
    uint32_t m_r1;
    uint32_t m_r2;
    uint32_t m_r3;
    uint32_t m_r4;
    uint32_t m_r5;
    uint32_t m_r6;
    uint32_t m_r7;
    uint32_t m_r8;
    uint32_t m_r9;
    uint32_t m_r10;
    uint32_t m_Fp;
    uint32_t m_r12;
    uint32_t m_Sp;
    uint32_t m_Lnk;
    uint32_t m_Pc;
    uint32_t m_Cpsr; // holds cpu mode, IRQ and FIQ status, and 4 flags (32-bit)
} PACKED;

typedef ARM926EInterruptState ARM926ESyscallState;
typedef ARM926EInterruptState ARM926EProcessorState;

/** @} */

//
// Part of the Implementation
//

uintptr_t ARM926EInterruptState::getStackPointer() const
{
  return m_Sp;
}
void ARM926EInterruptState::setStackPointer(uintptr_t stackPointer)
{
}
uintptr_t ARM926EInterruptState::getInstructionPointer() const
{
  return m_Pc;
}
void ARM926EInterruptState::setInstructionPointer(uintptr_t instructionPointer)
{
}
uintptr_t ARM926EInterruptState::getBasePointer() const
{
  return m_Fp; // assume frame pointer = base pointer
}
void ARM926EInterruptState::setBasePointer(uintptr_t basePointer)
{
  m_Fp = basePointer; // TODO: some form of casting? Not sure which to use...
}
size_t ARM926EInterruptState::getRegisterSize(size_t index) const
{
#if defined(BITS_32)
  return 4;
#else
  return 4; // TODO: handle other bits sizes (this is mainly here)
            // in order to help future development if ARM ends up
            // requiring 64-bit or something
#endif
}

bool ARM926EInterruptState::kernelMode() const
{
  // TODO: the ARM926E is NOT always in kernel mode, handle this properly
  // This'll require some reading up on the CPSR mode bits
  return true;
}
size_t ARM926EInterruptState::getInterruptNumber() const
{
  // TODO: implement
  return 0;
}

size_t ARM926EInterruptState::getSyscallService() const
{
  // TODO: implement
  return 0;
}
size_t ARM926EInterruptState::getSyscallNumber() const
{
  // TODO: implement
  return 0;
}

#endif
