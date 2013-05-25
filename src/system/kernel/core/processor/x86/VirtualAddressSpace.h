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

#ifndef KERNEL_PROCESSOR_X86_VIRTUALADDRESSSPACE_H
#define KERNEL_PROCESSOR_X86_VIRTUALADDRESSSPACE_H

#include <processor/types.h>
#include <Spinlock.h>
#include <processor/VirtualAddressSpace.h>

//
// Virtual address space layout
//
#define KERNEL_SPACE_START                  reinterpret_cast<void*>(0xC0000000)

#define USERSPACE_DYNAMIC_LINKER_LOCATION   reinterpret_cast<void*>(0x4FA00000)

#define USERSPACE_VIRTUAL_START             reinterpret_cast<void*>(0x400000)
#define USERSPACE_VIRTUAL_HEAP              reinterpret_cast<void*>(0x50000000)
#define USERSPACE_VIRTUAL_STACK             reinterpret_cast<void*>(0xB0000000)
#define USERSPACE_RESERVED_START            USERSPACE_VIRTUAL_HEAP
#define USERSPACE_VIRTUAL_MAX_STACK_SIZE    0x100000
#define USERSPACE_VIRTUAL_LOWEST_STACK      reinterpret_cast<void*>(0x70000000)
#define VIRTUAL_PAGE_DIRECTORY              reinterpret_cast<void*>(0xFFBFF000)
#define VIRTUAL_PAGE_TABLES                 reinterpret_cast<void*>(0xFFC00000)
#define KERNEL_VIRTUAL_TEMP1                reinterpret_cast<void*>(0xFFBFC000)
#define KERNEL_VIRTUAL_TEMP2                reinterpret_cast<void*>(0xFFBFD000)
#define KERNEL_VIRTUAL_TEMP3                reinterpret_cast<void*>(0xFFBFE000)
#define KERNEL_VIRTUAL_HEAP                 reinterpret_cast<void*>(0xC0000000)
#define KERNEL_VIRTUAL_HEAP_SIZE            0x10000000
#define KERNEL_VIRUTAL_PAGE_DIRECTORY       reinterpret_cast<void*>(0xFF7FF000)
#define KERNEL_VIRTUAL_ADDRESS              reinterpret_cast<void*>(0xFF400000 - 0x100000)
#define KERNEL_VIRTUAL_MEMORYREGION_ADDRESS reinterpret_cast<void*>(0xD0000000)
#define KERNEL_VIRTUAL_PAGESTACK_4GB        reinterpret_cast<void*>(0xF0000000)
#define KERNEL_VIRTUAL_STACK                reinterpret_cast<void*>(0xFF3F6000)
#define KERNEL_VIRTUAL_MEMORYREGION_SIZE    0x10000000
#define KERNEL_STACK_SIZE                   0x8000

/** @addtogroup kernelprocessorx86
 * @{ */

/** The X86VirtualAddressSpace implements the VirtualAddressSpace class for the x86 processor
 *  architecture, that means it wraps around the processor's paging functionality. */
class X86VirtualAddressSpace : public VirtualAddressSpace
{
  /** Processor::switchAddressSpace() needs access to m_PhysicalPageDirectory */
  friend class Processor;
  /** Multiprocessor::initialise() needs access to m_PhysicalPageDirectory */
  friend class Multiprocessor;
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
    virtual void *allocateStack();
    virtual void *allocateStack(size_t stackSz);
    virtual void freeStack(void *pStack);

    virtual bool memIsInHeap(void *pMem);
    virtual void *getEndOfHeap();

    virtual VirtualAddressSpace *clone();
    virtual void revertToKernelAddressSpace();

    //
    // Needed for the PhysicalMemoryManager
    //
    /** Map the page table or the page frame if none is currently present
     *\note This should only be used from the PhysicalMemoryManager and you have to
     *      switch to the VirtualAddressSpace you want to change first.
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
    static void initialise() INITIALISATION_ONLY;
    /** The destructor cleans up the address space */
    virtual ~X86VirtualAddressSpace();

    /** Gets start address of the kernel in the address space. */
    virtual uintptr_t getKernelStart() const
    {
        return reinterpret_cast<uintptr_t>(KERNEL_SPACE_START);
    }

    /** Gets start address of the region usable and cloneable for userspace. */
    virtual uintptr_t getUserStart() const
    {
        return reinterpret_cast<uintptr_t>(USERSPACE_VIRTUAL_START);
    }

    /** Gets start address of reserved areas of the userpace address space. */
    virtual uintptr_t getUserReservedStart() const
    {
        return reinterpret_cast<uintptr_t>(USERSPACE_RESERVED_START);
    }

    /** Gets address of the dynamic linker in the address space. */
    virtual uintptr_t getDynamicLinkerAddress() const
    {
        return reinterpret_cast<uintptr_t>(USERSPACE_DYNAMIC_LINKER_LOCATION);
    }

  protected:
    /** The constructor for already present paging structures
     *\param[in] Heap virtual address of the beginning of the heap
     *\param[in] PhysicalPageDirectory physical address of the page directory
     *\param[in] VirtualPageDirectory virtual address of the page directory
     *\param[in] VirtualPageTables virtual address of the page tables
     *\param[in] VirtualStack virtual address of the next stacks
     *\note This constructor is only used to construct the kernel VirtualAddressSpace */
    X86VirtualAddressSpace(void *Heap,
                           physical_uintptr_t PhysicalPageDirectory,
                           void *VirtualPageDirectory,
                           void *VirtualPageTables,
                           void *VirtualStack) INITIALISATION_ONLY;

    bool doIsMapped(void *virtualAddress);
    bool doMap(physical_uintptr_t physicalAddress,
               void *virtualAddress,
               size_t flags);
    void doGetMapping(void *virtualAddress,
                      physical_uintptr_t &physicalAddress,
                      size_t &flags);
    void doSetFlags(void *virtualAddress, size_t newFlags);
    void doUnmap(void *virtualAddress);
    void *doAllocateStack(size_t sSize);

  private:
    /** The default constructor */
    X86VirtualAddressSpace();

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

    /** Begins a "cross-address-space" transaction; maps this address space's
        page directory and a page table in temporarily to the current address
        space, to be used with mapCrossSpace.

        This uses the pages "KERNEL_VIRTUAL_TEMP2" and "KERNEL_VIRTUAL_TEMP3".

        \return A value to pass to mapCrossSpace. This value contains the
                current page table mapped into KERNEL_VIRTUAL_TEMP3. */
    uintptr_t beginCrossSpace(X86VirtualAddressSpace *pOther);
    /** The mapping function for cross-space mappings. beginCrossSpace must be
        called first.

        \param v Value given by "beginCrossSpace". */
    bool mapCrossSpace(uintptr_t &v,
                       physical_uintptr_t physicalAddress,
                       void *virtualAddress,
                       size_t flags);
    /** Called to end a cross-space transaction. */
    void endCrossSpace();
    
    /** Physical address of the page directory */
    physical_uintptr_t m_PhysicalPageDirectory;
    /** Virtual address of the page directory */
    void *m_VirtualPageDirectory;
    /** Virtual address of the page tables */
    void *m_VirtualPageTables;
    /** Current top of the stacks */
    void *m_pStackTop;
    /** List of free stacks */
    Vector<void*> m_freeStacks;
    /** Lock to guard against multiprocessor reentrancy. */
    Spinlock m_Lock;
};

/** The kernel's VirtualAddressSpace on x86 */
class X86KernelVirtualAddressSpace : public X86VirtualAddressSpace
{
  /** X86VirtualAddressSpace needs access to m_Instance */
  friend class X86VirtualAddressSpace;
  /** VirtualAddressSpace::getKernelAddressSpace() needs access to m_Instance */
  friend VirtualAddressSpace &VirtualAddressSpace::getKernelAddressSpace();
  public:
    //
    // VirtualAddressSpace Interface
    //
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

  private:
    /** The constructor */
    X86KernelVirtualAddressSpace();
    /** The destructor */
    ~X86KernelVirtualAddressSpace();
    /** The copy-constructor
     *\note NOT implemented (Singleton) */
    X86KernelVirtualAddressSpace(const X86KernelVirtualAddressSpace &);
    /** The assignment operator
     *\note NOT implemented (Singleton) */
    X86KernelVirtualAddressSpace &operator = (const X86KernelVirtualAddressSpace &);

    /** The kernel virtual address space */
    static X86KernelVirtualAddressSpace m_Instance;
};

/** @} */

#endif
