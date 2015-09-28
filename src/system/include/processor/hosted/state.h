/*
 * Copyright (c) 2008-2014, Pedigree Developers
 *
 * Please see the CONTRIB file in the root of the source tree for a full
 * list of contributors.
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

#ifndef KERNEL_PROCESSOR_HOSTED_STATE_H
#define KERNEL_PROCESSOR_HOSTED_STATE_H

#include <compiler.h>
#include <processor/types.h>
#include <Log.h>

/** @addtogroup kernelprocessorhosted
 * @{ */

/** x64 Interrupt State */
class HostedInterruptState
{
  friend class HostedProcessorState;
  friend class HostedInterruptManager;
  friend class PageFaultHandler;
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

    /** Construct a dummy interruptstate on the stack given in 'state', which when executed will 
     *  set the processor to 'state'. */
    static HostedInterruptState *construct(class HostedProcessorState &state, bool userMode);

  private:
    /** The default constructor */
    HostedInterruptState();
    /** The copy-constructor
     *\note NOT implemented */
    HostedInterruptState(const HostedInterruptState &);
    /** The assignement operator
     *\note NOT implemented */
    HostedInterruptState &operator = (const HostedInterruptState &);
    /** The destructor */
    ~HostedInterruptState();

    /**
     * State data - generally a pointer to the object asking for the interrupt.
     */
    uint64_t state;
    /** Signal number */
    uint64_t which;
    /** siginfo_t structure */
    uint64_t extra;
    /**
     * Third argument to the sigaction handler (has meaning for certain signals
     * such as SIGSEGV).
     */
    uint64_t meta;

    /** Extra pieces that can be filled before entering debugger. */
    uintptr_t m_basePointer;
    uintptr_t m_instructionPointer;
    uintptr_t m_stackPointer;
} PACKED;

/** x64 Syscall State */
class HostedSyscallState
{
  friend class HostedProcessorState;
  friend class HostedSyscallManager;
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
    /** Get the n'th parameter for this syscall. */
    inline uintptr_t getSyscallParameter(size_t n) const;
    inline void setSyscallReturnValue(uintptr_t val);
    inline void setSyscallErrno(uintptr_t val);

  public:
    uint64_t service;
    uint64_t number;
    uint64_t p1, p2, p3, p4, p5;
    uint64_t error;
    uint64_t error_ptr; // pointer to error
    uint64_t result;
    uint64_t rsp;
    uint64_t _align0; // deterministically pad to 16 bytes
} PACKED;

/** x64 ProcessorState */
class HostedProcessorState
{
  public:
    /** Default constructor initializes everything with 0 */
    inline HostedProcessorState();
    /** Copy-constructor */
    inline HostedProcessorState(const HostedProcessorState &);
    /** Construct a ProcessorState object from an InterruptState object
     *\param[in] x reference to the InterruptState object */
    inline HostedProcessorState(const HostedInterruptState &);
    /** Construct a ProcessorState object from an SyscallState object
     *\param[in] x reference to the SyscallState object */
    inline HostedProcessorState(const HostedSyscallState &);
    /** Assignment operator */
    inline HostedProcessorState &operator = (const HostedProcessorState &);
    /** Assignment from InterruptState
     *\param[in] reference to the InterruptState */
    inline HostedProcessorState &operator = (const HostedInterruptState &);
    /** Assignment from SyscallState
     *\param[in] reference to the SyscallState */
    inline HostedProcessorState &operator = (const HostedSyscallState &);
    /** Destructor does nothing */
    inline ~HostedProcessorState();

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

    uint64_t state;
};

/** x64 SchedulerState */
class HostedSchedulerState
{
public:
    uint64_t state[128];
};

/** @} */

//
// Part of the Implementation
//
uintptr_t HostedInterruptState::getStackPointer() const
{
  return m_stackPointer;
}
void HostedInterruptState::setStackPointer(uintptr_t stackPointer)
{
    m_stackPointer = stackPointer;
}
uintptr_t HostedInterruptState::getInstructionPointer() const
{
  return m_instructionPointer;
}
void HostedInterruptState::setInstructionPointer(uintptr_t instructionPointer)
{
  m_instructionPointer = instructionPointer;
}
uintptr_t HostedInterruptState::getBasePointer() const
{
  return m_basePointer;
}
void HostedInterruptState::setBasePointer(uintptr_t basePointer)
{
  m_basePointer = basePointer;
}
size_t HostedInterruptState::getRegisterSize(size_t index) const
{
  return sizeof(processor_register_t);
}

bool HostedInterruptState::kernelMode() const
{
  return true;
}
size_t HostedInterruptState::getInterruptNumber() const
{
  return which;
}

uint64_t HostedInterruptState::getFlags() const
{
  return 0;
}
void HostedInterruptState::setFlags(uint64_t flags)
{
}

uintptr_t HostedSyscallState::getStackPointer() const
{
  return rsp;
}
void HostedSyscallState::setStackPointer(uintptr_t stackPointer)
{
    rsp = stackPointer;
}
uintptr_t HostedSyscallState::getInstructionPointer() const
{
  return 0;
}
void HostedSyscallState::setInstructionPointer(uintptr_t instructionPointer)
{
}
uintptr_t HostedSyscallState::getBasePointer() const
{
  return 0;
}
void HostedSyscallState::setBasePointer(uintptr_t basePointer)
{
}
size_t HostedSyscallState::getRegisterSize(size_t index) const
{
  return sizeof(processor_register_t);
}

size_t HostedSyscallState::getSyscallService() const
{
  return service;
}
size_t HostedSyscallState::getSyscallNumber() const
{
  return number;
}
uintptr_t HostedSyscallState::getSyscallParameter(size_t n) const
{
    if(n == 0) return p1;
    if(n == 1) return p2;
    if(n == 2) return p3;
    if(n == 3) return p4;
    if(n == 4) return p5;
  return 0;
}
void HostedSyscallState::setSyscallReturnValue(uintptr_t val)
{
    result = val;
}
void HostedSyscallState::setSyscallErrno(uintptr_t val)
{
    error = val;
}


HostedProcessorState::HostedProcessorState()
  : state()
{
}
HostedProcessorState::HostedProcessorState(const HostedProcessorState &x)
  : state(x.state)
{
}
HostedProcessorState::HostedProcessorState(const HostedInterruptState &x)
  : state(x.state)
{
}
HostedProcessorState::HostedProcessorState(const HostedSyscallState &x)
{
}
HostedProcessorState &HostedProcessorState::operator = (const HostedProcessorState &x)
{
  state = x.state;
  return *this;
}
HostedProcessorState &HostedProcessorState::operator = (const HostedInterruptState &x)
{
  state = x.state;
  return *this;
}
HostedProcessorState &HostedProcessorState::operator = (const HostedSyscallState &x)
{
  return *this;
}
HostedProcessorState::~HostedProcessorState()
{
}

uintptr_t HostedProcessorState::getStackPointer() const
{
  return 0;
}
void HostedProcessorState::setStackPointer(uintptr_t stackPointer)
{
}
uintptr_t HostedProcessorState::getInstructionPointer() const
{
  return 0;
}
void HostedProcessorState::setInstructionPointer(uintptr_t instructionPointer)
{
}
uintptr_t HostedProcessorState::getBasePointer() const
{
  return 0;
}
void HostedProcessorState::setBasePointer(uintptr_t basePointer)
{
}

#endif
