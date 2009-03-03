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

#ifndef KERNEL_PROCESSOR_MEMORYREGION_H
#define KERNEL_PROCESSOR_MEMORYREGION_H

#include <processor/types.h>
#include <processor/PhysicalMemoryManager.h>

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
  /** X86CommonPhysicalMemoryManager needs access to MemoryRegion's members */
  friend class X86CommonPhysicalMemoryManager;
  /** As does PpcCommonPhysicalMemoryManager. */
  friend class PpcCommonPhysicalMemoryManager;
  public:
    /** The default constructor does nothing  */
    inline MemoryRegion(const char *pName)
      : m_VirtualAddress(0),m_PhysicalAddress(0), m_Size(0), m_pName(pName){}
    /** The destructor unregisters itself from the PMM. */
    inline virtual ~MemoryRegion() {PhysicalMemoryManager::instance().unmapRegion(this);}

    /** Get the address of the beginning of the MemoryRegion in the virtual
     *  address space
     *\return pointer to the beginning of the MemoryRegion (in the virtual address space) */
    inline void *virtualAddress() const
    {
      return m_VirtualAddress;
    }
    /** Get the physical address of the beginning of the MemoryRegion
     *\return pointer to the beginning of the MemoryRegion (in the physical address space) */
    inline physical_uintptr_t physicalAddress() const
    {
      return m_PhysicalAddress;
    }
    /** Get the size of the MemoryRegion
     *\return size of the MemoryRegion in bytes */
    inline size_t size() const
    {
      return m_Size;
    }
    /** Get the name of the memory-region
     *\return pointer to the name of the memory-region */
    inline const char *name() const
    {
      return m_pName;
    }

    inline operator bool() const
    {
      return (m_Size != 0);
    }

    inline bool physicalBoundsCheck(physical_uintptr_t address)
    {
      if (address >= m_PhysicalAddress &&
          address < (m_PhysicalAddress + m_Size))
        return true;
      return false;
    }
    template<typename T>
    inline T *convertPhysicalPointer(physical_uintptr_t address)
    {
      return reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(m_VirtualAddress) + (address - m_PhysicalAddress));
    }

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
    /** User-visible name of the memory-region */
    const char *m_pName;
};

/** @} */

#endif
