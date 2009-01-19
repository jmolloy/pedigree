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

#include <KernelElf.h>
#include <utilities/utility.h>
#include <processor/Processor.h>
#include <processor/VirtualAddressSpace.h>
#include <processor/PhysicalMemoryManager.h>
#include <Log.h>

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
                                            PhysicalMemoryManager::continuous,
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
    else if (!strcmp(pStr, ".shstrtab"))
    {
      #if defined(X86_COMMON)
        m_pShstrtab = m_AdditionalSections.convertPhysicalPointer<ElfSectionHeader_t>(reinterpret_cast<physical_uintptr_t>(pSh));
      #else
        m_pShstrtab = pSh;
      #endif
    }
    else if (!strcmp(pStr, ".debug_frame"))
    {
      #if defined(X86_COMMON)
        m_pDebugTable = m_AdditionalSections.convertPhysicalPointer<ElfSectionHeader_t>(reinterpret_cast<physical_uintptr_t>(pSh));
      #else
        m_pDebugTable = pSh;
      #endif
    }
  }

  // Initialise remaining member variables
  #if defined(X86_COMMON)
    m_pSectionHeaders = m_AdditionalSections.convertPhysicalPointer<ElfSectionHeader_t>(pBootstrap.addr);
  #else
    m_pSectionHeaders = reinterpret_cast<ElfSectionHeader_t*>(pBootstrap.addr);
  #endif
  m_nSectionHeaders = pBootstrap.num;

  return true;
}

KernelElf::KernelElf() :
  #if defined(X86_COMMON)
  m_AdditionalSections("Kernel ELF Sections"),
  #endif
  m_Modules()
{
}

KernelElf::~KernelElf()
{
}
#ifdef PPC_COMMON
uintptr_t loadbase = 0xe1000000;
#else
uintptr_t loadbase = 0xfa000000;
#endif
Module *KernelElf::loadModule(uint8_t *pModule, size_t len)
{
  Module *module = new Module;

  if (!module->elf.load(pModule, len))
  {
    ERROR ("Module load failed (1)");
    delete module;
    return 0;
  }

  /// \todo assign memory, and a decent address.
  for(int i = 0; i < 0x40000; i += 0x1000)
  {
    physical_uintptr_t phys = PhysicalMemoryManager::instance().allocatePage();
    bool b = VirtualAddressSpace::getKernelAddressSpace().map(phys,
                                                              reinterpret_cast<void*> (loadbase+i),
                                                              VirtualAddressSpace::Write);
    if (!b)
      WARNING("map() failed");
  }
  module->elf.setLoadBase(loadbase);

  if (!module->elf.writeSections())
  {
    ERROR ("Module load failed (2)");
    delete module;
    return 0;
  }
  if (!module->elf.relocateModinfo())
  {
    ERROR ("Module load failed (3)");
    delete module;
    return 0;
  }

  // Look up the module's name and entry/exit functions, and dependency list.
  module->name = *reinterpret_cast<const char**> (module->elf.lookupSymbol("g_pModuleName"));
  module->entry = *reinterpret_cast<void (**)()> (module->elf.lookupSymbol("g_pModuleEntry"));
  module->exit = *reinterpret_cast<void (**)()> (module->elf.lookupSymbol("g_pModuleExit"));
  module->depends = reinterpret_cast<const char **> (module->elf.lookupSymbol("g_pDepends"));
  NOTICE("Relocated module " << module->name);

#if defined(PPC_COMMON) || defined(MIPS_COMMON)
  ///\todo proper size in here.
  Processor::flushDCacheAndInvalidateICache(loadbase, loadbase+0x20000);
#endif
  loadbase += 0x100000;

  m_Modules.pushBack(module);

  // Can we load this module yet?
  if (moduleDependenciesSatisfied(module))
  {
    executeModule(module);

    // Now check if we've allowed any currently pending modules to load.
    bool somethingLoaded = true;
    while (somethingLoaded)
    {
      somethingLoaded = false;
      for (Vector<Module*>::Iterator it = m_PendingModules.begin();
           it != m_PendingModules.end();
           it++)
      {
        if (moduleDependenciesSatisfied(*it))
        {
          executeModule(*it);
          m_PendingModules.erase(it);
          somethingLoaded = true;
          break;
        }
      }
    }
  }
  else
  {
    m_PendingModules.pushBack(module);
  }

  return module;
}

bool KernelElf::moduleDependenciesSatisfied(Module *module)
{
  int i = 0;
  if (module->depends == 0) return true;

  while (module->depends[i] != 0)
  {
    bool found = false;
    for (int j = 0; j < m_LoadedModules.count(); j++)
    {
      if (!strcmp(m_LoadedModules[j], module->depends[i]))
      {
        found = true;
        break;
      }
    }
    if (!found){return false;}
    i++;
  }
  return true;
}

void KernelElf::executeModule(Module *module)
{
  m_LoadedModules.pushBack(const_cast<char*>(module->name));

  if (!module->elf.relocate())
  {
    ERROR ("Module relocation failed");
    Processor::breakpoint();
    return;
  }

  // Check for a constructors list and execute.
  uintptr_t startCtors = module->elf.lookupSymbol("start_ctors");
  uintptr_t endCtors = module->elf.lookupSymbol("end_ctors");

  if (startCtors && endCtors)
  {
    uintptr_t *iterator = reinterpret_cast<uintptr_t*>(startCtors);
    while (iterator < reinterpret_cast<uintptr_t*>(endCtors))
    {
      void (*fp)(void) = reinterpret_cast<void (*)(void)>(*iterator);
      fp();
      iterator++;
    }
  }

  NOTICE("Executing module " << module->name);
  if (module)
    module->entry();
}

uintptr_t KernelElf::globalLookupSymbol(const char *pName)
{
  /// \todo This shouldn't match local or weak symbols.
  // Try a lookup in the kernel.
  uintptr_t ret;
  if ((ret = lookupSymbol(pName)))
    return ret;

  // OK, try every module.
  for (Vector<Module*>::Iterator it = m_Modules.begin();
       it != m_Modules.end();
       it++)
  {
    if ((ret = (*it)->elf.lookupSymbol(pName)))
      return ret;
  }

  return 0;
}

const char *KernelElf::globalLookupSymbol(uintptr_t addr, uintptr_t *startAddr)
{
  /// \todo This shouldn't match local or weak symbols.
  // Try a lookup in the kernel.
  const char *ret;

  if ((ret = lookupSymbol(addr, startAddr)))
    return ret;

  // OK, try every module.
  for (Vector<Module*>::Iterator it = m_Modules.begin();
       it != m_Modules.end();
       it++)
  {
    if ((ret = (*it)->elf.lookupSymbol(addr, startAddr)))
      return ret;
  }
  WARNING("KernelElf::GlobalLookupSymbol(" << Hex << addr << ") failed.");
  return 0;
}
