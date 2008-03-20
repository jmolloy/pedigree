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
#include <processor/VirtualAddressSpace.h>

/** @addtogroup kernelprocessor
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
}

/// Defines for debug status flags.
#define DEBUG_BREAKPOINT_0 0x01   /// Breakpoint 0 was triggered.
#define DEBUG_BREAKPOINT_1 0x02   /// Breakpoint 1 was triggered.
#define DEBUG_BREAKPOINT_2 0x04   /// Breakpoint 2 was triggered.
#define DEBUG_BREAKPOINT_3 0x08   /// Breakpoint 3 was triggered.
#define DEBUG_REG_ACCESS   0x2000 /// The next instruction in the stream accesses a debug register, and GD is turned on.
#define DEBUG_SINGLE_STEP  0x4000 /// The exception was caused by single-step execution mode (TF enabled in EFLAGS).
#define DEBUG_TASK_SWITCH  0x8000 /// The exception was caused by a hardware task switch.

/** Interface to the processor's capabilities
 *\note static in member function declarations denotes that these functions return/process
 *      data on the processor that is executing this code. */
class Processor
{
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

    /** Switch to a different virtual address space
     *\param[in] AddressSpace the new address space */
    void switchAddressSpace(const VirtualAddressSpace &AddressSpace);

    /** Trigger a breakpoint */
    inline static void breakpoint() ALWAYS_INLINE;

    /** Return the (total) number of breakpoints
     *\return (total) number of breakpoints */
    static size_t getDebugBreakpointCount();
    /** Get information for a specific breakpoint
     *\param[in] nBpNumber the breakpoint number [0 - (getDebugBreakpointCount() - 1)]
     *\param[in,out] nFaultType the breakpoint type
     *\param[in,out] nLength the breakpoint size/length
     *\param[in,out] bEnabled is the breakpoint enabled? */
    static uintptr_t getDebugBreakpoint(size_t nBpNumber,
                                        DebugFlags::FaultType &nFaultType,
                                        size_t &nLength,
                                        bool &bEnabled);
    /** Enable a specific breakpoint
     *\param[in] nBpNumber the breakpoint number [0 - (getDebugBreakpointCount() - 1)]
     *\param[in] nLinearAddress the linear Adress that should trigger a breakpoint exception
     *\param[in] nFaultType the type of access that should trigger a breakpoint exception
     *\param[in] nLength the size/length of the breakpoint */
    static void enableDebugBreakpoint(size_t nBpNumber,
                                      uintptr_t nLinearAddress,
                                      DebugFlags::FaultType nFaultType,
                                      size_t nLength);
    /** Disable a specific breakpoint
     *\param[in] nBpNumber the breakpoint number [0 - (getDebugBreakpointCount() - 1)] */
    static void disableDebugBreakpoint(size_t nBpNumber);
    /** Get the debug status
     *\todo is the debug status somehow abtractable?
     *\return the debug status */
    static uintptr_t getDebugStatus();

    /** Enable/Disable IRQs
     *\param[in] bEnable true to enable IRSs, false otherwise */
    static void setInterrupts(bool bEnable);
    /** Enable/Disable single-stepping
     *\param[in] bEnable true to enable single-stepping, false otherwise
     *\param[in] state the interrupt-state */
    static void setSingleStep(bool bEnable, InterruptState &state);

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
    #ifdef MIPS_COMMON
      /** Invalidate a line in the instruction cache.
       *\param[in] nAddr The address in KSEG0 or KSEG1 of a memory location to invalidate from the Icache. */
      static void invalidateICache(uintptr_t nAddr);
      /** Invalidate a line in the data cache.
       *\param[in] nAddr The address in KSEG0 or KSEG1 of a memory location to invalidate from the Dcache. */
      static void invalidateDCache(uintptr_t nAddr);
    #endif
};

/** @} */

#ifdef X86_COMMON
  #include <processor/x86_common/Processor.h>
#endif

#endif
