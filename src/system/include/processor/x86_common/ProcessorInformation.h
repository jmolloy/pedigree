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

/** @addtogroup kernelprocessorx86common
 * @{ */

/** Common x86 processor information structure
 *\todo Local APIC Id, pointer to the TSS */
class X86CommonProcessorInformation
{
  friend class Processor;
  public:
    /** Get the current processor's VirtualAddressSpace
     *\return reference to the current processor's VirtualAddressSpace */
    inline VirtualAddressSpace &getVirtualAddressSpace() const
      {return *m_VirtualAddressSpace;}
    /** Set the current processor's VirtualAddressSpace
     *\param[in] virtualAddressSpace reference to the new VirtualAddressSpace */
    inline void setVirtualAddressSpace(VirtualAddressSpace &virtualAddressSpace)
      {m_VirtualAddressSpace = &virtualAddressSpace;}

  protected:
    /** Construct a X86CommonProcessor object
     *\param[in] processorId Identifier of the processor
     *\todo the local APIC id */
    inline X86CommonProcessorInformation(ProcessorId processorId)
      : m_ProcessorId(processorId), m_TSSSelector(0), m_TSS(0), m_VirtualAddressSpace(&VirtualAddressSpace::getKernelAddressSpace()){}
    /** The destructor does nothing */
    inline virtual ~X86CommonProcessorInformation(){}

    /** Set the processor's TSS selector
     *\param[in] TSSSelector the new TSS selector */
    inline void setTSSSelector(uint16_t TSSSelector)
      {m_TSSSelector = TSSSelector;}
    /** Set the processor's TSS
     *\param[in] TSS pointer to the new TSS */
    inline void setTSS(void *TSS)
      {m_TSS = TSS;}
    /** Get the processor's TSS selector
     *\return the TSS selector of the processor */
    inline uint16_t getTSSSelector() const
      {return m_TSSSelector;}
    /** Get the processor's TSS
     *\return the TSS of the processor */
    inline void *getTSS() const
      {return m_TSS;}

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
    uint16_t m_TSSSelector;
    /** Pointer to this processor's Task-State-Segment */
    void *m_TSS;
    /** The current VirtualAddressSpace */
    VirtualAddressSpace *m_VirtualAddressSpace;
};

/** @} */

#endif
