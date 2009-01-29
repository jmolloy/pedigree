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

#include <Log.h>
#include <linker/Elf.h>
#include <utilities/utility.h>
#include <BootstrapInfo.h>
#include <processor/Processor.h>
#include <processor/PhysicalMemoryManager.h>

Elf32::Elf32() :
  m_pSymbolTable(0),
  m_nSymbolTableSize(0),
  m_pStringTable(0),
  m_nStringTableSize(0),
  m_pShstrtab(0),
  m_pGotTable(0),
  m_pRelTable(0),
  m_pRelaTable(0),
  m_nRelTableSize(0),
  m_nRelaTableSize(0),
  m_pPltRelTable(0),
  m_pPltRelaTable(0),
  m_bUsesRela(false),
  m_pDebugTable(0),
  m_nDebugTableSize(0),
  m_pDynamicSymbolTable(0),
  m_nDynamicSymbolTableSize(0),
  m_pDynamicStringTable(0),
  m_pSectionHeaders(0),
  m_nSectionHeaders(0),
  m_pProgramHeaders(0),
  m_nProgramHeaders(0),
  m_nPltSize(0),
  m_nEntry(0)
{
}

Elf32::~Elf32()
{
}

bool Elf32::create(uint8_t *pBuffer, size_t length)
{
  // The main header will be at pBuffer[0].
  Elf32Header_t *pHeader = reinterpret_cast<Elf32Header_t *>(pBuffer);

  // Check the ident.
  if ( (m_pHeader->ident[1] != 'E') ||
       (m_pHeader->ident[2] != 'L') ||
       (m_pHeader->ident[3] != 'F') ||
       (m_pHeader->ident[0] != 127) )
  {
    ERROR("ELF file: ident check failed!");
    return false;
  }

  // Load in the section headers.
  m_nSectionHeaders = pHeader->shnum;
  m_pSectionHeaders = new Elf32SectionHeader_t[pHeader->shnum];
  memcpy (reinterpret_cast<uint8_t*>(m_pSectionHeaders), &pBuffer[pHeader->shoff], pHeader->shnum*sizeof(Elf32SectionHeader_t));

  // Find the section header string table.
  Elf32SectionHeader_t *pShstrtab = &pSectionHeaders[pHeader->shstrndx];

  // Load the section header string table.
  m_pShstrtab = new char[pShstrtab->size];
  memcpy (reinterpret_cast<uint8_t*>(m_pShstrtab), &pBuffer[pShstrtab->offset], pShstrtab->size);

  Elf32SectionHeader_t *pSymbolTable=0, *pStringTable=0;
  // Go through each section header, trying to find .symtab.
  for (int i = 0; i < m_pHeader->shnum; i++)
  {
    const char *pStr = m_pShstrtab + m_pSectionHeaders[i].name;
    if (!strcmp(pStr, ".symtab"))
      pSymbolTable = &m_pSectionHeaders[i];
    if (!strcmp(pStr, ".strtab"))
      pStringTable = &m_pSectionHeaders[i];
  }

  if (pSymbolTable == 0)
  {
    WARNING("ELF: symbol table not found!");
  }
  else
  {
    m_nSymbolTableSize = pSymbolTable->size;
    m_pSymbolTable = new Elf32Symbol_t[m_nSymbolTableSize/sizeof(Elf32Symbol_t)];
    memcpy (reinterpret_cast<uint8_t*>(m_pSymbolTable), &pBuffer[pSymbolTable->offset], pSymbolTable->size);
  }

  if (pStringTable == 0)
  {
    WARNING("ELF: string table not found!");
  }
  else
  {
    m_pStringTable = new char[pStringTable->size];
    memcpy (reinterpret_cast<uint8_t*>(m_pStringTable), &pBuffer[pStringTable->offset], pStringTable->size);
  }

  // Attempt to load in some program headers, if they exist.
  if (pHeader->phnum > 0)
  {
    m_nProgramHeaders = pHeader->phnum;
    m_pProgramHeaders = new Elf32ProgramHeader_t[pHeader->phnum];
    memcpy(reinterpret_cast<uint8_t*>(m_pProgramHeaders), &pBuffer[pHeader->phoff], sizeof(Elf32ProgramHeader_t)*pHeader->phnum);

    // Look for the dynamic program header.
    for (int i = 0; i < m_nProgramHeaders; i++)
    {
      if (m_pProgramHeaders[i].type == ELF32_PT_DYNAMIC)
      {
        Elf32ProgramHeader_t *pDynamic = &m_pProgramHeaders[i];
        Elf32Dyn_t *pDyn = reinterpret_cast<Elf32Dyn_t*> (&pBuffer[pDynamic->offset]);
        size_t nDynamicStringTableSize = 0;
        
        // Cycle through all dynamic entries until the NULL entry.
        while (pDyn->tag != ELF32_DT_NULL)
        {
          switch (pDyn->tag)
          {
            case ELF32_DT_NEEDED:
              m_NeededLibraries.pushBack(reinterpret_cast<char*> (pDyn->un.ptr));
            case ELF32_DT_SYMTAB:
              m_pDynamicSymbolTable = reinterpret_cast<Elf32Symbol_t*> (pDyn->un.ptr);
              break;
            case ELF32_DT_STRTAB:
              m_pDynamicStringTable = reinterpret_cast<const char*> (pDyn->un.ptr);
              break;
            case ELF32_DT_SYMENT:
              m_nDynamicSymbolTableSize = pDyn->un.val;
              break;
            case ELF32_DT_STRSZ:
              nDynamicStringTableSize = pDyn->un.val;
              break;
            case ELF32_DT_RELA:
              m_pRelaTable = reinterpret_cast<Elf32Rela_t*> (pDyn->un.ptr);
              break;
            case ELF32_DT_REL:
              m_pRelTable = reinterpret_cast<Elf32Rel_t*> (pDyn->un.ptr);
              break;
            case ELF32_DT_RELSZ:
              m_nRelTableSize = pDyn->un.val;
              break;
            case ELF32_DT_RELASZ:
              m_nRelaTableSize = pDyn->un.val;
              break;
            case ELF32_DT_PLTGOT:
              m_pGotTable = reinterpret_cast<uintptr_t*> (pDyn->un.ptr);
              break;
            case ELF32_DT_JMPREL:
            {
              if (m_bUsesRela)
                m_pPltRelaTable = reinterpret_cast<Elf32Rela_t*> (pDyn->un.ptr);
              else
                m_pPltRelTable = reinterpret_cast<Elf32Rel_t*> (pDyn->un.ptr);
              break;
            }
            case ELF32_DT_PLTREL:
            {
              if (pDyn->un.val == ELF32_DT_RELA)
              {
                m_bUsesRela = true;
              }
              break;
            }
            case ELF32_DT_PLTRELSZ:
              m_nPltSize = pDyn->un.val;
              break;
          }

          pDyn++;
        }
      }

      // If we found a dynamic symbol table, string table and Rel(a) table, attempt to find the segment they reside in
      // and copy them locally.
      if (m_pDynamicSymbolTable)
      {
        for (int i = 0; i < m_nProgramHeaders; i++)
        {
          Elf32ProgramHeader_t ph = m_pProgramHeaders[i];
          if ( (ph.addr <= m_pDynamicSymbolTable) && (m_pDynamicSymbolTable < ph.addr+ph.size) )
          {
            uintptr_t loc = reinterpret_cast<uintptr_t>(m_pDynamicSymbolTable) - ph.addr;
            m_pDynamicSymbolTable = new Elf32DynamicSymbol_t[m_nDynamicSymbolTableSize/sizeof(Elf32DynamicSymbol_t)];
            memcpy (reinterpret_cast<uint8_t*>(m_pDynamicSymbolTable), reinterpret_cast<uint8_t*>(loc),
                    m_nDynamicSymbolTableSize);
            break;
          }
        }
      }
      if (m_pDynamicStringTable)
      {
        for (int i = 0; i < m_nProgramHeaders; i++)
        {
          Elf32ProgramHeader_t ph = m_pProgramHeaders[i];
          if ( (ph.addr <= m_pDynamicStringTable) && (m_pDynamicStringTable < ph.addr+ph.size) )
          {
            uintptr_t loc = reinterpret_cast<uintptr_t>(m_pDynamicStringTable) - ph.addr;
            m_pDynamicStringTable = new char[nDynamicStringTableSize];
            memcpy (reinterpret_cast<uint8_t*>(m_pDynamicStringTable), reinterpret_cast<uint8_t*>(loc),
                    nDynamicStringTableSize);
            break;
          }
        }
        // Make sure the string references to needed libraries actually work as pointers...
        for (List<char*>::Iterator it = m_NeededLibraries.begin();
             it != m_NeededLibraries.end();
             it++)
        {
          *it = *it + m_pDynamicStringTable;
        }
      }
      if (m_pRelTable)
      {
        for (int i = 0; i < m_nProgramHeaders; i++)
        {
          Elf32ProgramHeader_t ph = m_pProgramHeaders[i];
          if ( (ph.addr <= m_pRelTable) && (m_pRelTable < ph.addr+ph.size) )
          {
            uintptr_t loc = reinterpret_cast<uintptr_t>(m_pRelTable) - ph.addr;
            m_pRelTable = new Elf32Rel_t[m_nRelTableSize/sizeof(Elf32Rel_t)];
            memcpy (reinterpret_cast<uint8_t*>(m_pRelTable), reinterpret_cast<uint8_t*>(loc),
                    m_nRelTableSize);
            break;
          }
        }
      }
      if (m_pRelaTable)
      {
        for (int i = 0; i < m_nProgramHeaders; i++)
        {
          Elf32ProgramHeader_t ph = m_pProgramHeaders[i];
          if ( (ph.addr <= m_pRelaTable) && (m_pRelaTable < ph.addr+ph.size) )
          {
            uintptr_t loc = reinterpret_cast<uintptr_t>(m_pRelaTable) - ph.addr;
            m_pRelaTable = new Elf32Rela_t[m_nRelaTableSize/sizeof(Elf32Rela_t)];
            memcpy (reinterpret_cast<uint8_t*>(m_pRelaTable), reinterpret_cast<uint8_t*>(loc),
                    m_nRelaTableSize);
            break;
          }
        }
      }
      if (m_pPltRelTable)
      {
        for (int i = 0; i < m_nProgramHeaders; i++)
        {
          Elf32ProgramHeader_t ph = m_pProgramHeaders[i];
          if ( (ph.addr <= m_pPltRelTable) && (m_pPltRelTable < ph.addr+ph.size) )
          {
            uintptr_t loc = reinterpret_cast<uintptr_t>(m_pPltRelTable) - ph.addr;
            m_pPltRelTable = new Elf32Rel_t[m_nPltSize/sizeof(Elf32Rel_t)];
            memcpy (reinterpret_cast<uint8_t*>(m_pPltRelTable), reinterpret_cast<uint8_t*>(loc),
                    m_nPltSize);
            break;
          }
        }
      }
      if (m_pPltRelaTable)
      {
        for (int i = 0; i < m_nProgramHeaders; i++)
        {
          Elf32ProgramHeader_t ph = m_pProgramHeaders[i];
          if ( (ph.addr <= m_pPltRelaTable) && (m_pPltRelaTable < ph.addr+ph.size) )
          {
            uintptr_t loc = reinterpret_cast<uintptr_t>(m_pPltRelaTable) - ph.addr;
            m_pPltRelaTable = new Elf32Rela_t[m_nPltSize/sizeof(Elf32Rela_t)];
            memcpy (reinterpret_cast<uint8_t*>(m_pPltRelaTable), reinterpret_cast<uint8_t*>(loc),
                    m_nPltSize);
            break;
          }
        }
      }
    }
  }

  m_nEntry = pHeader->entry;

  // Success.
  return true;
}

bool Elf32::loadModule(uint8_t *pBuffer, size_t length, uintptr_t &loadBase)
{
  // Run through the sections to calculate the size required.
  uintptr_t size = 0;
  for (int i = 0; i < m_nSectionHeaders; i++)
  {
    if (m_pSectionHeaders[i].flags & SHF_ALLOC)
    {
      /// \todo Total bollocks?
      size += m_pSectionHeaders[i].addr; // The .addr won't be accounted for in the .size,
                                           // so add it here so we don't end up overwriting what we just wrote!
      size += m_pSectionHeaders[i].size;
    }
  }

  loadBase = KernelElf::instance().getModuleAllocator()->allocate(size);

  // Now actually map and populate the sections.
  uintptr_t offset = loadBase;
  for (int i = 0; i < m_nSectionHeaders; i++)
  {
    if (m_pSectionHeaders[i].flags & SHF_ALLOC)
    {
      // Add load-base into the equation.
      if (m_pSectionHeaders[i].addr == 0)
        m_pSectionHeaders[i].addr = offset;
      else
      {
        int tmp = m_pSectionHeaders[i].addr;
        m_pSectionHeaders[i].addr += offset;
        /// \todo Total bollocks?
        offset += tmp; // The .addr won't be accounted for in the .size, so add it here so we don't
                       // end up overwriting what we just wrote!
      }

      // We now know where to place this section, so map some memory for it.
      for (uintptr_t j = m_pSectionHeaders[i].addr;
           j < (m_pSectionHeaders[i].addr+m_pSectionHeaders[i].size)+0x1000; /// \todo This isn't the correct formula - fix.
           j += 0x1000)
      {
        physical_uintptr_t phys = PhysicalMemoryManager::instance().allocatePage();
        bool b = Processor::information().getVirtualAddressSpace().map(phys,
                                            reinterpret_cast<void*> (j&0xFFFFF000),
                                            VirtualAddressSpace::Write | VirtualAddressSpace::KernelMode);
        if (!b)
          WARNING("map() failed for section " << Dec << i << ", address " << Hex << j);
      }

      if (m_pSectionHeaders[i].type != SHT_NOBITS)
      {
        // Copy section data from the file.
        memcpy (reinterpret_cast<uint8_t*> (m_pSectionHeaders[i].addr),
                        &pBuffer[m_pSectionHeaders[i].offset],
                        m_pSectionHeaders[i].size);
#if defined(PPC_COMMON) || defined(MIPS_COMMON)
        Processor::flushDCacheAndInvalidateICache(m_pSectionHeaders[i].addr, m_pSectionHeaders[i].addr+m_pSectionHeaders[i].size);
#endif
      }
      else
      {
        memset (reinterpret_cast<uint8_t*> (m_pSectionHeaders[i].addr),
                        0,
                        m_pSectionHeaders[i].size);
      }
      offset += m_pSectionHeaders[i].size;
    }
  }

  // Firstly, we need to change the symbol table so that the ::value member is actually valid.
  // Currently, it's the offset into the symbol's section - we add the section base address
  // on to that to make it a valid pointer.
  Elf32Symbol_t *pSymbol = m_pSymbolTable;
  for (size_t i = 0; i < m_nSymbolTableSize / sizeof(Elf32Symbol_t); i++)
  {
    // Only relocate functions, variables and notypes.
    if (ELF32_ST_TYPE(pSymbol->info) < 3 && pSymbol->shndx < m_pHeader->shnum)
    {
      Elf32SectionHeader_t *pSh = &m_pSectionHeaders[pSymbol->shndx];
      pSymbol->value += pSh->addr;
    }
    pSymbol ++;
  }

  relocateModinfo(pBuffer, length);

  return true;
}

bool Elf32::finaliseModule(uint8_t *pBuffer, uint32_t length)
{
  return relocate(pBuffer, length);
}

bool Elf32::allocate(uint8_t *pBuffer, size_t length, uintptr_t &loadBase)
{
  // Scan the segments to find the size.
  uintptr_t size = 0;
  uintptr_t start = ~0;
  for (int i = 0; i < m_nProgramHeaders; i++)
  {
    if (m_pProgramHeaders[i].type == ELF32_PT_LOAD)
    {
      /// \todo Total bollocks?
      size += m_pProgramHeaders[i].vaddr;// The .addr won't be accounted for in the .size, so add it here so we don't
                                         // end up overwriting what we just wrote!
      size += m_pProgramHeaders[i].size;
      if (m_pProgramHeaders[i].vaddr < start) start = m_pProgramHeaders[i].vaddr;
    }
  }

  // Here we use an atrocious heuristic for determining if the Elf needs relocating - if its entry point is < 1MB, it
  // is likely that it needs relocation.
  if (m_nEntry < 0x100000)
  {
    loadBase = Processor::information().getCurrentThread()->getParent()->getSpaceAllocator()->allocate(size);
    if (!loadBase)
      return false;
  }
  else
  {
    loadBase = 0;
    // Make sure the Process knows that we've just plonked an Elf at a specific place, and doesn't try to allocate
    // mmaps or libraries over it!
    if (!Processor::information().getCurrentThread()->getParent()->getSpaceAllocator()->allocateSpecific(start, size))
      return false;
  }

  // Allocate segments only.
  uintptr_t offset = loadBase;
  for (int i = 0; i < m_nProcessHeaders; i++)
  {
    if (m_pProgramHeaders[i].type == ELF32_PT_LOAD)
    {
      uintptr_t loadAddr = m_pProgramHeaders[i].vaddr;
      // If we're loading a relocated file...
      if (loadBase != 0)
      {
        // Add load-base into the equation.
        if (m_pProgramHeaders[i].vaddr == 0)
          loadAddr = offset;
        else
        {
          int tmp = m_pProgramHeaders[i].vaddr;
          loadAddr = m_pProgramHeaders[i].vaddr + offset;
          /// \todo Total bollocks?
          offset += tmp; // The .addr won't be accounted for in the .size, so add it here so we don't
                          // end up overwriting what we just wrote!
        }
      }

      /// \todo Algorithm isn't correct - fix.
      for (unsigned int j = loadAddr; j < (loadAddr+m_pProgramHeaders[i].memsz)+0x1000; j += 0x1000)
      {
        physical_uintptr_t phys = PhysicalMemoryManager::instance().allocatePage();
        bool b = Processor::information().getVirtualAddressSpace().map(phys,
                                                                       reinterpret_cast<void*> (j&0xFFFFF000),
                                                                       VirtualAddressSpace::Write);
        if (!b)
          WARNING("map() failed for section " << Dec << i << ", address " << Hex << j);
      }
      offset += m_pProgramHeaders[i].size;
    }
  }

  return true;
}

bool Elf32::load(uint8_t *pBuffer, size_t length, uintptr_t loadBase, SymbolLookupFn fn, uintptr_t nStart, uintptr_t nEnd)
{
  uintptr_t offset = loadBase;
  for (int i = 0; i < m_nProgramHeaders; i++)
  {
    if (m_pProgramHeaders[i].type == ELF32_PT_LOAD)
    {
      uintptr_t loadAddr = m_pProgramHeaders[i].vaddr;
      // If we're loading a relocated file...
      if (m_LoadBase != 0)
      {
        // Add load-base into the equation.
        if (m_pProgramHeaders[i].vaddr == 0)
          loadAddr = offset;
        else
        {
          int tmp = m_pProgramHeaders[i].vaddr;
          loadAddr = m_pProgramHeaders[i].vaddr + offset;
          /// \todo Total bollocks?
          offset += tmp; // The .addr won't be accounted for in the .size, so add it here so we don't
                         // end up overwriting what we just wrote!
        }
      }

      // Check whether we should be copying this segment at all (nStart/nEnd)
      if (loadAddr >= nStart && nEnd < (loadAddr+m_pProgramHeaders[i].memsz))
      {
        uintptr_t sectionStart = (loadAddr >= nStart) ? loadAddr : nStart;
        uintptr_t offset = m_pProgramHeaders[i].offset + (sectionStart-loadAddr);
        uintptr_t filesz = (loadAddr+m_pProgramHeaders[i].filesz >= nEnd)
                                                      ? (nEnd-sectionStart)
                                                      : (loadAddr+m_pProgramHeaders[i].filesz-sectionStart);
        uintptr_t memsz = (loadAddr+m_pProgramHeaders[i].filesz >= nEnd)
                                                      ? 0
                                                      : (loadAddr+m_pProgramHeaders[i].memsz-sectionStart);

        // Copy segment data from the file.
        memcpy (reinterpret_cast<uint8_t*> (nStart),
                &pBuffer[offset],
                filesz);

        memset (reinterpret_cast<uint8_t*> (nStart+filesz),
                0,
                memsz);

#if defined(PPC_COMMON) || defined(MIPS_COMMON)
        Processor::flushDCacheAndInvalidateICache(loadAddr, loadAddr+m_pProgramHeaders[i].filesz);
#endif
      }
      offset += m_pProgramHeaders[i].size;
    }
  }

  // Apply relocations for the given area.

  // Is it a relocation section?
  if (m_pRelTable)
  {
    Elf32Rel_t *pRelTable = adjust_pointer(m_pRelTable, loadBase);
    // For each relocation entry...
    for (Elf32Rel_t *pRel = pRelTable;
          pRel < (m_pRelTable+(m_nRelTableSize/sizeof(Elf32Rel_t)));
          pRel++)
    {
      if ( (pRel->offset + loadBase < nStart) || (pRel->offset + loadBase > nEnd) )
        continue;
      if (!applyRelocation(*pRel, 0, fn, loadBase))
        return false;
    }
  }
  // How about a relocation with addend?
  if (m_pRelaTable)
  {
    // For each relocation entry...
    for (Elf32Rela_t *pRel = pRelaTable;
          pRel < (m_pRelaTable+(m_nRelaTableSize/sizeof(Elf32Rela_t)));
          pRel++)
    {
      if (!applyRelocation(*pRel, 0, fn, loadBase))
        return false;
    }
  }

  // Success.
  return true;
}

unsigned int Elf32::getLastAddress()
{
  // TODO
  return 0;
}

const char *Elf32::lookupSymbol(uintptr_t addr, uintptr_t *startAddr)
{
  if (!m_pSymbolTable || !m_pStringTable)
    return 0; // Just return null if we haven't got a symbol table.

  Elf32Symbol_t *pSymbol = reinterpret_cast<Elf32Symbol_t *>(m_pSymbolTable);

  const char *pStrtab = reinterpret_cast<const char *>(m_pStringTable);

  for (size_t i = 0; i < m_pSymbolTable->size / sizeof(Elf32Symbol_t); i++)
  {
    // Make sure we're looking at an object or function.
    if (ELF32_ST_TYPE(pSymbol->info) != 0x2 /* function */ &&
        ELF32_ST_TYPE(pSymbol->info) != 0x0 /* notype (asm functions) */)
    {
      pSymbol++;
      continue;
    }

    // If we're checking for a symbol that is apparently zero-sized, add one so we can actually
    // count it!
    uint32_t size = pSymbol->size;
    if (size == 0)
      size = 0x100;
    if ( (addr >= pSymbol->value) &&
         (addr < (pSymbol->value + size)) )
    {
      const char *pStr = pStrtab + pSymbol->name;
      if (startAddr)
        *startAddr = pSymbol->value;
      return pStr;
    }
    pSymbol ++;
  }
  return 0;

}

uint32_t Elf32::lookupSymbol(const char *pName)
{
  if (!m_pSymbolTable || !m_pStringTable)
    return 0; // Just return null if we haven't got a symbol table.

  Elf32Symbol_t *pSymbol = reinterpret_cast<Elf32Symbol_t *>(m_pSymbolTable);

  const char *pStrtab = reinterpret_cast<const char *>(m_pStringTable);

  for (size_t i = 0; i < m_pSymbolTable->size / sizeof(Elf32Symbol_t); i++)
  {
    const char *pStr;
    if (ELF32_ST_TYPE(pSymbol->info) == 3)
    {
      // Section type - the name will be the name of the section header it refers to.
      Elf32SectionHeader_t *pSh = &m_pSectionHeaders[pSymbol->shndx];
      // If it's not allocated, it's a link-once-only section that we can ignore.
      if (!(pSh->flags & SHF_ALLOC))
      {
        pSymbol++;
        continue;
      }
      // Grab the shstrtab
      pStr = reinterpret_cast<const char*> (m_pShstrtab) + pSh->name;
    }
    else
      pStr = pStrtab + pSymbol->name;

    if (!strcmp(pName, pStr))
    {
      return pSymbol->value;
    }
    pSymbol ++;
  }
  return 0;
}

uint32_t Elf32::lookupDynamicSymbolAddress(const char *sym)
{
  if (!m_pDynamicSymbolTable || !m_pDynamicStringTable)
    return 0; // Just return null if we haven't got a symbol table.

  Elf32Symbol_t *pSymbol = m_pDynamicSymbolTable;

  const char *pStrtab = m_pDynamicStringTable;

  /// \todo Don't rely on this. Look at nchain in the hash table.
  while (reinterpret_cast<uintptr_t>(pSymbol) < reinterpret_cast<uintptr_t>(m_pDynamicStringTable))
  {
    const char *pStr = pStrtab + pSymbol->name;

    // Check the type is global!
    if (!strcmp(sym, pStr))
    {
      return pSymbol->value + m_LoadBase;
    }
    pSymbol ++;
  }

  return 0;
}

uintptr_t Elf32::getGlobalOffsetTable(uintptr_t loadBase)
{
  return reinterpret_cast<uintptr_t> (m_pGotTable);
}

uint32_t Elf32::getEntryPoint()
{
  return m_nEntry;
}

bool Elf32::relocate(uint8_t *pBuffer, uint32_t length)
{
  // For every section...
  for (int i = 0; i < m_nSectionHeaders; i++)
  {
    Elf32SectionHeader_t *pSh = &m_pSectionHeaders[i];

    if (pSh->type != ELF32_SHT_REL && pSh->type != ELF32_SHT_RELA)
      continue;

    // Grab the section header that this relocation section refers to.
    Elf32SectionHeader_t *pLink = &m_pSectionHeaders[pSh->info];

    // Grab the shstrtab
    const char *pStr = reinterpret_cast<const char*> (m_pShstrtab) + pLink->name;
    if (!strcmp(pStr, ".modinfo"))
      continue;

    // Is it a relocation section?
    if (pSh->type == SHT_REL)
    {
      // For each relocation entry...
      for (Elf32Rel_t *pRel = reinterpret_cast<Elf32Rel_t*> (&pBuffer[pSh->offset]);
           pRel < reinterpret_cast<Elf32Rel_t*> (&pBuffer[pSh->offset+pSh->size]);
           pRel++)
      {
        if (!applyRelocation(*pRel, pLink))
          return false;
      }
    }
    // How about a relocation with addend?
    else if (pSh->type == SHT_RELA)
    {
      // For each relocation entry...
      for (Elf32Rela_t *pRel = reinterpret_cast<Elf32Rela_t*> (&pBuffer[pSh->offset]);
           pRel < reinterpret_cast<Elf32Rela_t*> (&pBuffer[pSh->offset+pSh->size]);
           pRel++)
      {
        if (!applyRelocation(*pRel, pLink))
          return false;
      }
    }
  }

  // Success!
  return true;
}

bool Elf32::relocateModinfo(uint8_t *pBuffer, uint32_t length)
{
  // For every section...
  for (int i = 0; i < m_nSectionHeaders; i++)
  {
    Elf32SectionHeader_t *pSh = &m_pSectionHeaders[i];
    if (pSh->type != ELF32_SHT_REL && pSh->type != ELF32_SHT_RELA)
      continue;
    // Grab the section header that this relocation section refers to.
    Elf32SectionHeader_t *pLink = &m_pSectionHeaders[pSh->info];

    // Grab the shstrtab
    const char *pStr = reinterpret_cast<const char*> (m_pShstrtab->addr) + pLink->name;
    if (strcmp(pStr, ".modinfo"))
      continue;

    // Is it a relocation section?
    if (pSh->type == SHT_REL)
    {
      // For each relocation entry...
      for (Elf32Rel_t *pRel = reinterpret_cast<Elf32Rel_t*> (&pBuffer[pSh->offset]);
           pRel < reinterpret_cast<Elf32Rel_t*> (&pBuffer[pSh->offset+pSh->size]);
           pRel++)
      {
        if (!applyRelocation(*pRel, pLink))
          return false;
      }
    }
    // How about a relocation with addend?
    else if (pSh->type == SHT_RELA)
    {
      // For each relocation entry...
      for (Elf32Rela_t *pRel = reinterpret_cast<Elf32Rela_t*> (&pBuffer[pSh->offset]);
           pRel < reinterpret_cast<Elf32Rela_t*> (&pBuffer[pSh->offset+pSh->size]);
           pRel++)
      {
        if (!applyRelocation(*pRel, pLink))
          return false;
      }
    }
  }

  // Success!
  return true;
}

uintptr_t Elf32::applySpecificRelocation(uint32_t off, SymbolLookupFn fn, uintptr_t loadBase)
{
  // Is it a relocation section?
  if (m_pPltRelTable)
  {
    // For each relocation entry...
    Elf32Rel_t *pRel = adjust_pointer(m_pPltRelTable, loadBase+off);

    applyRelocation(*pRel, 0, fn, loadBase);

    uintptr_t address = loadBase + pRel->offset;

    return * reinterpret_cast<uintptr_t*> (address);
  }
  // How about a relocation with addend?
  if (m_pPltRelaTable)
  {
    // For each relocation entry...
    Elf32Rela_t *pRel = adjust_pointer(m_pPltRelaTable, loadBase+off);

    applyRelocation(*pRel, 0, fn, loadBase);

    uintptr_t address = m_LoadBase + pRel->offset;

    return * reinterpret_cast<uintptr_t*> (address);
  }
}

uintptr_t Elf32::debugFrameTable()
{
  return m_pDebugTable->addr;
}

uintptr_t Elf32::debugFrameTableLength()
{
  return m_pDebugTable->size;
}

List<char*> &Elf32::neededLibraries()
{
  return m_NeededLibaries;
}

size_t Elf32::getPltSize ()
{
  return m_nPltSize;
}
