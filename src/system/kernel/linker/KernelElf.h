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

#ifndef KERNEL_LINKER_KERNELELF_H
#define KERNEL_LINKER_KERNELELF_H

#include <linker/Elf.h>
#include <compiler.h>
#include <processor/MemoryRegion.h>
#include <BootstrapInfo.h>
#include <utilities/Vector.h>
#include <utilities/MemoryAllocator.h>

/** @addtogroup kernellinker
 * @{ */

class Module
{
public:
  Module() : elf(), name(0), entry(0), exit(0), depends(0), buffer(0), buflen(0) {}
  Elf elf;
  const char *name;
  void (*entry)();
  void (*exit)();
  const char **depends;
  uint8_t *buffer;
  size_t buflen;
protected:
  Module(const Module &);
  Module &operator = (const Module &);
};

class KernelElf : public Elf
{
  public:
    /** Get the class instance
     *\return reference to the class instance */
    inline static KernelElf &instance()
      {return m_Instance;}

    /** Extracts the symbol and string tables from the given BootstrapInfo class. */
    bool initialise(const BootstrapStruct_t &pBootstrap) INITIALISATION_ONLY;

    /** Treats the given pointer as an ELF partially linked object file
     *  and loads it, relocates it and links it.
     *\param pModule A pointer to an ELF module/driver.
     *\param len The length of pModule, in bytes.
     *\return A pointer to a Elf class describing the loaded module. */
    Module *loadModule(uint8_t *pModule, size_t len);

    /** Looks up the address of the symbol with name 'pName' globally, that is throughout
     *  all modules and the kernel itself. */
    uintptr_t globalLookupSymbol(const char *pName);
    const char *globalLookupSymbol(uintptr_t addr, uintptr_t *startAddr=0);

    /** Returns the address space allocator for modules. */
    MemoryAllocator &getModuleAllocator() {return m_ModuleAllocator;}
    
  private:
    /** Default constructor does nothing */
    KernelElf() INITIALISATION_ONLY;
    /** Destructor does nothing */
    ~KernelElf();
    /** Copy-constructor
     *\note NOT implemented (singleton class) */
    KernelElf(const KernelElf &);
    /** Assignment operator
     *\note NOT implemented (singleton class) */
    KernelElf &operator = (const KernelElf &);

    bool moduleDependenciesSatisfied(Module *module);
    void executeModule(Module *module);

    #if defined(BITS_64)
      typedef Elf64SectionHeader_t ElfSectionHeader_t;
    #elif defined(BITS_32)
      typedef ElfSectionHeader_t ElfSectionHeader_t;
    #endif

    #if defined(X86_COMMON)
      MemoryRegion m_AdditionalSections;
    #endif

    /** Instance of the KernelElf class */
    static KernelElf m_Instance;

    /** List of modules */
    Vector<Module*> m_Modules;
    /** List of successfully loaded module names. */
    Vector<char*> m_LoadedModules;
    /** List of pending modules - modules whose dependencies have not yet been
        satisfied. */
    Vector<Module*> m_PendingModules;
    /** Memory allocator for modules - where they can be loaded. */
    MemoryAllocator m_ModuleAllocator;
};

/** @} */

#endif
