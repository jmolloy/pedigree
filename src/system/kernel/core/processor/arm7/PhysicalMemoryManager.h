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

#ifndef KERNEL_PROCESSOR_ARM7_PHYSICALMEMORYMANAGER_H
#define KERNEL_PROCESSOR_ARM7_PHYSICALMEMORYMANAGER_H

#include <BootstrapInfo.h>
#include <utilities/RangeList.h>
#include <processor/PhysicalMemoryManager.h>
#include <Spinlock.h>

/** @addtogroup kernelprocessorArm7
 * @{ */

/** The common arm implementation of the PhysicalMemoryManager
 *\brief Implementation of the PhysicalMemoryManager for common arm */
class Arm7PhysicalMemoryManager : public PhysicalMemoryManager
{
  public:
    /** Get the Arm7PhysicalMemoryManager instance
     *\return instance of the Arm7PhysicalMemoryManager */
    inline static Arm7PhysicalMemoryManager &instance(){return m_Instance;}

    //
    // PhysicalMemoryManager Interface
    //
    virtual physical_uintptr_t allocatePage();
    virtual void freePage(physical_uintptr_t page);
    virtual bool allocateRegion(MemoryRegion &Region,
                                size_t cPages,
                                size_t pageConstraints,
                                size_t Flags,
                                physical_uintptr_t start = -1);

    void initialise(const BootstrapStruct_t &info);

  protected:
    /** The constructor */
    Arm7PhysicalMemoryManager();
    /** The destructor */
    virtual ~Arm7PhysicalMemoryManager();

  private:
    /** The copy-constructor
     *\note Not implemented (singleton) */
    Arm7PhysicalMemoryManager(const Arm7PhysicalMemoryManager &);
    /** The copy-constructor
     *\note Not implemented (singleton) */
    Arm7PhysicalMemoryManager &operator = (const Arm7PhysicalMemoryManager &);

    /** Same as freePage, but without the lock. Will panic if the lock is unlocked.
      * \note Use in the wrong place and you die. */
    virtual void freePageUnlocked(physical_uintptr_t page);
    
    /** Unmaps a memory region - called ONLY from MemoryRegion's destructor. */
    virtual void unmapRegion(MemoryRegion *pRegion);

    /** The actual page stack contains is a Stack of the pages with the constraints
     *  below4GB and below64GB and those pages without address size constraints.
     *\brief The Stack of pages (below4GB, below64GB, no constraint). */
    class PageStack
    {
      public:
        /** Default constructor does nothing */
        PageStack() INITIALISATION_ONLY;
        /** Allocate a page with certain constraints
         *\param[in] constraints either below4GB or below64GB or 0
         *\return The physical address of the allocated page or 0 */
        physical_uintptr_t allocate(size_t constraints);
        /** Free a physical page
         *\param[in] physicalAddress physical address of the page */
        void free(physical_uintptr_t  physicalAddress);
        /** The destructor does nothing */
        inline ~PageStack(){}

      private:
        /** The copy-constructor
         *\note Not implemented */
        PageStack(const PageStack &);
        /** The copy-constructor
         *\note Not implemented */
        PageStack &operator = (const PageStack &);

        /** Pointer to the base address of the stack. The stack grows upwards. */
#ifdef ARM_BEAGLE
        // physical_uintptr_t m_Stack[0x10000000 / sizeof(physical_uintptr_t)]; // 256 MB, one entry per address
        physical_uintptr_t m_Stack[1024];
#else
        physical_uintptr_t m_Stack[1];
#endif
        /** Size of the currently mapped stack */
        size_t m_StackMax;
        /** Currently used size of the stack */
        size_t m_StackSize;
    };

    /** The page stack */
    PageStack m_PageStack;

    /** RangeList of free physical memory */
    RangeList<uint64_t> m_PhysicalRanges;
    
    /** Virtual-memory available for MemoryRegions
     *\todo rename this member (conflicts with PhysicalMemoryManager::m_MemoryRegions) */
    RangeList<uintptr_t> m_VirtualMemoryRegions;

    /** The Arm7PhysicalMemoryManager class instance */
    static Arm7PhysicalMemoryManager m_Instance;

    /** To guard against multiprocessor reentrancy. */
    Spinlock m_Lock, m_RegionLock;
};

/** @} */

#endif
