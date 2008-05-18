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

#ifndef KERNEL_PROCESSOR_PPC32_VIRTUALADDRESSSPACE_H
#define KERNEL_PROCESSOR_PPC32_VIRTUALADDRESSSPACE_H

#include <processor/types.h>
#include <processor/VirtualAddressSpace.h>
#include <machine/ppc_common/types.h>

/// 4K page sizes.
#ifndef PAGE_SIZE
  #define PAGE_SIZE 0x1000
#endif

/** @addtogroup kernelprocessorPPC32
 * @{ */

class PPC32VirtualAddressSpace : public VirtualAddressSpace
{
  /** Processor::switchAddressSpace() needs access to m_PhysicalPageDirectory */
  friend class Processor;
  friend VirtualAddressSpace &VirtualAddressSpace::getKernelAddressSpace();
public:
  //
  // VirtualAddressSpace Interface
  //
  virtual bool isAddressValid(void *virtualAddress);
  virtual bool isMapped(void *virtualAddress);

  virtual bool map(physical_uintptr_t physicalAddress,
                   void *virtualAddress,
                   size_t flags);
  virtual void getMapping(void *virtualAddress,
                          physical_uintptr_t &physicalAddress,
                          size_t &flags);
  virtual void setFlags(void *virtualAddress, size_t newFlags);
  virtual void unmap(void *virtualAddress);
  virtual void *allocateStack();
  virtual void freeStack(void *pStack);

protected:
  /** The destructor does nothing */
  virtual ~PPC32VirtualAddressSpace();

private:
  /** The constructor for already present paging structures */
  PPC32VirtualAddressSpace();
  /** The copy-constructor
   *\note NOT implemented */
  PPC32VirtualAddressSpace(const PPC32VirtualAddressSpace &);
  /** The copy-constructor
   *\note Not implemented */
  PPC32VirtualAddressSpace &operator = (const PPC32VirtualAddressSpace &);

  /** The kernel virtual address space */
  static PPC32VirtualAddressSpace m_KernelSpace;
};

/** @} */

//
// Virtual address space layout
//
#define USERSPACE_VIRTUAL_HEAP static_cast<uintptr_t>(0x10000000)
#define VIRTUAL_PAGE_DIRECTORY static_cast<uintptr_t>(0xC0000000)
#define KERNEL_VIRTUAL_HEAP    static_cast<uintptr_t>(0xC0800000)

#endif
