/*
 * Copyright (c) 2008-2014, Pedigree Developers
 *
 * Please see the CONTRIB file in the root of the source tree for a full
 * list of contributors.
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

#ifndef KERNEL_PROCESSOR_HOSTED_VIRTUALADDRESSSPACE_H
#define KERNEL_PROCESSOR_HOSTED_VIRTUALADDRESSSPACE_H

#include <utilities/Vector.h>
#include <processor/types.h>
#include <processor/VirtualAddressSpace.h>
#include <Spinlock.h>

//
// Virtual address space layout
//
#define KERNEL_SPACE_START                      reinterpret_cast<void*>(0x0000700000000000)

#define USERSPACE_DYNAMIC_LINKER_LOCATION       reinterpret_cast<void*>(0x4FA00000)

#define USERSPACE_VIRTUAL_START                 reinterpret_cast<void*>(0x20000000)
#define USERSPACE_VIRTUAL_HEAP                  reinterpret_cast<void*>(0x50000000)
#define USERSPACE_RESERVED_START                USERSPACE_DYNAMIC_LINKER_LOCATION
#define USERSPACE_VIRTUAL_STACK_SIZE            0x100000
#define USERSPACE_VIRTUAL_MAX_STACK_SIZE        0x100000
#define USERSPACE_DYNAMIC_START                 reinterpret_cast<void*>(0x0000400000000000)
#define USERSPACE_DYNAMIC_END                   reinterpret_cast<void*>(0x0000500000000000)
#define USERSPACE_VIRTUAL_LOWEST_STACK          reinterpret_cast<void*>(USERSPACE_DYNAMIC_END + USERSPACE_VIRTUAL_MAX_STACK_SIZE)
#define USERSPACE_VIRTUAL_STACK                 reinterpret_cast<void*>(0x00006FFFFFFFF000)
#define KERNEL_VIRTUAL_MODULE_BASE              reinterpret_cast<void*>(0x70000000)
#define KERNEL_VIRTUAL_MODULE_SIZE              0x10000000
#define KERNEL_VIRTUAL_EVENT_BASE               reinterpret_cast<void*>(0x300000000000)
#define KERNEL_VIRTUAL_HEAP                     reinterpret_cast<void*>(0x0000700000000000)
#define KERNEL_VIRTUAL_HEAP_SIZE                0x40000000
#define KERNEL_VIRTUAL_ADDRESS                  reinterpret_cast<void*>(0x400000)
#define KERNEL_VIRTUAL_CACHE                    reinterpret_cast<void*>(0x200000000000)
#define KERNEL_VIRTUAL_CACHE_SIZE               0x10000000
#define KERNEL_VIRTUAL_MEMORYREGION_ADDRESS     reinterpret_cast<void*>(0x0000700040000000)
#define KERNEL_VIRTUAL_MEMORYREGION_SIZE        0x40000000
#define KERNEL_VIRTUAL_PAGESTACK_4GB            reinterpret_cast<void*>(0x0000700080000000)
#define KERNEL_VIRTUAL_STACK                    reinterpret_cast<void*>(0x0000700FFFFFF000)
#define KERNEL_STACK_SIZE                       0x10000

/** @addtogroup kernelprocessorhosted
 * @{ */

/** The HostedVirtualAddressSpace implements the VirtualAddressSpace class for the x64 processor
 *  architecture, that means it wraps around the processor's paging functionality. */
class HostedVirtualAddressSpace : public VirtualAddressSpace
{
  /** Processor::switchAddressSpace() needs access to m_PhysicalPML4 */
  friend class Processor;
  /** Multiprocessor::initialise() needs access to m_PhysicalPML4 */
  friend class Multiprocessor;
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
    virtual void *allocateStack();
    virtual void *allocateStack(size_t stackSz);
    virtual void freeStack(void *pStack);

    virtual bool memIsInHeap(void *pMem);
    virtual void *getEndOfHeap();

    virtual VirtualAddressSpace *clone();
    virtual void revertToKernelAddressSpace();

    /** The destructor cleans up the address space */
    virtual ~HostedVirtualAddressSpace();

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

    /** Gets address of the start of the kernel's heap region. */
    virtual uintptr_t getKernelHeapStart() const
    {
        return reinterpret_cast<uintptr_t>(KERNEL_VIRTUAL_HEAP);
    }

    /** Gets address of the end of the kernel's heap region. */
    virtual uintptr_t getKernelHeapEnd() const
    {
        return reinterpret_cast<uintptr_t>(KERNEL_VIRTUAL_HEAP) + KERNEL_VIRTUAL_HEAP_SIZE;
    }

    /** Gets address of the start of dynamic memory mapping area. */
    virtual uintptr_t getDynamicStart() const
    {
        return reinterpret_cast<uintptr_t>(USERSPACE_DYNAMIC_START);
    }

    /** Gets address of the end of dynamic memory mapping area. */
    virtual uintptr_t getDynamicEnd() const
    {
        return reinterpret_cast<uintptr_t>(USERSPACE_DYNAMIC_END);
    }

    /** Gets address of the start of the kernel's cache region. */
    virtual uintptr_t getKernelCacheStart() const
    {
        return reinterpret_cast<uintptr_t>(KERNEL_VIRTUAL_CACHE);
    }

    /** Gets address of the end of the kernel's cache region. */
    virtual uintptr_t getKernelCacheEnd() const
    {
        return reinterpret_cast<uintptr_t>(KERNEL_VIRTUAL_CACHE) + KERNEL_VIRTUAL_CACHE_SIZE;
    }

    /** Gets address of the start of the kernel's event handling block. */
    virtual uintptr_t getKernelEventBlockStart() const
    {
        return reinterpret_cast<uintptr_t>(KERNEL_VIRTUAL_EVENT_BASE);
    }

    /** Gets address of the start of the kernel's module region. */
    virtual uintptr_t getKernelModulesStart() const
    {
        return reinterpret_cast<uintptr_t>(KERNEL_VIRTUAL_MODULE_BASE);
    }

    /** Gets address of the end of the kernel's module region. */
    virtual uintptr_t getKernelModulesEnd() const
    {
        return reinterpret_cast<uintptr_t>(KERNEL_VIRTUAL_MODULE_BASE) + KERNEL_VIRTUAL_MODULE_SIZE;
    }

  private:
    /** The default constructor */
    HostedVirtualAddressSpace();
    /** The constructor for already present paging structures
     *\param[in] Heap virtual address of the beginning of the heap
     *\param[in] PhysicalPageDirectory physical address of the page directory
     *\param[in] VirtualStack virtual address of the top of the next kernel stack
     *\note This constructor is only used to construct the kernel VirtualAddressSpace */
    HostedVirtualAddressSpace(void *Heap, void *VirtualStack);

    /** The copy-constructor
     *\note NOT implemented */
    HostedVirtualAddressSpace(const HostedVirtualAddressSpace &);
    /** The copy-constructor
     *\note Not implemented */
    HostedVirtualAddressSpace &operator = (const HostedVirtualAddressSpace &);

    /** Switch address spaces. */
    static void switchAddressSpace(VirtualAddressSpace &oldSpace, VirtualAddressSpace &newSpace);

    /** Convert the processor independant flags to the processor's representation of the flags
     *\param[in] flags the processor independant flag representation
     *\param[in] bFinal whether this is for the actual page or just an intermediate PTE/PDE
     *\return the proessor specific flag representation */
    uint64_t toFlags(size_t flags, bool bFinal = false);
    /** Convert processor's representation of the flags to the processor independant representation
     *\param[in] Flags the processor specific flag representation
     *\param[in] bFinal whether this is for the actual page or just an intermediate PTE/PDE
     *\return the proessor independant flag representation */
    size_t fromFlags(uint64_t Flags, bool bFinal = false);
    
    /** Allocates a stack with a given size. */
    void *doAllocateStack(size_t sSize);

    typedef struct
    {
      bool active;
      void *vaddr;
      physical_uintptr_t paddr;
      size_t flags; // Real flags, not the mmap-specific ones.
    } mapping_t;

    /** Current top of the stacks */
    void *m_pStackTop;
    /** List of free stacks */
    Vector<void*> m_freeStacks;
    /** Is this the kernel space? */
    bool m_bKernelSpace;
    /** Lock to guard against multiprocessor reentrancy. */
    Spinlock m_Lock;
    /** Tracks the current mappings made in this address space. */
    mapping_t *m_pKnownMaps;
    /** Tracks the size of the known mappings list. */
    size_t m_KnownMapsSize;
    /** Tracks the number of known mappings we have. */
    size_t m_numKnownMaps;
    /** Last unmap for easier finding of places to fit entries. */
    size_t m_nLastUnmap;

    static HostedVirtualAddressSpace m_KernelSpace;
};

/** @} */

#endif
