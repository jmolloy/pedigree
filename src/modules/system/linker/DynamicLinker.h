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

#ifndef DYNAMIC_LINKER_H
#define DYNAMIC_LINKER_H

#include <processor/types.h>
#include <linker/Elf.h>
#include <process/Process.h>
#include <utilities/Tree.h>
#include <utilities/List.h>
#include <utilities/RadixTree.h>
#include <processor/state.h>
#include <vfs/File.h>
#include <vfs/MemoryMappedFile.h>

/** The dynamic linker tracks instances of shared objects through
    an address space. */
class DynamicLinker
{
public:
    /** Creates a new DynamicLinker object. */
    DynamicLinker();

    ~DynamicLinker();

    /** Creates a new DynamicLinker from another. Copies all mappings. */
    DynamicLinker(DynamicLinker &other);

    /** Checks the given program for a requested interpreter, that we should
        run instead of loading the binary ourselves. */
    bool checkInterpreter(File *pFile, String &actualFilename)
    {
        return loadProgram(pFile, true, true, &actualFilename);
    }

    /** Checks dependencies for a given program. Returns true if
        all dependencies are available and loadable, false if not. */
    bool checkDependencies(File *pFile)
    {
        return loadProgram(pFile, true);
    }

    /** Loads the main program. This must be an ELF file, and the
        linker will also load all library dependencies. If any of
        these loads fails, false is returned.
        \param pFile The ELF file. */
    bool loadProgram(File *pFile, bool bDryRun = false, bool bInterpreter = false, String *sInterpreter = 0);

    /** Loads a shared object into this address space (along with
        any dependencies.
        \param pFile The ELF object to load.
        \return True if the ELF and all dependencies was loaded successfully. */
    bool loadObject(File *pFile, bool bDryRun = false);

    /** Callback given to KernelCoreSyscallManager to resolve PLT relocations lazily. */
    static uintptr_t resolvePlt(SyscallState &state);

    /** Called when a trap is handled by the DLTrapHandler.
        \param address Address of the trap.
        \return True if the trap was handled successfully. */
    bool trap(uintptr_t address);

    /** Returns the program ELF. */
    Elf *getProgramElf()
        {return m_pProgramElf;}

    /** Manually resolves a given symbol name. */
    uintptr_t resolve(String name);

private:
    /** Operator= is unused and is therefore private. */
    DynamicLinker &operator=(const DynamicLinker&);

    /** A shared object/library. */
    struct SharedObject
    {
        SharedObject(Elf *e, MemoryMappedFile *f, uintptr_t b, uintptr_t a, size_t s) :
            elf(e), file(f), buffer(b), address(a), size(s)
        {}
        Elf                 *elf;
        MemoryMappedFile    *file;
        uintptr_t           buffer;
        uintptr_t           address;
        size_t              size;
    };

    uintptr_t resolvePltSymbol(uintptr_t libraryId, uintptr_t symIdx);

    void initPlt(Elf *pElf, uintptr_t value);

    Elf *m_pProgramElf;
    uintptr_t m_ProgramStart;
    size_t m_ProgramSize;
    uintptr_t m_ProgramBuffer;
    RadixTree<void*> m_LoadedObjects;

    Tree<uintptr_t, SharedObject*> m_Objects;
};

/** Tiny class for dispatching MemoryTrap events to DynamicLinkers.
    A MemoryTrap event will occur when a page of an ELF has yet to
    be loaded. */
class DLTrapHandler : public MemoryTrapHandler
{
public:
    /** Retrieve the singleton DLTrapHandler instance. */
    static DLTrapHandler &instance() {return m_Instance;}

    //
    // MemoryTrapHandler interface.
    //
    virtual bool trap(uintptr_t address, bool bIsWrite);

private:
    /** Private constructor - does nothing. */
    DLTrapHandler();
    /** Private destructor - does nothing, not expected to be called. */
    ~DLTrapHandler();

    static DLTrapHandler m_Instance;
};

#endif
