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
#ifndef KERNEL_PROCESSOR_VIRTUALADDRESSSPACE_H
#define KERNEL_PROCESSOR_VIRTUALADDRESSSPACE_H

#include <processor/types.h>

/** @addtogroup kernelprocessor
 * @{ */

/** The VirtualAddressSpace encapsulates all the functionality of a virtual memory-
 *  management. This includes management of the mapping between physical and virtual
 *  memory, management of allocated physical memory pages and management of free/allocated
 *  virtual memory. */
class VirtualAddressSpace
{
  public:
    /** The page is only accessible from kernel-mode. If this flag is not set the
     *  page is also accessible from user-mode. */
    static const size_t KernelMode    = 0x01;
    /** The page is writeable. If this flag is not set the page is read-only. */
    static const size_t Write         = 0x02;
    /** The page is executable. If this flag is not set the page is not executable. */
    static const size_t Execute       = 0x04;
    /** If this flag is set, the cache strategy is write-through. */
    static const size_t WriteThrough  = 0x08;
    /** If this flag is set, the cache is disabled. */
    static const size_t CacheDisable  = 0x10;

    /** Get the kernel virtual address space
     *\return reference to the kernel virtual address space */
    static VirtualAddressSpace &getKernelAddressSpace();

    /** Expand the heap
     *\param[in] pageCount the number of pages that should be allocated and mapped to the heap end
     *\param[in] flags flags that describe which accesses should be allowed on the page
     *\return pointer to the beginning of the newly allocated/mapped heap, 0 otherwise */
    virtual void *expandHeap(size_t pageCount, size_t flags);

    /** Map a specific physical page at a specific location into the virtual address
     *  space.
     *\param[in] physicalAddress the address of the physical page that should be mapped into
     *                           the virtual address space.
     *\param[in] virtualAddress the virtual address at which the page apears within the virtual
     *                          address space.
     *\param[in] flags flags that describe which accesses should be allowed on the page.
     *\todo I think this function is not supposed to be used from outside (except from the
     *      PhysicalMemoryManager perhaps)
     *\return true, if successfull, false otherwise */
    virtual bool map(physical_uintptr_t physicalAddress,
                     void *virtualAddress,
                     size_t flags) = 0;
    /** Get the physical address and the flags associated with the specific virtual address
     *\param[in] virtualAddress the address in the virtual address space
     *\param[out] flags the flags
     *\param[out] physicalAddress the physical address
     *\return true, if a mapping exists, false otherwise */
    virtual bool getMaping(void *virtualAddress,
                           physical_uintptr_t &physicalAddress,
                           size_t &flags) = 0;
    /** Set the flags of the page at a specific virtual address
     *\param[in] virtualAddress the virtual address
     *\param[in] newFlags the flags */
    virtual bool setFlags(void *virtualAddress, size_t newFlags) = 0;
    /** Remove the page at the specific virtual address from the mapping
     *\param[in] virtualAddress the virtual address
     *\return true, if successfull, false otherwise */
    virtual bool unmap(void *virtualAddress) = 0;

  protected:
    /** The constructor does nothing */
    inline VirtualAddressSpace(void *Heap)
      : m_Heap(Heap), m_HeapEnd(Heap){}
    /** The destructor does nothing */
    inline virtual ~VirtualAddressSpace(){}

  private:
    /** The default constructor */
    VirtualAddressSpace();
    /** The copy-constructor
     *\note NOT implemented */
    VirtualAddressSpace(const VirtualAddressSpace &);
    /** The copy-constructor
     *\note Not implemented */
    VirtualAddressSpace &operator = (const VirtualAddressSpace &);

    /** \todo documentation */
    void rollbackHeapExpansion(void *virtualAddress, size_t pageCount);

    /** Pointer to the beginning of the heap */
    void *m_Heap;
    /** Pointer to the current heap end */
    void *m_HeapEnd;
};

/** @} */

#endif
