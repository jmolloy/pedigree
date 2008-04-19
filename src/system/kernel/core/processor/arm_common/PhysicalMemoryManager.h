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
#ifndef KERNEL_PROCESSOR_ARM_COMMON_PHYSICALMEMORYMANAGER_H
#define KERNEL_PROCESSOR_ARM_COMMON_PHYSICALMEMORYMANAGER_H

#include <processor/PhysicalMemoryManager.h>

/** @addtogroup kernelprocessorarmcommon
 * @{ */

/** The common arm implementation of the PhysicalMemoryManager
 *\brief Implementation of the PhysicalMemoryManager for common arm */
class ArmCommonPhysicalMemoryManager : public PhysicalMemoryManager
{
  public:
    /** Get the ArmCommonPhysicalMemoryManager instance
     *\return instance of the ArmCommonPhysicalMemoryManager */
    inline static ArmCommonPhysicalMemoryManager &instance(){return m_Instance;}

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

  protected:
    /** The constructor */
    ArmCommonPhysicalMemoryManager();
    /** The destructor */
    virtual ~ArmCommonPhysicalMemoryManager();

  private:
    /** The copy-constructor
     *\note Not implemented (singleton) */
    ArmCommonPhysicalMemoryManager(const ArmCommonPhysicalMemoryManager &);
    /** The copy-constructor
     *\note Not implemented (singleton) */
    ArmCommonPhysicalMemoryManager &operator = (const ArmCommonPhysicalMemoryManager &);

    /** The ArmCommonPhysicalMemoryManager class instance */
    static ArmCommonPhysicalMemoryManager m_Instance;
};

/** @} */

#endif
