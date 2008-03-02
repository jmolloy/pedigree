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
#ifndef KERNEL_PROCESSOR_PROCESSOR_H
#define KERNEL_PROCESSOR_PROCESSOR_H

#include <processor/types.h>
#include <processor/state.h>

/** @addtogroup kernelprocessor processor-specific kernel
 * processor-specific kernel interface
 *  @ingroup kernel
 * @{ */

namespace DebugFlags
{
  enum FaultType
  {
    InstructionFetch = 0,
    DataWrite        = 1,
    IOReadWrite      = 2,
    DataReadWrite    = 3
  };
  
  enum Length
  {
    OneByte    = 0,
    TwoBytes   = 1,
    FourBytes  = 3,
    EightBytes = 2
  };
}

/// Defines for debug status flags.
#define DEBUG_BREAKPOINT_0 0x01   /// Breakpoint 0 was triggered.
#define DEBUG_BREAKPOINT_1 0x02   /// Breakpoint 1 was triggered.
#define DEBUG_BREAKPOINT_2 0x04   /// Breakpoint 2 was triggered.
#define DEBUG_BREAKPOINT_3 0x08   /// Breakpoint 3 was triggered.
#define DEBUG_REG_ACCESS   0x2000 /// The next instruction in the stream accesses a debug register, and GD is turned on.
#define DEBUG_SINGLE_STEP  0x4000 /// The exception was caused by single-step execution mode (TF enabled in EFLAGS).
#define DEBUG_TASK_SWITCH  0x8000 /// The exception was caused by a hardware task switch.

class Processor
{
  friend class ProcessorState;

  public:
    /** Get the base-pointer of the calling function
     *\return base-pointer of the calling function */
    static uintptr_t getBasePointer();
    /** Get the stack-pointer of the calling function
     *\return stack-pointer of the calling function */
    static uintptr_t getStackPointer();
    /** Get the instruction-pointer of the calling function
     *\return instruction-pointer of the calling function */
    static uintptr_t getInstructionPointer();
    static uintptr_t getDebugBreakpoint(uint32_t nBpNumber, DebugFlags::FaultType &nFaultType, DebugFlags::Length &nLength, bool &bEnabled);
    static void enableDebugBreakpoint(uint32_t nBpNumber, uintptr_t nLinearAddress, DebugFlags::FaultType nFaultType, DebugFlags::Length nLength);
    static void disableDebugBreakpoint(uint32_t BpNumber);
    static uintptr_t getDebugStatus();
    
    static void setInterrupts(bool bEnable); /// Start maskable interrupts.
    static void setSingleStep(bool bEnable, InterruptState &state); /// Enable single step mode.

    #ifdef X86_COMMON
      /** Read a Machine/Model-specific register
       *\param[in] index the register index
       *\return the value of the register */
      static uint64_t readMachineSpecificRegister(uint32_t index);
      /** Write a Machine/Model-specific register
       *\param[in] index the register index
       *\param[in] value the new value of the register */
      static void writeMachineSpecificRegister(uint32_t index, uint64_t value);
    #endif
};

/** @} */

#endif
