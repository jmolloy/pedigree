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
#include <KernelElf.h>
#include <utilities/utility.h>
#include <processor/VirtualAddressSpace.h>
#include <processor/PhysicalMemoryManager.h>

KernelElf KernelElf::m_Instance;

bool KernelElf::initialise(const BootstrapStruct_t &pBootstrap)
{
  #if defined(X86_COMMON)

    // Calculate the range
    physical_uintptr_t start = pBootstrap.addr;
    physical_uintptr_t end   = pBootstrap.addr + pBootstrap.num * pBootstrap.size;
    for (size_t i = 1; i < pBootstrap.num; i++)
    {
      ElfSectionHeader_t *pSh = reinterpret_cast<ElfSectionHeader_t*>(pBootstrap.addr + i * pBootstrap.size);
  
      if ((pSh->flags & SHF_ALLOC) != SHF_ALLOC)
        if (pSh->addr >= end)
          end = pSh->addr + pSh->size;
    }
  
    // Allocate the range
    // TODO: PhysicalMemoryManager::nonRamMemory?
    PhysicalMemoryManager &physicalMemoryManager = PhysicalMemoryManager::instance();
    if (physicalMemoryManager.allocateRegion(m_AdditionalSections,
                                            (end - start + PhysicalMemoryManager::getPageSize() - 1) / PhysicalMemoryManager::getPageSize(),
                                            PhysicalMemoryManager::continuous | PhysicalMemoryManager::nonRamMemory,
                                            VirtualAddressSpace::KernelMode,
                                            start)
        == false)
    {
      return false;
    }

  #endif

  // Get the string table
  const char *tmpStringTable = reinterpret_cast<const char*>(reinterpret_cast<ElfSectionHeader_t*>(pBootstrap.addr + pBootstrap.shndx * pBootstrap.size)->addr);

  // Search for the symbol/string table and adjust sections
  for (size_t i = 1; i < pBootstrap.num; i++)
  {
    ElfSectionHeader_t *pSh = reinterpret_cast<ElfSectionHeader_t*>(pBootstrap.addr + i * pBootstrap.size);

    #if defined(X86_COMMON)

      // Adjust the section
      if ((pSh->flags & SHF_ALLOC) != SHF_ALLOC)
      {
        pSh->addr = reinterpret_cast<uintptr_t>(m_AdditionalSections.convertPhysicalPointer<void>(pSh->addr));
        pSh->offset = pSh->addr;
      }

    #endif

    // Save the symbol/string table
    const char *pStr = tmpStringTable + pSh->name;
    if (pSh->type == SHT_SYMTAB)
    {
      #if defined(X86_COMMON)
        m_pSymbolTable = m_AdditionalSections.convertPhysicalPointer<ElfSectionHeader_t>(reinterpret_cast<physical_uintptr_t>(pSh));
      #else
        m_pSymbolTable = pSh;
      #endif
    }
    else if (!strcmp(pStr, ".strtab"))
    {
      #if defined(X86_COMMON)
        m_pStringTable = m_AdditionalSections.convertPhysicalPointer<ElfSectionHeader_t>(reinterpret_cast<physical_uintptr_t>(pSh));
      #else
        m_pStringTable = pSh;
      #endif
    }
    else if (!strcmp(pStr, ".debug_frame"))
    {
      #if defined(X86_COMMON)
        m_pDebugTable = m_AdditionalSections.convertPhysicalPointer<ElfSectionHeader_t>(reinterpret_cast<physical_uintptr_t>(pSh));
      #else
        m_pStringTable = pSh;
      #endif
    }
  }

  // Initialise remaining member variables
  // TODO: What about m_pShstrtab
  #if defined(X86_COMMON)
    m_pSectionHeaders = m_AdditionalSections.convertPhysicalPointer<ElfSectionHeader_t>(pBootstrap.addr);
  #else
    m_pSectionHeaders = reinterpret_cast<ElfSectionHeader_t*>(pBootstrap.addr);
  #endif
  m_nSectionHeaders = pBootstrap.num;

  return true;
}

KernelElf::KernelElf()
  #if defined(X86_COMMON)
    : m_AdditionalSections("Kernel ELF Sections")
  #endif
{
}
KernelElf::~KernelElf()
{
}
