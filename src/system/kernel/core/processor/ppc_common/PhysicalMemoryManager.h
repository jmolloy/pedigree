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

#ifndef KERNEL_PROCESSOR_PPC32_COMMON_PHYSICALMEMORYMANAGER_H
#define KERNEL_PROCESSOR_PPC32_COMMON_PHYSICALMEMORYMANAGER_H

#include <processor/PhysicalMemoryManager.h>
#include <utilities/RangeList.h>
#include "../ppc32/Translation.h"

/** @addtogroup kernelprocessorppccommon
 * @{ */

/** The common PPC implementation of the PhysicalMemoryManager.
 *
 *  This implementation has two modes - the initial mode and the 'normal' mode.
 *  In the initial mode the PMM will allocate a contiguous set of frames starting
 *  from PMM_INITIAL_START. There is no way to free pages in this mode.
 *
 *  In 'normal' mode the PMM uses a page stack and a rangelist for allocation/
 *  deallocation.
 *
 *  The PMM is in initial mode as soon as the constructor is called - a call to
 *  'initialise' will put it into normal mode.
 *\brief Implementation of the PhysicalMemoryManager for common ppc */
class PpcCommonPhysicalMemoryManager : public PhysicalMemoryManager
{
public:
  /** Get the PpcCommonPhysicalMemoryManager instance
   *\return instance of the PpcCommonPhysicalMemoryManager */
  inline static PpcCommonPhysicalMemoryManager &instance(){return m_Instance;}

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

  void initialise(Translations &translations, uintptr_t ramMax);

  void unmapRegion(MemoryRegion *pRegion);

protected:
  /** The constructor */
  PpcCommonPhysicalMemoryManager();
  /** The destructor */
  virtual ~PpcCommonPhysicalMemoryManager();

private:
  /** The copy-constructor
   *\note Not implemented (singleton) */
  PpcCommonPhysicalMemoryManager(const PpcCommonPhysicalMemoryManager &);
  /** The copy-constructor
   *\note Not implemented (singleton) */
  PpcCommonPhysicalMemoryManager &operator = (const PpcCommonPhysicalMemoryManager &);

  /** The stack of available pages. */
  class PageStack
  {
  public:
    /** Default constructor does nothing */
    PageStack() INITIALISATION_ONLY;
    /** Allocate a page with certain constraints
     *\return The physical address of the allocated page or 0 */
    physical_uintptr_t allocate();
    /** Free a physical page
     *\param[in] physicalAddress physical address of the page */
    void free(uintptr_t physicalAddress);
    /** The destructor does nothing */
    inline ~PageStack(){}

  private:
    /** The copy-constructor
     *\note Not implemented */
    PageStack(const PageStack &);
    /** The copy-constructor
     *\note Not implemented */
    PageStack &operator = (const PageStack &);

    /** Pointer to the base address of the stack. The stack grows upwards. */
    physical_uintptr_t *m_Stack;
    /** Size of the currently mapped stack */
    size_t m_StackMax;
    /** Currently used size of the stack */
    size_t m_StackSize;
  };

  /** The page stack */
  PageStack m_PageStack;

  /** The current operating mode. True for 'initial', false for 'normal'. */
  bool m_InitialMode;  

  /** Variable used in initial mode to keep track of where the next page to allocate is. */
  physical_uintptr_t m_NextPage;

  /** RangeList of free physical memory */
  RangeList<uint64_t> m_PhysicalRanges;

  /** Virtual memory available for MemoryRegions */
  RangeList<uintptr_t> m_MemoryRegions;

  /** The PpcCommonPhysicalMemoryManager class instance */
  static PpcCommonPhysicalMemoryManager m_Instance;
};

//
// Initial physical layout.
//
#define PMM_INITIAL_START 0x2000000

/** @} */

#endif
