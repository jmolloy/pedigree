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

#ifndef KERNEL_PROCESSOR_HOSTED_PHYSICALMEMORYMANAGER_H
#define KERNEL_PROCESSOR_HOSTED_PHYSICALMEMORYMANAGER_H

#include <compiler.h>
#include <BootstrapInfo.h>
#include <utilities/RangeList.h>
#include <utilities/HashTable.h>
#include <processor/PhysicalMemoryManager.h>
#include <Spinlock.h>

/** @addtogroup kernelprocessorhosted
 * @{ */

#define HOSTED_PHYSICAL_MEMORY_SIZE (1UL << 31)

/** The common x86 implementation of the PhysicalMemoryManager
 *\brief Implementation of the PhysicalMemoryManager for common x86 */
class HostedPhysicalMemoryManager : public PhysicalMemoryManager
{
  friend class CacheManager;
  friend class Cache;
  /** For getBackingFile() */
  friend class HostedVirtualAddressSpace;
  public:
    /** Get the HostedPhysicalMemoryManager instance
     *\return instance of the HostedPhysicalMemoryManager */
    inline static HostedPhysicalMemoryManager &instance()
    {
      return m_Instance;
    }

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

    virtual void pin(physical_uintptr_t page);

    /** Initialise the page stack
     *\param[in] Info reference to the multiboot information structure */
    void initialise(const BootstrapStruct_t &Info) INITIALISATION_ONLY;

    /** Unmap & free the .init section */
    void initialisationDone();

    #if defined(ACPI)
      inline const RangeList<uint64_t> &getAcpiRanges() const
          {return m_AcpiRanges;}
    #endif

  protected:
    /** The constructor */
    HostedPhysicalMemoryManager() INITIALISATION_ONLY;
    /** The destructor */
    virtual ~HostedPhysicalMemoryManager();

  private:
    /** The copy-constructor
     *\note Not implemented (singleton) */
    HostedPhysicalMemoryManager(const HostedPhysicalMemoryManager &);
    /** The copy-constructor
     *\note Not implemented (singleton) */
    HostedPhysicalMemoryManager &operator = (const HostedPhysicalMemoryManager &);

    inline int getBackingFile() const
    {
      return m_BackingFile;
    }

    void unmapRegion(MemoryRegion *pRegion);
    
    /** Same as freePage, but without the lock. Will panic if the lock is unlocked.
      * \note Use in the wrong place and you die. */
    virtual void freePageUnlocked(physical_uintptr_t page);

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
        void free(uint64_t physicalAddress);
        /** The destructor does nothing */
        inline ~PageStack(){}

        inline size_t freePages() const { return m_FreePages; }

      private:
        /** The copy-constructor
         *\note Not implemented */
        PageStack(const PageStack &);
        /** The copy-constructor
         *\note Not implemented */
        PageStack &operator = (const PageStack &);

        /** The number of Stacks */
        static const size_t StackCount = 1;

        /** Pointer to the base address of the stack. The stack grows upwards. */
        void *m_Stack[StackCount];
        /** Size of the currently mapped stack */
        size_t m_StackMax[StackCount];
        /** Currently used size of the stack */
        size_t m_StackSize[StackCount];
        /** Current pages available. */
        size_t m_FreePages;
    };

    /** The page stack */
    PageStack m_PageStack;

    /** RangeList of free physical memory */
    RangeList<uint64_t> m_PhysicalRanges;

    /** Virtual-memory available for MemoryRegions
     *\todo rename this member (conflicts with PhysicalMemoryManager::m_MemoryRegions) */
    RangeList<uintptr_t> m_MemoryRegions;

    /** To guard against multiprocessor reentrancy. */
    Spinlock m_Lock, m_RegionLock;

    /** Utility to wrap a physical address and hash it. */
    class PageHashable {
        public:
            PageHashable() {
                m_Hash = m_Page = 0;
            }

            PageHashable(physical_uintptr_t p) {
                m_Hash = p / getPageSize();
                m_Page = p;
            }

            size_t hash() const {
                return m_Hash;
            }

            bool operator == (const PageHashable &p) const {
                return p.m_Page == m_Page;
            }

        private:
            size_t m_Hash;
            physical_uintptr_t m_Page;
    };

    /** Physical page metadata. */
    struct page {
        size_t refcount;
    };

    /** Page metadata table */
    HashTable<PageHashable, struct page> m_PageMetadata;

    /** Hosted: backing file for physical memory. */
    int m_BackingFile;

    /** Static instance. */
    static HostedPhysicalMemoryManager m_Instance;
};

/** @} */

#endif
