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

#ifndef KERNEL_PROCESSOR_X64_VIRTUALADDRESSSPACE_H
#define KERNEL_PROCESSOR_X64_VIRTUALADDRESSSPACE_H

#include <processor/types.h>
#include <processor/VirtualAddressSpace.h>

/** @addtogroup kernelprocessorx64
 * @{ */

/** The X64VirtualAddressSpace implements the VirtualAddressSpace class for the x64 processor
 *  architecture, that means it wraps around the processor's paging functionality. */
class X64VirtualAddressSpace : public VirtualAddressSpace
{
  /** Processor::switchAddressSpace() needs access to m_PhysicalPML4 */
  friend class Processor;
  /** VirtualAddressSpace::getKernelAddressSpace() needs access to m_KernelSpace */
  friend VirtualAddressSpace &VirtualAddressSpace::getKernelAddressSpace();
  /** VirtualAddressSpace::create needs access to the constructor */
  friend VirtualAddressSpace *VirtualAddressSpace::create();
  public:
    //
    // VirtualAddressSpace Interface
    //
    virtual bool isAddressValid(void *virtualAddress);
    virtual bool isMapped(void *virtualAddress);

    virtual bool map(physical_uintptr_t physAddress,
                     void *virtualAddress,
                     size_t flags);
    virtual void getMapping(void *virtualAddress,
                            physical_uintptr_t &physAddress,
                            size_t &flags);
    virtual void setFlags(void *virtualAddress, size_t newFlags);
    virtual void unmap(void *virtualAddress);

    //
    // Needed for the PhysicalMemoryManager
    //
    /** Map the page directory pointer table, the page directory, the page table or the page
     *  frame if none is currently present
     *\note This should only be used from the PhysicalMemoryManager
     *\param[in] physAddress the physical page that should be used as page table or page frame
     *\param[in] virtualAddress the virtual address that should be checked for the existance
     *                          of a page directory pointer table, page directory, page table 
     *                          or page frame
     *\param[in] flags the flags used for the mapping
     *\return true, if a page table/frame is already mapped for that address, false if the
     *        physicalAddress has been used as a page directory pointer table, page directory,
     *        page table or page frame. */
    bool mapPageStructures(physical_uintptr_t physAddress,
                           void *virtualAddress,
                           size_t flags);

    /** The destructor cleans up the address space */
    virtual ~X64VirtualAddressSpace();

  private:
    /** The default constructor */
    X64VirtualAddressSpace();
    /** The constructor for already present paging structures
     *\param[in] Heap virtual address of the beginning of the heap
     *\param[in] PhysicalPageDirectory physical address of the page directory
     *\note This constructor is only used to construct the kernel VirtualAddressSpace */
    X64VirtualAddressSpace(void *Heap, physical_uintptr_t PhysicalPML4);

    /** The copy-constructor
     *\note NOT implemented */
    X64VirtualAddressSpace(const X64VirtualAddressSpace &);
    /** The copy-constructor
     *\note Not implemented */
    X64VirtualAddressSpace &operator = (const X64VirtualAddressSpace &);

    /** Get the page table entry, if it exists and check whether a page is mapped or marked as
     *  swapped out.
     *\param[in] virtualAddress the virtual address
     *\param[out] pageTableEntry pointer to the page table entry
     *\return true, if the page table is present and the page mapped or marked swapped out, false
     *        otherwise */
    bool getPageTableEntry(void *virtualAddress,
                           uint64_t *&pageTableEntry);
    /** Convert the processor independant flags to the processor's representation of the flags
     *\param[in] flags the processor independant flag representation
     *\return the proessor specific flag representation */
    uint64_t toFlags(size_t flags);
    /** Convert processor's representation of the flags to the processor independant representation
     *\param[in] Flags the processor specific flag representation
     *\return the proessor independant flag representation */
    size_t fromFlags(uint64_t Flags);
    /** Allocate and map the table entry if none is present
     *\param[in] tableEntry pointer to the current table entry
     *\param[in] flags flags that are used for the mapping
     *\return true, if successfull, false otherwise */
    bool conditionalTableEntryAllocation(uint64_t *tableEntry, uint64_t flags);
    /** map the table entry if none is present
     *\param[in] tableEntry pointer to the current table entry
     *\param[in] physAddress physical address of the page used as the table
     *\param[in] flags flags that are used for the mapping
     *\return true, if the page has been used, false otherwise */
    bool conditionalTableEntryMapping(uint64_t *tableEntry,
                                      uint64_t physAddress,
                                      uint64_t flags);

    /** Physical address of the Page Map Level 4 */
    physical_uintptr_t m_PhysicalPML4;

    /** The kernel virtual address space */
    static X64VirtualAddressSpace m_KernelSpace;
};

/** @} */

//
// Virtual address space layout
//
#define USERSPACE_VIRTUAL_HEAP reinterpret_cast<void*>(0x10000000)
#define KERNEL_VIRTUAL_HEAP reinterpret_cast<void*>(0xFFFFFFFF88000000)
#define KERNEL_VIRTUAL_ADDRESS reinterpret_cast<void*>(0xFFFFFFFF7FF00000)
#define KERNEL_VIRTUAL_MEMORYREGION_ADDRESS reinterpret_cast<void*>(0xFFFFFFFF90000000)
#define KERNEL_VIRTUAL_PAGESTACK_4GB reinterpret_cast<void*>(0xFFFFFFFF7FC00000)
#define KERNEL_VIRTUAL_MEMORYREGION_SIZE 0x50000000

#endif
