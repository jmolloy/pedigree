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
#ifndef KERNEL_PROCESSOR_X86_VIRTUALADDRESSSPACE_H
#define KERNEL_PROCESSOR_X86_VIRTUALADDRESSSPACE_H

#include <processor/types.h>
#include <processor/VirtualAddressSpace.h>

/** @addtogroup kernelprocessorx86
 * @{ */

/** The X86VirtualAddressSpace implements the VirtualAddressSpace class for the x86 processor
 *  architecture, that means it wraps around the processor's paging functionality. */
class X86VirtualAddressSpace : public VirtualAddressSpace
{
  friend class Processor;
  friend VirtualAddressSpace &VirtualAddressSpace::getKernelAddressSpace();
  public:
    virtual bool map(physical_uintptr_t physicalAddress,
                     void *virtualAddress,
                     size_t flags);
    virtual bool getMaping(void *virtualAddress,
                           physical_uintptr_t &physicalAddress,
                           size_t &flags);
    virtual bool setFlags(void *virtualAddress, size_t newFlags);
    virtual bool unmap(void *virtualAddress);

    bool getPageTableEntry(void *virtualAddress, uint32_t *&pageTableEntry);
    uint32_t toFlags(size_t flags);
    size_t fromFlags(uint32_t Flags);

    //
    // Needed for the PhysicalMemoryManager
    //
    bool mapPageStructures(physical_uintptr_t physicalAddress,
                           void *virtualAddress,
                           size_t flags);

  protected:
    /** The default constructor does nothing */
    X86VirtualAddressSpace();
    /** The destructor does nothing */
    virtual ~X86VirtualAddressSpace();

  private:
    /** The constructor for already present paging structures
     *\param[in] PhysicalPageDirectory physical address of the page directory
     *\param[in] VirtualPageDirectory virtual address of the page directory
     *\param[in] VirtualPageTables virtual address of the page tables */
    X86VirtualAddressSpace(void *Heap,
                           physical_uintptr_t PhysicalPageDirectory,
                           void *VirtualPageDirectory,
                           void *VirtualPageTables);
    /** The copy-constructor
     *\note NOT implemented */
    X86VirtualAddressSpace(const X86VirtualAddressSpace &);
    /** The copy-constructor
     *\note Not implemented */
    X86VirtualAddressSpace &operator = (const X86VirtualAddressSpace &);

    physical_uintptr_t m_PhysicalPageDirectory;
    void *m_VirtualPageDirectory;
    void *m_VirtualPageTables;

    static X86VirtualAddressSpace m_KernelSpace;
};

/** @} */

#endif
