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
#include "../ppc_common/VsidManager.h"

/// 4K page sizes.
#ifndef PAGE_SIZE
  #define PAGE_SIZE 0x1000
#endif

/** @addtogroup kernelprocessorPPC32
 * @{ */

/**
 * In PPC we have to keep a shadow page table for all address spaces, as 
 * the hashed processor page table is finite in size and thus not all mappings
 * may be able to fit.
 *
 * We use an intel-style two level page table system. These page tables are 
 * never given verbatim to the processor, however, so we are able to take some
 * shortcuts that aren't possible in the x86 architecture.
 *
 * The lower X bytes of virtual address space are reserved for the kernel,
 * and any map(), isMapped(), setFlags(), or unmap() calls get forwarded
 * directly to the kernel address space.
 **/
class PPC32VirtualAddressSpace : public VirtualAddressSpace
{
  /** Processor::switchAddressSpace() needs access to m_PhysicalPageDirectory */
  friend class Processor;
  friend VirtualAddressSpace &VirtualAddressSpace::getKernelAddressSpace();
  /** HashedPageTable needs access to */
  friend class HashedPageTable;
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
  /** The type of a shadow page table - x86 style */
  struct ShadowPageTable
  {
    uint32_t entries[1024];
  };

  /** The constructor for already present paging structures */
  PPC32VirtualAddressSpace();
  /** The copy-constructor
   *\note NOT implemented */
  PPC32VirtualAddressSpace(const PPC32VirtualAddressSpace &);
  /** The copy-constructor
   *\note Not implemented */
  PPC32VirtualAddressSpace &operator = (const PPC32VirtualAddressSpace &);

  /** Initialises the kernel address space, called by Processor. */
  bool initialise();

  /** The kernel virtual address space */
  static PPC32VirtualAddressSpace m_KernelSpace;

  /** The x86-style shadow page directory */
  ShadowPageTable *m_pPageDirectory[1024];
  
  /** The VSIDs - Virtual segment identifiers */
  VsidManager::Vsid m_Vsid;
};

/** @} */

//
// Virtual address space layout
//
#define KERNEL_INITIAL_PAGE_TABLES static_cast<uintptr_t>(0xC0000000)
#define KERNEL_SPACE_START     static_cast<uintptr_t>(0x80000000)
#define USERSPACE_VIRTUAL_HEAP static_cast<uintptr_t>(0x60000000)
#define KERNEL_VIRTUAL_HEAP    static_cast<uintptr_t>(0xD0000000)

#endif
