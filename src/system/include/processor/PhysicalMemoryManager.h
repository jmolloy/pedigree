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

/** @addtogroup kernelprocessor
 * @{ */

const size_t continuous   = 1 << 0;
const size_t nonRamMemory = 1 << 1;
#ifdef X86_COMMON
  const size_t below1MB   = 1 << 2;
  const size_t below16MB  = 2 << 2;
  const size_t below4GB   = 3 << 2;
  const size_t below64GB  = 4 << 2;
#endif

class MemoryRegionBase
{
};
class MemoryRegion : public MemoryRegionBase
{
};

class PhysicalMemoryManager
{
  public:
    /** Get the PhysicalMemoryManager instance
     *\return instance of the PhysicalMemoryManager */
    static PhysicalMemoryManager &instance();

    inline static size_t getPageSize(){return 4096;}
    virtual physical_uintptr_t allocatePage() = 0;
    virtual void freePage(physical_uintptr_t page) = 0;

    virtual bool allocateRegion(MemoryBase &Region,
                                size_t count,
                                size_t pageConstraints,
                                physical_uintptr_t start = -1) = 0;

    // TODO
    // TODO: provide a template to cast void* to T*
    // virtual void *heapMap(physical_uintptr_t address) = 0;

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
