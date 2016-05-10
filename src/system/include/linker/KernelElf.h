/*
 * Copyright (c) 2008-2014, Pedigree Developers
 *
 * Please see the CONTRIB file in the root of the source tree for a full
 * list of contributors.
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
#include <utilities/SharedPointer.h>

#ifdef THREADS
#include <process/Semaphore.h>
#include <Spinlock.h>
#endif

#ifdef STATIC_DRIVERS
#include <Module.h>
#endif

/** @addtogroup kernellinker
 * @{ */

class Module
{
    public:
        Module() : elf(), name(0), entry(0), exit(0), depends(0),
            depends_opt(0), buffer(0), buflen(0) {}
        Elf elf;
        const char *name;
        bool (*entry)();
        void (*exit)();
        const char **depends;
        const char **depends_opt;
        uint8_t *buffer;
        size_t buflen;
        uintptr_t loadBase;
        size_t loadSize;
    protected:
        Module(const Module &);
        Module &operator = (const Module &);
};

class KernelElf : public Elf
{
    friend void system_reset();
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
        *\param silent If true will not update the boot progress(default is false).
        *\return A pointer to a Elf class describing the loaded module. */
        Module *loadModule(uint8_t *pModule, size_t len, bool silent=false);
#ifdef STATIC_DRIVERS
        Module *loadModule(struct ModuleInfo *info, bool silent = false);
#endif

        /** Unloads the specified module. */
        void unloadModule(const char *name, bool silent=false, bool progress=true);
        void unloadModule(Vector<Module*>::Iterator it, bool silent=false, bool progress=true);

        /** Unloads all loaded modules. */
        void unloadModules();

        /** Returns true if a module with the specified name has been loaded. */
        bool moduleIsLoaded(char *name);

        /** Returns the name of the first module that have the specified module as dependency. */
        char *getDependingModule(char *name);

        /** Looks up the address of the symbol with name 'pName' globally, that is throughout
        *  all modules and the kernel itself. */
        uintptr_t globalLookupSymbol(const char *pName);
        const char *globalLookupSymbol(uintptr_t addr, uintptr_t *startAddr=0);

        /** Returns the address space allocator for modules. */
        MemoryAllocator &getModuleAllocator() {return m_ModuleAllocator;}

        /** Do we have pending modules still? */
        bool hasPendingModules() const;

        /** Updates the status of the given module. */
        void updateModuleStatus(Module *module, bool status);

        /** Waits for all modules to complete (whether successfully or not). */
        void waitForModulesToLoad();

    private:
        /** Default constructor does nothing */
        KernelElf() INITIALISATION_ONLY;
        /** Copy-constructor
        *\note NOT implemented (singleton class) */
        KernelElf(const KernelElf &);
        /** Destructor does nothing */
        virtual ~KernelElf();
        /** Assignment operator
        *\note NOT implemented (singleton class) */
        KernelElf &operator = (const KernelElf &);

        bool moduleDependenciesSatisfied(Module *module);
        bool executeModule(Module *module);

#if defined(X86_COMMON)
        MemoryRegion m_AdditionalSectionContents;
        MemoryRegion *m_AdditionalSectionHeaders;
#endif

        /** Instance of the KernelElf class */
        static KernelElf m_Instance;

        /** List of modules */
        Vector<Module*> m_Modules;
        /** List of successfully loaded modules. */
        Vector<Module*> m_LoadedModules;
        /** List of unsuccessfully loaded modules. */
        Vector<SharedPointer<String>> m_FailedModules;
        /** List of pending modules - modules whose dependencies have not yet been
            satisfied. */
        Vector<Module*> m_PendingModules;
        /** Memory allocator for modules - where they can be loaded. */
        MemoryAllocator m_ModuleAllocator;

        /** Override Elf base class members. */
#if defined(X86_COMMON)
        Elf32SectionHeader_t   *m_pSectionHeaders;
        Elf32Symbol_t          *m_pSymbolTable;

        typedef Elf32SectionHeader_t KernelElfSectionHeader_t;
        typedef Elf32Symbol_t KernelElfSymbol_t;
#else
        ElfSectionHeader_t   *m_pSectionHeaders;
        ElfSymbol_t          *m_pSymbolTable;

        typedef ElfSectionHeader_t KernelElfSectionHeader_t;
        typedef ElfSymbol_t KernelElfSymbol_t;
#endif

        /** Tracks the module loading process. */
#ifdef THREADS
        Semaphore m_ModuleProgress;
        Spinlock m_ModuleAdjustmentLock;
#endif
};

/** @} */

#endif
