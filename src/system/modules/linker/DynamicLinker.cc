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

#include "DynamicLinker.h"
#include <Log.h>
#include <vfs/VFS.h>
#include <vfs/File.h>
#include <vfs/Symlink.h>
#include <utilities/StaticString.h>
#include <Module.h>
#include <processor/Processor.h>
#include <processor/VirtualAddressSpace.h>
#include <processor/PhysicalMemoryManager.h>
#include <processor/KernelCoreSyscallManager.h>
#include <process/Scheduler.h>
#include <panic.h>

DLTrapHandler DLTrapHandler::m_Instance;

uintptr_t DynamicLinker::resolvePlt(SyscallState &state)
{
    Process *pProcess = Processor::information().getCurrentThread()->getParent();

    return pProcess->getLinker()->resolvePltSymbol(state.getSyscallParameter(0), state.getSyscallParameter(1));
}

DynamicLinker::DynamicLinker() :
    m_pProgramElf(0), m_ProgramStart(0), m_ProgramSize(0), m_ProgramBuffer(0), m_Objects()
{
  
}

DynamicLinker::DynamicLinker(DynamicLinker &other) :
    m_pProgramElf(other.m_pProgramElf), m_ProgramStart(other.m_ProgramStart),
    m_ProgramSize(other.m_ProgramSize), m_ProgramBuffer(other.m_ProgramBuffer),
    m_Objects()
{
    for (Tree<uintptr_t,SharedObject*>::Iterator it = other.m_Objects.begin();
         it != other.m_Objects.end();
         it++)
    {
        uintptr_t key = reinterpret_cast<uintptr_t>(it.key());
        SharedObject *pSo = reinterpret_cast<SharedObject*>(it.value());
        m_Objects.insert(key, new SharedObject(pSo->elf, pSo->file, pSo->buffer,
                                               pSo->address, pSo->size));
    }
}

DynamicLinker::~DynamicLinker()
{
//    VirtualAddressSpace &va = Processor::information().getVirtualAddressSpace();

//    delete m_pProgramElf;
}

bool DynamicLinker::loadProgram(File *pFile)
{
    uintptr_t buffer;
    MemoryMappedFile *pMmFile = MemoryMappedFileManager::instance().map(pFile, buffer);

    m_pProgramElf = new Elf();
    if (!m_pProgramElf->create(reinterpret_cast<uint8_t*>(buffer), pFile->getSize()))
    {
        FATAL("DynamicLinker: Main program ELF failed to create: `" << pFile->getName() << "' at " << buffer);
        MemoryMappedFileManager::instance().unmap(pMmFile);
        return false;
    }
    NOTICE("Allocate");
    if (!m_pProgramElf->allocate(reinterpret_cast<uint8_t*>(buffer), pFile->getSize(), m_ProgramStart, 0, false, &m_ProgramSize))
    {
        ERROR("DynamicLinker: Main program ELF failed to load: `" << pFile->getName() << "'");
        MemoryMappedFileManager::instance().unmap(pMmFile);
        return false;
    }

    m_ProgramBuffer = buffer;

    NOTICE("Get dependencies");
    List<char*> &dependencies = m_pProgramElf->neededLibraries();

    // Load all dependencies
    /// \todo Check for cyclic dependencies.
    for (List<char*>::Iterator it = dependencies.begin();
         it != dependencies.end();
         it++)
    {
        String filename;
        filename += "root\xAF/libraries/";
        filename += *it;
        File *pFile = VFS::instance().find(filename);
        if (!pFile)
        {
            ERROR("DynamicLinker: Dependency `" << pFile->getName() << "' not found!");
            return false;
        }
        while (pFile && pFile->isSymlink())
            pFile = Symlink::fromFile(pFile)->followLink();
        if (!pFile || !loadObject(pFile))
        {
            ERROR("DynamicLinker: Dependency `" << pFile->getName() << "' failed to load!");
            return false;
        }
    }
    NOTICE("init plt");
    initPlt(m_pProgramElf, 0);

    return true;
}

bool DynamicLinker::loadObject(File *pFile)
{
    NOTICE("load object: " << pFile->getName());
    uintptr_t buffer;
    size_t size;
    uintptr_t loadBase;
    MemoryMappedFile *pMmFile = MemoryMappedFileManager::instance().map(pFile, buffer);

    Elf *pElf = new Elf();
    if (!pElf->create(reinterpret_cast<uint8_t*>(buffer), pFile->getSize()))
    {
        ERROR("DynamicLinker: ELF creation failed for file `" << pFile->getName() << "'");
        return false;
    }

    if (!pElf->allocate(reinterpret_cast<uint8_t*>(buffer), pFile->getSize(), loadBase, m_pProgramElf->getSymbolTable(), false, &size))
    {
        ERROR("DynamicLinker: ELF allocate failed for file `" << pFile->getName() << "'");
        return false;
    }
    NOTICE("Size: " << size);
    SharedObject *pSo = new SharedObject(pElf, pMmFile, buffer, loadBase, size);

    m_Objects.insert(loadBase, pSo);

    List<char*> &dependencies = pElf->neededLibraries();

    // Load all dependencies
    /// \todo Check for cyclic dependencies.
    for (List<char*>::Iterator it = dependencies.begin();
         it != dependencies.end();
         it++)
    {
        String filename;
        filename += "root:/libraries/";
        filename += *it;
        File *pFile = VFS::instance().find(filename);
        if (!pFile)
        {
            ERROR("DynamicLinker: Dependency `" << pFile->getName() << "' not found!");
            return false;
        }
        while (pFile && pFile->isSymlink())
            pFile = Symlink::fromFile(pFile)->followLink();
        if (!pFile || !loadObject(pFile))
        {
            ERROR("DynamicLinker: Dependency `" << pFile->getName() << "' failed to load!");
            return false;
        }
    }

    initPlt(pElf, loadBase);
    
    return true;
}

bool DynamicLinker::trap(uintptr_t address)
{
    Elf *pElf = 0;
    uintptr_t offset = 0;
    uintptr_t buffer = 0;
    size_t size = 0;

    if (address >= m_ProgramStart && address < m_ProgramStart+m_ProgramSize)
    {
        pElf = m_pProgramElf;
        offset = 0;
        buffer = m_ProgramBuffer;
        size = m_ProgramSize;
    }
    else
    {
        for (Tree<uintptr_t, SharedObject*>::Iterator it = m_Objects.begin();
             it != m_Objects.end();
             it++)
        {
            SharedObject *pSo = reinterpret_cast<SharedObject*>(it.value());

            if (address >= pSo->address && address < pSo->address+pSo->size)
            {
                pElf = pSo->elf;
                offset = pSo->address;
                buffer = pSo->buffer;
                size = pSo->size;
                break;
            }
        }
    }

    if (!pElf)
        return false;

    VirtualAddressSpace &va = Processor::information().getVirtualAddressSpace();

    uintptr_t v = address & ~(PhysicalMemoryManager::getPageSize()-1);

    // Grab a physical page.
    physical_uintptr_t p = PhysicalMemoryManager::instance().allocatePage();
    // Map it into the address space.
    if (!va.map(p, reinterpret_cast<void*>(v), VirtualAddressSpace::Write))
    {
        WARNING("IMAGE: map() failed in ElfImage::trap()");
        return false;
    }
    
    // Now that it's mapped, load the ELF region.
    if (pElf->load(reinterpret_cast<uint8_t*>(buffer), size, offset, m_pProgramElf->getSymbolTable(), v,
                     v+PhysicalMemoryManager::getPageSize()) == false)
    {
        WARNING("LINKER: load() failed in DynamicLinker::trap()");
        return false;
    }

    return true;
}

uintptr_t DynamicLinker::resolve(String name)
{
    return m_pProgramElf->getSymbolTable()->lookup(name, m_pProgramElf);
}

DLTrapHandler::DLTrapHandler()
{
    PageFaultHandler::instance().registerHandler(this);
}

DLTrapHandler::~DLTrapHandler()
{
}

bool DLTrapHandler::trap(uintptr_t address, bool bIsWrite)
{
    return Processor::information().getCurrentThread()->getParent()->getLinker()->trap(address);
}

void init()
{
    KernelCoreSyscallManager::instance().registerSyscall(KernelCoreSyscallManager::link, &DynamicLinker::resolvePlt);
}

void destroy()
{
}

MODULE_NAME("linker");
MODULE_ENTRY(&init);
MODULE_EXIT(&destroy);
MODULE_DEPENDS("VFS");
