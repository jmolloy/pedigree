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
} PACKED;

typedef PPC32InterruptState PPC32SyscallState;
typedef PPC32InterruptState PPC32ProcessorState;

/** @} */

//
// Part of the Implementation
//

uintptr_t PPC32InterruptState::getStackPointer() const
{
  return 0;
}
void PPC32InterruptState::setStackPointer(uintptr_t stackPointer)
{
}
uintptr_t PPC32InterruptState::getInstructionPointer() const
{
  return 0;
}
void PPC32InterruptState::setInstructionPointer(uintptr_t instructionPointer)
{
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
  return true;
}
size_t PPC32InterruptState::getInterruptNumber() const
{
  return 0; // Return the ExcCode field of the Cause register.
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
