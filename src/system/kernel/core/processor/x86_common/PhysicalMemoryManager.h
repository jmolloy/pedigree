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
#ifndef KERNEL_PROCESSOR_X86_COMMON_PHYSICALMEMORYMANAGER_H
#define KERNEL_PROCESSOR_X86_COMMON_PHYSICALMEMORYMANAGER_H

#include <BootstrapInfo.h>
#include <utilities/RangeList.h>
#include <processor/PhysicalMemoryManager.h>

/** @addtogroup kernelprocessorx86common
 * @{ */

/** The common x86 implementation of the PhysicalMemoryManager
 *\brief Implementation of the PhysicalMemoryManager for common x86 */
class X86CommonPhysicalMemoryManager : public PhysicalMemoryManager
{
  public:
    /** Get the X86CommonPhysicalMemoryManager instance
     *\return instance of the X86CommonPhysicalMemoryManager */
    inline static X86CommonPhysicalMemoryManager &instance(){return m_Instance;}

    //
    // PhysicalMemoryManager Interface
    //
    virtual physical_uintptr_t allocatePage();
    virtual void freePage(physical_uintptr_t page);
    virtual bool allocateRegion(MemoryRegion &Region,
                                size_t count,
                                size_t pageConstraints,
                                physical_uintptr_t start = -1);

    /** Initialise the page stack
     *\param[in] Info reference to the multiboot information structure */
    void initialise(const BootstrapStruct_t &Info);

  protected:
    /** The constructor */
    X86CommonPhysicalMemoryManager();
    /** The destructor */
    virtual ~X86CommonPhysicalMemoryManager();

  private:
    /** The copy-constructor
     *\note Not implemented (singleton) */
    X86CommonPhysicalMemoryManager(const X86CommonPhysicalMemoryManager &);
    /** The copy-constructor
     *\note Not implemented (singleton) */
    X86CommonPhysicalMemoryManager &operator = (const X86CommonPhysicalMemoryManager &);

    /** The actual page stack contains is a Stack of the pages with the constraints
     *  below4GB and below64GB and those pages without address size constraints.
     *\brief The Stack of pages (below4GB, below64GB, no constraint). */
    class PageStack
    {
      public:
        /** Default constructor does nothing */
        PageStack();
        /** Allocate a page with certain constraints
         *\param[in] constaints either below4GB or below64GB or 0
         *\return The physical address of the allocated page or 0 */
        physical_uintptr_t allocate(size_t constraints);
        /** Free a physical page
         *\param[in] physicalAddress physical address of the page */
        void free(uint64_t physicalAddress);
        /** The destructor does nothing */
        inline ~PageStack(){}

      private:
        /** The copy-constructor
         *\note Not implemented */
        PageStack(const PageStack &);
        /** The copy-constructor
         *\note Not implemented */
        PageStack &operator = (const PageStack &);

        /** The number of Stacks */
        #if defined(X86)
          static const size_t StackCount = 1;
        #elif defined(X64)
          static const size_t StackCount = 3;
        #endif

        /** Pointer to the base address of the stack. The stack grows upwards. */
        void *m_Stack[StackCount];
        /** Size of the currently mapped stack */
        size_t m_StackMax[StackCount];
        /** Currently used size of the stack */
        size_t m_StackSize[StackCount];
    };

    /** The page stack */
    PageStack m_PageStack;

    /** RangeList for the usable memory below 1MB */
    RangeList<uint32_t> m_RangeBelow1MB;
    /** RangeList for the usable memory below 16MB */
    RangeList<uint32_t> m_RangeBelow16MB;

    /** RangeList of free physical memory */
    RangeList<uint64_t> m_PhysicalRanges;

    /** The X86CommonPhysicalMemoryManager class instance */
    static X86CommonPhysicalMemoryManager m_Instance;
};

/** @} */

#endif
