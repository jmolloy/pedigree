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
#ifndef KERNEL_PROCESSOR_MEMORYREGION_H
#define KERNEL_PROCESSOR_MEMORYREGION_H

#include <processor/types.h>

/** @addtogroup kernelprocessor
 * @{ */

/** A MemoryRegion is a special memory entity that is mapped continuously in the
 *  virtual address space, but might not be continuous in the physical address
 *  space. These entities are allocated and freed via the PhysicalMemoryManager.
 *  A MemoryRegion is mapped into the kernel's virtual address space and as such
 *  accessible from kernel-mode within every process's virtual address space.
 *\brief Special memory entity in the kernel's virtual address space */
class MemoryRegion
{
  /** PhysicalMemoryManager needs access to MemoryRegion's members */
  friend class PhysicalMemoryManager;
  public:
    /** Get the address of the beginning of the MemoryRegion in the virtual
     *  address space
     *\return pointer to the beginning of the MemoryRegion */
    inline void *virtualAddress() const
    {
      return m_VirtualAddress;
    }
    /** Get the size of the MemoryRegion
     *\return size of the MemoryRegion in bytes */
    inline size_t size() const
    {
      return m_Size;
    }

  protected:
    /** The default constructor does nothing  */
    inline MemoryRegion()
      : m_VirtualAddress(0),m_PhysicalAddress(0), m_Size(0){}
    /** The destructor does nothing */
    inline virtual ~MemoryRegion(){}

  private:
    /** The copy-constructor
     *\note Not implemented */
    MemoryRegion(const MemoryRegion &);
    /** The copy-constructor
     *\note Not implemented */
    MemoryRegion &operator = (const MemoryRegion &);

    /** Pointer to the beginning of the memory region in the virtual address
     *  space. */
    void *m_VirtualAddress;
    /** Pointer to the beginning of the memory region in the physical address
     * space, if the region is physically continuous, otherwise 0. */
    physical_uintptr_t m_PhysicalAddress;
    /** The size of the memory-region in bytes */
    size_t m_Size;
};

/** @} */

#endif
