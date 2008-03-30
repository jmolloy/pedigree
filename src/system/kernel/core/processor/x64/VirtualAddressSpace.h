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
#ifndef KERNEL_PROCESSOR_X64_VIRTUALADDRESSSPACE_H
#define KERNEL_PROCESSOR_X64_VIRTUALADDRESSSPACE_H

#include <processor/types.h>
#include <processor/VirtualAddressSpace.h>

/** @addtogroup kernelprocessorx64
 * @{ */

/** The X64VirtualAddressSpace implements the VirtualAddressSpace class for the x64 processor
 *  architecture, that means it wraps around the processor's paging functionality. */
class X64VirtualAddressSpace : public VirtualAddressSpace
{
  friend class Processor;
  friend VirtualAddressSpace &VirtualAddressSpace::getKernelAddressSpace();
  public:
    virtual bool map(physical_uintptr_t physAddress,
                     void *virtualAddress,
                     size_t flags);
    virtual bool getMaping(void *virtualAddress,
                           physical_uintptr_t &physAddress,
                           size_t &flags);
    virtual bool setFlags(void *virtualAddress, size_t newFlags);
    virtual bool unmap(void *virtualAddress);

    //
    // Needed for the PhysicalMemoryManager
    //
    bool mapPageStructures(physical_uintptr_t physAddress,
                           void *virtualAddress,
                           size_t flags);

    uint64_t toFlags(size_t flags);
    size_t fromFlags(uint64_t Flags);

  protected:
    /** The destructor does nothing */
    virtual ~X64VirtualAddressSpace();

  private:
    /** The constructor for already present paging structures
     *\param[in] Heap virtual address of the beginning of the heap
     *\param[in] PhysicalPageDirectory physical address of the page directory */
    X64VirtualAddressSpace(void *Heap, physical_uintptr_t PhysicalPML4);

    /** The default constructor 
     *\note NOT implemented */
    X64VirtualAddressSpace();
    /** The copy-constructor
     *\note NOT implemented */
    X64VirtualAddressSpace(const X64VirtualAddressSpace &);
    /** The copy-constructor
     *\note Not implemented */
    X64VirtualAddressSpace &operator = (const X64VirtualAddressSpace &);

    physical_uintptr_t m_PhysicalPML4;

    static X64VirtualAddressSpace m_KernelSpace;
};

/** @} */

#endif
