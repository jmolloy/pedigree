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
#ifndef KERNEL_PROCESSOR_PHYSICALMEMORYMANAGER_H
#define KERNEL_PROCESSOR_PHYSICALMEMORYMANAGER_H

#include <compiler.h>
#include <processor/types.h>
#include <processor/MemoryRegion.h>

/** @addtogroup kernelprocessor
 * @{ */

/** The PhysicalMemoryManager manages the physical address space. That means it provides
 *  functions to allocate and free pages. */
class PhysicalMemoryManager
{
  public:
    /** If this flag is set the pages are physically continuous */
    static const size_t continuous   = 1 << 0;
    /** If this flag is set we allocate pages that are not in RAM */
    static const size_t nonRamMemory = 1 << 1;

    // x86/x64 specific flags
    #ifdef X86_COMMON
      /** The allocated pages should be below the 1MB mark */
      static const size_t below1MB   = 1 << 2;
      /** The allocated pages should be below the 16MB mark */
      static const size_t below16MB  = 2 << 2;
      /** The allocated pages should be below the 4GB mark */
      static const size_t below4GB   = 3 << 2;
      /** The allocated pages should be below the 64GB mark */
      static const size_t below64GB  = 4 << 2;
    #endif

    /** Get the PhysicalMemoryManager instance
     *\return instance of the PhysicalMemoryManager */
    static PhysicalMemoryManager &instance();

    /** Get the size of one page
     *\return size of one page in bytes */
    inline static size_t getPageSize(){return PAGE_SIZE;}
    /** Allocate a 'normal' page. Normal means that the page does not need to fullfill any
     *  constraints. These kinds of pages can be used to map normal memory into a virtual
     *  address space.
     *\return physical address of the page or 0 if no page available */
    virtual physical_uintptr_t allocatePage() = 0;
    /** Free a page allocated with the allocatePage() function
     *\param[in] page physical address of the page */
    virtual void freePage(physical_uintptr_t page) = 0;

    /** Allocate a memory-region with specific constraints the pages need to fullfill.
     *\param[in] Region reference to the MemoryRegion object
     *\param[in] count the number of pages to allocate for the MemoryRegion object
     *\param[in] pageConstraints the constraints the pages have to fullfill
     *\param[in] start the physical address of the beginning of the region (optional)
     *\return true, if a valid MemoryRegion object is created, false otherwise */
    virtual bool allocateRegion(MemoryRegion &Region,
                                size_t count,
                                size_t pageConstraints,
                                physical_uintptr_t start = -1) = 0;

  protected:
    /** The constructor */
    inline PhysicalMemoryManager(){}
    /** The destructor */
    inline virtual ~PhysicalMemoryManager(){}

  private:
    /** The copy-constructor
     *\note Not implemented (singleton) */
    PhysicalMemoryManager(const PhysicalMemoryManager &);
    /** The copy-constructor
     *\note Not implemented (singleton) */
    PhysicalMemoryManager &operator = (const PhysicalMemoryManager &);
};

/** @} */

#endif
