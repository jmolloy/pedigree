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

#ifndef KERNEL_PROCESSOR_ARM_COMMON_PROCESSORINFORMATION_H
#define KERNEL_PROCESSOR_ARM_COMMON_PROCESSORINFORMATION_H

#include <process/Thread.h>
#include <processor/types.h>
#include <process/PerProcessorScheduler.h>
#include <processor/VirtualAddressSpace.h>

/** @addtogroup kernelprocessorarmv7
 * @{ */

/** ARMv7 processor information structure */
class ArmCommonProcessorInformation
{
  friend class Processor;
  // friend class Multiprocessor;
  public:

    /** Get the current processor's VirtualAddressSpace
     *\return reference to the current processor's VirtualAddressSpace */
    inline VirtualAddressSpace &getVirtualAddressSpace() const
    {
        if (m_VirtualAddressSpace)
            return *m_VirtualAddressSpace;
        else
            return VirtualAddressSpace::getKernelAddressSpace();
    }
    /** Set the current processor's VirtualAddressSpace
     *\param[in] virtualAddressSpace reference to the new VirtualAddressSpace */
    inline void setVirtualAddressSpace(VirtualAddressSpace &virtualAddressSpace)
      {m_VirtualAddressSpace = &virtualAddressSpace;}

    inline uintptr_t getKernelStack() const;
    inline void setKernelStack(uintptr_t stack);
#ifdef THREADS
    inline Thread *getCurrentThread() const
      {return m_pCurrentThread;}
    inline void setCurrentThread(Thread *pThread)
      {m_pCurrentThread = pThread;}

    inline PerProcessorScheduler &getScheduler()
      {return m_Scheduler;}
#endif

  protected:
    /** Construct a ArmCommonProcessor object
     *\param[in] processorId Identifier of the processor */
    inline ArmCommonProcessorInformation(ProcessorId processorId, uint8_t apicId = 0)
      : m_ProcessorId(processorId), m_VirtualAddressSpace(&VirtualAddressSpace::getKernelAddressSpace())
#ifdef THREADS
      , m_pCurrentThread(0), m_Scheduler()
#endif
    {}
    /** The destructor does nothing */
    inline virtual ~ArmCommonProcessorInformation(){}

  private:
    /** Default constructor
     *\note NOT implemented */
    ArmCommonProcessorInformation();
    /** Copy-constructor
     *\note NOT implemented */
    ArmCommonProcessorInformation(const ArmCommonProcessorInformation &);
    /** Assignment operator
     *\note NOT implemented */
    ArmCommonProcessorInformation &operator = (const ArmCommonProcessorInformation &);

    /** Identifier of that processor */
    ProcessorId m_ProcessorId;
    /** The current VirtualAddressSpace */
    VirtualAddressSpace *m_VirtualAddressSpace;
#ifdef THREADS
    /** The current thread */
    Thread *m_pCurrentThread;
    /** The processor's scheduler. */
    PerProcessorScheduler m_Scheduler;
#endif
};

/** @} */

//
// Part of the implementation
//
uintptr_t ArmCommonProcessorInformation::getKernelStack() const
{
    return 0;
/*
    uintptr_t ret = 0;

    // Switch to IRQ mode
    uint32_t cpsr = 0;
    asm volatile("mrs %0, cpsr" : "=r" (cpsr));
    uint32_t oldMode = cpsr & 0x3F;
    if(oldMode != 0x12)
    {
        cpsr &= ~0x3F;
        cpsr |= 0x12;
        asm volatile("msr cpsr_c, %0" : : "r" (cpsr));
    }

    // Load new stack and all that
    asm volatile("mov %0, sp" : "=r" (ret));

    // Switch back to the previous mode
    if(oldMode != 0x12)
    {
        cpsr &= ~0x3F;
        cpsr |= oldMode;
        asm volatile("msr cpsr_c, %0" : : "r" (cpsr));
    }

    return ret;
*/
}
void ArmCommonProcessorInformation::setKernelStack(uintptr_t stack)
{
/*
    // Handle IRQ save location
    stack -= 0x10;

    // Switch to IRQ mode
    uint32_t cpsr = 0;
    asm volatile("mrs %0, cpsr" : "=r" (cpsr));
    uint32_t oldMode = cpsr & 0x3F;
    if(oldMode == 0x12)
    {
        // Can't switch a stack we're using!
        return;
    }
    cpsr &= ~0x3F;
    cpsr |= 0x12;
    asm volatile("msr cpsr_c, %0" : : "r" (cpsr));

    // Load new stack and all that
    asm volatile("mov sp, %0; mov r13, %1" : : "r" (stack), "r" (stack + 0x10) : "sp", "r13");

    // Switch back to the previous mode
    cpsr &= ~0x3F;
    cpsr |= oldMode;
    asm volatile("msr cpsr_c, %0" : : "r" (cpsr));
*/
}

#endif
