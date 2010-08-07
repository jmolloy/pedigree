/*
 * Copyright (c) 2010 Matthew Iselin, Heights College
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

#ifndef KERNEL_PROCESSOR_ARMV7_VIRTUALADDRESSSPACE_H
#define KERNEL_PROCESSOR_ARMV7_VIRTUALADDRESSSPACE_H

#include <processor/types.h>
#include <processor/VirtualAddressSpace.h>

/** @addtogroup kernelprocessorArmV7
 * @{ */

/** The ArmV7VirtualAddressSpace implements the VirtualAddressSpace class for the ARMv7
  * architecture.
  */
class ArmV7VirtualAddressSpace : public VirtualAddressSpace
{
  /** Processor::switchAddressSpace() needs access to m_PhysicalPageDirectory */
  friend class Processor;
  /** VirtualAddressSpace::create needs access to the constructor */
  friend VirtualAddressSpace *VirtualAddressSpace::create();
  friend class ArmV7KernelVirtualAddressSpace;
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
    

    /** Clone this VirtualAddressSpace. That means that we copy-on-write-map the application
     *  image. */
    virtual VirtualAddressSpace *clone() {return 0;};

    /** Undo a clone() - this happens when an application is Exec()'d - we destroy all mappings
        not in the kernel address space so the space is 'clean'.*/
    virtual void revertToKernelAddressSpace() {};

  protected:
    /** The destructor does nothing */
    virtual ~ArmV7VirtualAddressSpace();

    /** The constructor for already present paging structures
     *\param[in] Heap virtual address of the beginning of the heap
     *\param[in] PhysicalPageDirectory physical address of the page directory
     *\param[in] VirtualPageDirectory virtual address of the page directory
     *\param[in] VirtualPageTables virtual address of the page tables
     *\param[in] VirtualStack virtual address of the next stacks
     *\note This constructor is only used to construct the kernel VirtualAddressSpace */
    ArmV7VirtualAddressSpace(void *Heap,
                           physical_uintptr_t PhysicalPageDirectory,
                           void *VirtualPageDirectory,
                           void *VirtualPageTables,
                           void *VirtualStack);

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

    /** The constructor for already present paging structures
     *\param[in] Heap virtual address of the beginning of the heap
     *\param[in] PhysicalPageDirectory physical address of the page directory
     *\param[in] VirtualPageDirectory virtual address of the page directory
     *\param[in] VirtualPageTables virtual address of the page tables */
    ArmV7VirtualAddressSpace(void *Heap,
                           physical_uintptr_t PhysicalPageDirectory,
                           void *VirtualPageDirectory,
                           void *VirtualPageTables);

    /** The default constructor
     *\note NOT implemented */
    ArmV7VirtualAddressSpace();
    /** The copy-constructor
     *\note NOT implemented */
    ArmV7VirtualAddressSpace(const ArmV7VirtualAddressSpace &);
    /** The copy-constructor
     *\note Not implemented */
    ArmV7VirtualAddressSpace &operator = (const ArmV7VirtualAddressSpace &);

    /** Initialises the kernel address space, called by Processor. */
    bool initialise();

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
    
    /** Section B3.3 in the ARM Architecture Reference Manual (ARM7) */
    
    /// First level descriptor - roughly equivalent to a page directory entry
    /// on x86
    struct FirstLevelDescriptor
    {
        /// Type field for descriptors
        /// 0 = fault
        /// 1 = page table
        /// 2 = section or supersection
        /// 3 = reserved
        
        union {
            struct {
                uint32_t type : 2;
                uint32_t ignore : 30;
            } PACKED fault;
            struct {
                uint32_t type : 2;
                uint32_t sbz1 : 1;
                uint32_t ns : 1;
                uint32_t sbz2 : 1;
                uint32_t domain : 4;
                uint32_t imp : 1;
                uint32_t baseaddr : 22;
            } PACKED pageTable;
            struct {
                uint32_t type : 2;
                uint32_t b : 1;
                uint32_t c : 1;
                uint32_t xn : 1;
                uint32_t domain : 4; /// extended base address for supersection
                uint32_t imp : 1;
                uint32_t ap1 : 2;
                uint32_t tex : 3;
                uint32_t ap2 : 1;
                uint32_t s : 1;
                uint32_t nG : 1;
                uint32_t sectiontype : 1; /// = 0 for section, 1 for supersection
                uint32_t ns : 1;
                uint32_t base : 12;
            } PACKED section;
            
            uint32_t entry;
        } descriptor;
    } PACKED;

    /// Second level descriptor - roughly equivalent to a page table entry
    /// on x86
    struct SecondLevelDescriptor
    {
        /// Type field for descriptors
        /// 0 = fault
        /// 1 = large page
        /// >2 = small page (NX at bit 0)
        
        union
        {
            struct {
                uint32_t type : 2;
                uint32_t ignore : 30;
            } PACKED fault;
            struct {
                uint32_t type : 2;
                uint32_t b : 1;
                uint32_t c : 1;
                uint32_t ap1 : 2;
                uint32_t sbz : 3;
                uint32_t ap2 : 1;
                uint32_t s : 1;
                uint32_t nG : 1;
                uint32_t tex : 3;
                uint32_t xn : 1;
                uint32_t base : 16;
            } PACKED largepage;
            struct {
                uint32_t type : 2;
                uint32_t b : 1;
                uint32_t c : 1;
                uint32_t ap1 : 2;
                uint32_t sbz : 3;
                uint32_t ap2 : 1;
                uint32_t s : 1;
                uint32_t nG : 1;
                uint32_t base : 20;
            } PACKED smallpage;
            
            uint32_t entry;
        } descriptor;
    } PACKED;

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

/** The kernel's VirtualAddressSpace on ArmV7 */
class ArmV7KernelVirtualAddressSpace : public ArmV7VirtualAddressSpace
{
  /** Give Processor access to m_Instance */
  friend class Processor;
  /** ArmV7VirtualAddressSpace needs access to m_Instance */
  friend class ArmV7VirtualAddressSpace;
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

    virtual bool initialiseKernelAddressSpace();

  private:
    /** The constructor */
    ArmV7KernelVirtualAddressSpace();
    /** The destructor */
    ~ArmV7KernelVirtualAddressSpace();
    /** The copy-constructor
     *\note NOT implemented (Singleton) */
    ArmV7KernelVirtualAddressSpace(const ArmV7KernelVirtualAddressSpace &);
    /** The assignment operator
     *\note NOT implemented (Singleton) */
    ArmV7KernelVirtualAddressSpace &operator = (const ArmV7KernelVirtualAddressSpace &);

    /** The kernel virtual address space */
    static ArmV7KernelVirtualAddressSpace m_Instance;
};

/** @} */

/** 1 GB given to userspace, 3 GB given to the kernel.
  * The 3 GB region also contains all MMIO regions.
  */

#define USERSPACE_VIRTUAL_HEAP                  reinterpret_cast<void*>(0x20000000)
#define USERSPACE_VIRTUAL_STACK                 reinterpret_cast<void*>(0x30000000)
#define USERSPACE_PAGEDIR                       reinterpret_cast<void*>(0x3FFFF000) /// Top two page tables are allocated during initialisation to enable mapping here.
#define USERSPACE_PAGETABLES                    reinterpret_cast<void*>(0x3FE00000)
#define USERSPACE_VIRTUAL_STACK_SIZE            0x100000
#define KERNEL_SPACE_START                      reinterpret_cast<void*>(0x40000000)
#define KERNEL_VIRTUAL_HEAP                     reinterpret_cast<void*>(0x40000000)
#define KERNEL_VIRTUAL_HEAP_SIZE                0x40000000
#define KERNEL_VIRTUAL_ADDRESS                  reinterpret_cast<void*>(0x80008000)
#define KERNEL_VIRTUAL_MEMORYREGION_ADDRESS     reinterpret_cast<void*>(0xB0000000)
#define KERNEL_VIRTUAL_PAGESTACK_4GB            reinterpret_cast<void*>(0xF0000000)
#define KERNEL_VIRTUAL_STACK                    reinterpret_cast<void*>(0xFEFFF000)
#define KERNEL_TEMP0                            reinterpret_cast<void*>(0xFE000000)
#define KERNEL_TEMP1                            reinterpret_cast<void*>(0xFE001000)
#define KERNEL_PAGEDIR                          reinterpret_cast<void*>(0xFFFB0000) // 0xFFFF0000 is where we'll put the exception base vectors
#define KERNEL_PAGETABLES                       reinterpret_cast<void*>(0xFF000000)
#define KERNEL_VIRTUAL_MEMORYREGION_SIZE        0x30000000
#define KERNEL_STACK_SIZE                       0x20000

#endif
