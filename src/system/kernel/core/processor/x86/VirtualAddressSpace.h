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
#ifndef KERNEL_PROCESSOR_X86_VIRTUALADDRESSSPACE_H
#define KERNEL_PROCESSOR_X86_VIRTUALADDRESSSPACE_H

#include <processor/types.h>
#include <processor/VirtualAddressSpace.h>

/** @addtogroup kernelprocessorx86
 * @{ */

/** The X86VirtualAddressSpace implements the VirtualAddressSpace class for the x86 processor
 *  architecture, that means it wraps around the processor's paging functionality. */
class X86VirtualAddressSpace : public VirtualAddressSpace
{
  /** Processor::switchAddressSpace() needs access to m_PhysicalPageDirectory */
  friend class Processor;
  /** VirtualAddressSpace::getKernelAddressSpace needs access to m_KernelSpace */
  friend VirtualAddressSpace &VirtualAddressSpace::getKernelAddressSpace();
  /** VirtualAddressSpace::create needs access to the constructor */
  friend VirtualAddressSpace *VirtualAddressSpace::create();
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

    //
    // Needed for the PhysicalMemoryManager
    //
    /** Map the page table or the page frame if none is currently present
     *\note This should only be used from the PhysicalMemoryManager
     *\param[in] physicalAddress the physical page that should be used as page table or
     *                           page frame
     *\param[in] virtualAddress the virtual address that should be checked for the existance
     *                          of a page table and page frame
     *\param[in] flags the flags used for the mapping
     *\return true, if a page table/frame is already mapped for that address, false if the
     *        physicalAddress has been used as a page table or as a page frame. */
    bool mapPageStructures(physical_uintptr_t physicalAddress,
                           void *virtualAddress,
                           size_t flags);

    /** Initialise the static members of VirtualAddressSpace */
    static void initialise();
    /** The destructor cleans up the address space */
    virtual ~X86VirtualAddressSpace();

  private:
    /** The default constructor */
    X86VirtualAddressSpace();
    /** The constructor for already present paging structures
     *\param[in] Heap virtual address of the beginning of the heap
     *\param[in] PhysicalPageDirectory physical address of the page directory
     *\param[in] VirtualPageDirectory virtual address of the page directory
     *\param[in] VirtualPageTables virtual address of the page tables
     *\note This constructor is only used to construct the kernel VirtualAddressSpace */
    X86VirtualAddressSpace(void *Heap,
                           physical_uintptr_t PhysicalPageDirectory,
                           void *VirtualPageDirectory,
                           void *VirtualPageTables);

    /** The copy-constructor
     *\note NOT implemented */
    X86VirtualAddressSpace(const X86VirtualAddressSpace &);
    /** The copy-constructor
     *\note Not implemented */
    X86VirtualAddressSpace &operator = (const X86VirtualAddressSpace &);

    /** Get the page table entry, if it exists and check whether a page is mapped or marked as
     *  swapped out.
     *\param[in] virtualAddress the virtual address
     *\param[out] pageTableEntry pointer to the page table entry
     *\return true, if the page table is present and the page mapped or marked swapped out, false
     *        otherwise */
    bool getPageTableEntry(void *virtualAddress,
                           uint32_t *&pageTableEntry);
    /** Convert the processor independant flags to the processor's representation of the flags
     *\param[in] flags the processor independant flag representation
     *\return the proessor specific flag representation */
    uint32_t toFlags(size_t flags);
    /** Convert processor's representation of the flags to the processor independant representation
     *\param[in] Flags the processor specific flag representation
     *\return the proessor independant flag representation */
    size_t fromFlags(uint32_t Flags);

    /** Physical address of the page directory */
    physical_uintptr_t m_PhysicalPageDirectory;
    /** Virtual address of the page directory */
    void *m_VirtualPageDirectory;
    /** Virtual address of the page tables */
    void *m_VirtualPageTables;

    /** The kernel virtual address space */
    static X86VirtualAddressSpace m_KernelSpace;
};

/** @} */

//
// Virtual address space layout
//
#define USERSPACE_VIRTUAL_HEAP reinterpret_cast<void*>(0x10000000)
#define VIRTUAL_PAGE_DIRECTORY reinterpret_cast<void*>(0xFFBFF000)
#define VIRTUAL_PAGE_TABLES reinterpret_cast<void*>(0xFFC00000)
#define KERNEL_VIRTUAL_HEAP reinterpret_cast<void*>(0xC8000000)
#define KERNEL_VIRUTAL_PAGE_DIRECTORY reinterpret_cast<void*>(0xFF7FF000)
#define KERNEL_VIRTUAL_ADDRESS reinterpret_cast<void*>(0xBFF00000)
#define KERNEL_VIRTUAL_MEMORYREGION_ADDRESS reinterpret_cast<void*>(0xD0000000)
#define KERNEL_VIRTUAL_PAGESTACK_4GB reinterpret_cast<void*>(0xF0000000)
#define KERNEL_VIRTUAL_MEMORYREGION_SIZE 0x20000000

#endif
