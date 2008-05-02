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
#ifndef KERNEL_PROCESSOR_ARM_COMMON_PROCESSORINFORMATION_H
#define KERNEL_PROCESSOR_ARM_COMMON_PROCESSORINFORMATION_H

#include <processor/types.h>
#include <processor/VirtualAddressSpace.h>

/** @addtogroup kernelprocessorarmcommon
 * @{ */

/** Common arm processor information structure */
class ARMProcessorInformation
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
    /** Construct a ARMProcessorInformation object
     *\param[in] processorId Identifier of the processor
     *\todo the local APIC id */
    inline ARMProcessorInformation(ProcessorId processorId)
      : m_ProcessorId(processorId), m_VirtualAddressSpace(&VirtualAddressSpace::getKernelAddressSpace()){}
    /** The destructor does nothing */
    inline virtual ~ARMProcessorInformation(){}

  private:
    /** Default constructor
     *\note NOT implemented */
    ARMProcessorInformation();
    /** Copy-constructor
     *\note NOT implemented */
    ARMProcessorInformation(const ARMProcessorInformation &);
    /** Assignment operator
     *\note NOT implemented */
    ARMProcessorInformation &operator = (const ARMProcessorInformation &);

    /** Identifier of that processor */
    ProcessorId m_ProcessorId;
    /** The current VirtualAddressSpace */
    VirtualAddressSpace *m_VirtualAddressSpace;
};

/** @} */

#endif
