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

#ifndef KERNEL_PROCESSOR_X86_COMMON_PROCESSORINFORMATION_H
#define KERNEL_PROCESSOR_X86_COMMON_PROCESSORINFORMATION_H

#include <processor/types.h>
#include <processor/VirtualAddressSpace.h>
#if defined(X86)
  #include <processor/x86/tss.h>
#else
  #include <processor/x64/tss.h>
#endif

/** @addtogroup kernelprocessorx86common
 * @{ */

/** Common x86 processor information structure
 *\todo Local APIC Id, pointer to the TSS */
class X86CommonProcessorInformation
{
  friend class Processor;
  friend class Multiprocessor;
  public:
    #if defined(X86)
      typedef X86TaskStateSegment TaskStateSegment;
    #else
      typedef X64TaskStateSegment TaskStateSegment;
    #endif

    /** Get the current processor's VirtualAddressSpace
     *\return reference to the current processor's VirtualAddressSpace */
    inline VirtualAddressSpace &getVirtualAddressSpace() const
      {return *m_VirtualAddressSpace;}
    /** Set the current processor's VirtualAddressSpace
     *\param[in] virtualAddressSpace reference to the new VirtualAddressSpace */
    inline void setVirtualAddressSpace(VirtualAddressSpace &virtualAddressSpace)
      {m_VirtualAddressSpace = &virtualAddressSpace;}

    /** Set the processor's TSS selector
     *\param[in] TssSelector the new TSS selector */
    inline void setTssSelector(uint16_t TssSelector)
      {m_TssSelector = TssSelector;}
    /** Set the processor's TSS
     *\param[in] Tss pointer to the new TSS */
    inline void setTss(void *Tss)
      {m_Tss= reinterpret_cast<TaskStateSegment*>(Tss);}
    /** Get the processor's TSS selector
     *\return the TSS selector of the processor */
    inline uint16_t getTssSelector() const
      {return m_TssSelector;}
    /** Get the processor's TSS
     *\return the Tss of the processor */
    inline void *getTss() const
      {return reinterpret_cast<void*>(m_Tss);}

    inline uintptr_t getKernelStack() const;
    inline void setKernelStack(uintptr_t stack);

  protected:
    /** Construct a X86CommonProcessor object
     *\param[in] processorId Identifier of the processor
     *\todo the local APIC id */
    inline X86CommonProcessorInformation(ProcessorId processorId, uint8_t apicId = 0)
      : m_ProcessorId(processorId), m_TssSelector(0), m_Tss(0),
        m_VirtualAddressSpace(&VirtualAddressSpace::getKernelAddressSpace()), m_LocalApicId(apicId){}
    /** The destructor does nothing */
    inline virtual ~X86CommonProcessorInformation(){}

  private:
    /** Default constructor
     *\note NOT implemented */
    X86CommonProcessorInformation();
    /** Copy-constructor
     *\note NOT implemented */
    X86CommonProcessorInformation(const X86CommonProcessorInformation &);
    /** Assignment operator
     *\note NOT implemented */
    X86CommonProcessorInformation &operator = (const X86CommonProcessorInformation &);

    /** Identifier of that processor */
    ProcessorId m_ProcessorId;
    /** The Task-State-Segment selector of that Processor */
    uint16_t m_TssSelector;
    /** Pointer to this processor's Task-State-Segment */
    TaskStateSegment *m_Tss;
    /** The current VirtualAddressSpace */
    VirtualAddressSpace *m_VirtualAddressSpace;
    /** Local APIC Id */
    uint8_t m_LocalApicId;
};

/** @} */

//
// Part of the implementation
//
uintptr_t X86CommonProcessorInformation::getKernelStack() const
{
  #if defined(X86)
    return m_Tss->esp0;
  #else
    return m_Tss->rsp0;
  #endif
}
void X86CommonProcessorInformation::setKernelStack(uintptr_t stack)
{
  #if defined(X86)
    m_Tss->esp0 = stack;
  #else
    m_Tss->rsp0 = stack;
  #endif
}

#endif
