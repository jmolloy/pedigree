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
#ifndef KERNEL_PROCESSOR_MIPS32_VIRTUALADDRESSSPACE_H
#define KERNEL_PROCESSOR_MIPS32_VIRTUALADDRESSSPACE_H

#include <processor/types.h>
#include <processor/VirtualAddressSpace.h>
#include <machine/mips_common/types.h>

/// 4K page sizes.
#ifndef PAGE_SIZE
  #define PAGE_SIZE 0x1000
#endif

/// Platform specific flags.
#define MIPS32_PTE_GLOBAL 0x1  // If set, the TLB ignores the current ASID.
#define MIPS32_PTE_VALID  0x2  // If unset, the entry is invalid.
#define MIPS32_PTE_DIRTY  0x4  // Actually, write-enable. 1 to allow writes, 0 for read-only.
// ...Plus 3 bit entry for cache behaviour.

/** @addtogroup kernelprocessorMIPS32
 * @{ */

/** The X86VirtualAddressSpace implements the VirtualAddressSpace class for the mip32
 *  processor, which means it encompasses paging (KUSEG) and KSEG0, KSEG1, KSEG2.
 *
 *  Virtual addressing on MIPS is a little complex. There are two paged areas - KUSEG and KSEG2.
 *  The former is intended for user applications and is located in the lower 2GB of the address space.
 *  The latter is intended for kernel use and is located in the highest 1GB of the address space.
 *  It is normal practice to store the current page table in KSEG2, so that a full 2MB of physical 
 *  RAM doesn't have to be assigned per address space. However, this causes problems, as the MIPS TLB
 *  is rather small and it is possible that an entry for the page table doesn't exist, so a TLB refill
 *  will fault.
 *
 *  In this case, the CPU will double fault and enter our 'normal' exception entry point. The exception handler
 *  must fix up the TLB so that the piece of page table being accessed by the TLB refill handler is present.
 *  The CPU will then return to its original faulting instruction, fault again, but this time the TLB refill
 *  handler will be able to take care of the miss.
 *
 *  We consider the page table as a set of 4KB chunks. It is important to note that because the R4000 is in
 *  essence a 64-bit CPU, the design of the 'standard' page table is that each entry is 64 bits long. This
 *  gives us more bits to play with (in the 32-bit version), but doubles the size of the page table.
 *
 *  So the page table covers 4GB / 4KB = 1M pages, each entry being 2 words in size (64-bits) = 2M words.
 *  
 *  But to map the page table into KSEG2, we split the page table down again into 4KB chunks, which means we
 *  have 2M / 1K (1K being 4KB / 4bytes per word) = 2048 words.
 *  
 *  Here however, we have a problem. VirtualAddressSpace objects are part of Process objects, which are stored
 *  on the kernel heap (which is in KSEG2). So in order to fix up the page table which maps KSEG2, we need to do
 *  a lookup... in KSEG2!
 *
 *  Obviously this wouldn't work, so we set the page table entr(ies) containing this VirtualAddressSpace as a
 *  permanent (or 'wired') TLB entry. That way we can be certain that it will always be present.
 *
 *  One further optimisation is that while the KUSEG mappings will be different through each address space, the 
 *  KSEG2 mappings will all be the same - we want the kernel heap to look the same from every address space!
 *  Thus, we can split our page table 'directory' into two chunks - the KUSEG chunk and the KSEG2 chunk (which 
 *  will be static across all VirtualAddressSpace objects). This has the added advantage that we can get rid of
 *  mappings for the 1GB of KSEG0 and KSEG1 mappings in between KUSEG and KSEG2, as these areas aren't paged.
 *
 *  So that's 1024 words to map KUSEG, and 512 words to map KSEG2.
 */
class MIPS32VirtualAddressSpace : public VirtualAddressSpace
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

  /** Intended to be called solely by MIPS32TlbManager. Returns the physical address of the
   *  chunkIdx'th 4KB chunk of the page table. */
  uintptr_t getPageTableChunk(uintptr_t chunkIdx);

protected:
  /** The destructor does nothing */
  virtual ~MIPS32VirtualAddressSpace();

private:
  /** The constructor for already present paging structures */
  MIPS32VirtualAddressSpace();
  /** The copy-constructor
   *\note NOT implemented */
  MIPS32VirtualAddressSpace(const MIPS32VirtualAddressSpace &);
  /** The copy-constructor
   *\note Not implemented */
  MIPS32VirtualAddressSpace &operator = (const MIPS32VirtualAddressSpace &);

  /** Obtains a new physical frame and generates 'null' entries throughout. */
  uintptr_t generateNullChunk();

  /** Convert the processor independant flags to the processor's representation of the flags
   *\param[in] flags the processor independant flag representation
   *\return the proessor specific flag representation */
  uint32_t toFlags(size_t flags);
  /** Convert processor's representation of the flags to the processor independant representation
   *\param[in] Flags the processor specific flag representation
   *\return the proessor independant flag representation */
  size_t fromFlags(uint32_t Flags);

  /** Our 'directory' - contains the physical address of each page table 'chunk' for KUSEG. */
  uintptr_t m_pKusegDirectory[1024];
  /** Our 'directory' for KSEG2. Shared across all address spaces. */
  static uintptr_t m_pKseg2Directory[512];

  /** The kernel virtual address space */
  static MIPS32VirtualAddressSpace m_KernelSpace;
};

/** @} */

//
// Virtual address space layout
//
#define USERSPACE_VIRTUAL_HEAP static_cast<uintptr_t>(0x10000000)
#define VIRTUAL_PAGE_DIRECTORY static_cast<uintptr_t>(0xC0000000)
#define KERNEL_VIRTUAL_HEAP    static_cast<uintptr_t>(0xC0200000)

#endif
