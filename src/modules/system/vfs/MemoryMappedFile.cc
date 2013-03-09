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

#include "MemoryMappedFile.h"

#include <processor/PhysicalMemoryManager.h>
#include <Spinlock.h>

MemoryMappedFileManager MemoryMappedFileManager::m_Instance;

MemoryMappedFile::MemoryMappedFile(File *pFile, size_t extentOverride) :
    m_pFile(pFile), m_Mappings(), m_bMarkedForDeletion(false),
    m_Extent(extentOverride ? extentOverride : pFile->getSize() + 1), m_RefCount(0), m_Lock()
{
    if (m_Extent & ~(PhysicalMemoryManager::getPageSize()-1))
    {
        m_Extent = (m_Extent + PhysicalMemoryManager::getPageSize()) &
            ~(PhysicalMemoryManager::getPageSize()-1);
    }
}

MemoryMappedFile::~MemoryMappedFile()
{
    // Free all physical pages.
    for (Tree<uintptr_t,uintptr_t>::Iterator it = m_Mappings.begin();
         it != m_Mappings.end();
         it++)
    {
        uintptr_t p = it.value();
        PhysicalMemoryManager::instance().freePage(p);
    }
    m_Mappings.clear();
}

bool MemoryMappedFile::load(uintptr_t &address, Process *pProcess, size_t extentOverride)
{
    LockGuard<Mutex> guard(m_Lock);

    size_t extent = m_Extent;
    if(extentOverride)
        extent = extentOverride;

    // Verify that we're not trying to memory map an empty file
    /// \todo It's ok to do so if write is enabled!
    if(extent <= 1)
        return false;

    if (!pProcess)
        pProcess = Processor::information().getCurrentThread()->getParent();

    if (address == 0)
    {
        if (!pProcess->getSpaceAllocator().allocate(extent+PhysicalMemoryManager::getPageSize(), address))
            return false;
        if (address & (PhysicalMemoryManager::getPageSize()-1))
        {
            address = (address + PhysicalMemoryManager::getPageSize()) &
                ~(PhysicalMemoryManager::getPageSize()-1);
        }
    }
    else
    {
        if (!pProcess->getSpaceAllocator().allocateSpecific(address, extent))
            return false;
    }

    NOTICE("MemoryMappedFile: " << Hex << address << " -> " << (address+extent) << " (pid " << pProcess->getId() << ")");

    // Create a spinlock as an easy way of disabling interrupts.
    Spinlock spinlock;
    spinlock.acquire();

    VirtualAddressSpace *va = pProcess->getAddressSpace();

    VirtualAddressSpace &oldva = Processor::information().getVirtualAddressSpace();

    if (&oldva != va)
        Processor::switchAddressSpace(*va);

    // Add all the V->P mappings we currently posess.
    for (Tree<uintptr_t,uintptr_t>::Iterator it = m_Mappings.begin();
         it != m_Mappings.end();
         it++)
    {
        uintptr_t v = it.key() + address;
        uintptr_t p = it.value();

        // Extent overridden means don't map the entire file in...
        if(it.key() >= extent) {
            break;
        }

        if (!va->map(p, reinterpret_cast<void*>(v), VirtualAddressSpace::Execute | VirtualAddressSpace::Write))
        {
            WARNING("MemoryMappedFile: map() failed at " << v);
            return false;
        }
    }

    if (&oldva != va)
        Processor::switchAddressSpace(oldva);

    spinlock.release();

    return true;
}

void MemoryMappedFile::unload(uintptr_t address)
{
    LockGuard<Mutex> guard(m_Lock);

    // Create a spinlock as an easy way of disabling interrupts.
    VirtualAddressSpace &va = Processor::information().getVirtualAddressSpace();

    // Remove all the V->P mappings we currently posess.
    for (Tree<uintptr_t,uintptr_t>::Iterator it = m_Mappings.begin();
         it != m_Mappings.end();
         it++)
    {
        uintptr_t v = it.key() + address;

        if (va.isMapped(reinterpret_cast<void*>(v)))
        {
            uintptr_t p;
            size_t flags;
            va.getMapping(reinterpret_cast<void*>(v), p, flags);

            va.unmap(reinterpret_cast<void*>(v));

            // If we forked and copied this page, we want to delete the second copy.
            // So, if the physical mapping is not what we have on record, free it.
            if (p != it.value())
                PhysicalMemoryManager::instance().freePage(p);
        }
    }
}

void MemoryMappedFile::trap(uintptr_t address, uintptr_t offset, uintptr_t fileoffset)
{
    LockGuard<Mutex> guard(m_Lock);

    //NOTICE_NOLOCK("trap at " << address << "[" << offset << ", " << fileoffset << "]");

    // Quick sanity check...
    if (address-offset > m_Extent)
    {
        FATAL_NOLOCK("MemoryMappedFile: trap called with invalid address: " << Hex << address);
        return;
    }

    VirtualAddressSpace &va = Processor::information().getVirtualAddressSpace();

    uintptr_t v = address & ~(PhysicalMemoryManager::getPageSize()-1);

    // Mapped?
    if(va.isMapped(reinterpret_cast<void *>(v)))
    {
        FATAL_NOLOCK("MemoryMappedFile: trap() on an already-mapped address");
        return;
    }

    // Allocate a physical page to store the result.
    physical_uintptr_t p = PhysicalMemoryManager::instance().allocatePage();

    // Map it into the address space.
    if (!va.map(p, reinterpret_cast<void *>(v), VirtualAddressSpace::Write | VirtualAddressSpace::Execute))
    {
        FATAL_NOLOCK("MemoryMappedFile: map() failed in trap()");
        return;
    }

    // Add the mapping to our list.
    uintptr_t readloc = (v - offset) + fileoffset;
    //NOTICE("readloc: " << readloc);
    m_Mappings.insert(readloc, p);

    // We can't just read directly into the new page, as the ATA driver
    // among others use thread-based IPC, which means they run in the
    // kernel address space, not ours. So we must create a buffer and
    // memcpy.
    uint8_t *buffer = new uint8_t[PhysicalMemoryManager::getPageSize()];

    if (m_pFile->read(readloc, PhysicalMemoryManager::getPageSize(),
                      reinterpret_cast<uintptr_t>(buffer)) == 0)
    {
        WARNING_NOLOCK("MemoryMappedFile: read() failed in trap()");
        WARNING_NOLOCK("File is " << m_pFile->getName() << ", offset was " << readloc << ", reading a page.");
        // Non-fatal, continue.
    }

    memcpy(reinterpret_cast<uint8_t*>(v), buffer, PhysicalMemoryManager::getPageSize());

    delete [] buffer;

    // Now that the file is read and memory written, change the mapping
    // to read only.
    // va.setFlags(reinterpret_cast<void*>(v), 0);
}

MemoryMappedFileManager::MemoryMappedFileManager() :
    m_MmFileLists(), m_Cache(), m_CacheLock()
{
    PageFaultHandler::instance().registerHandler(this);
}

MemoryMappedFileManager::~MemoryMappedFileManager()
{
}

MemoryMappedFile *MemoryMappedFileManager::map(File *pFile, uintptr_t &address, size_t sizeOverride, size_t offset, bool shared)
{
    MemoryMappedFile *pMmFile = 0;
    if(shared)
    {
        m_CacheLock.acquire();

        // Attempt to find an existing MemoryMappedFile* in the cache.
        pMmFile = m_Cache.lookup(pFile);
        if (pMmFile)
        {
            // File existed in cache - check if the file has been changed
            // since it was put in the cache.
            if (false /*pFile->hasBeenTouched()*/)
            {
                pMmFile->markForDeletion();
                m_Cache.remove(pFile);
                pMmFile = 0;
            }
        }

        // At this point we could have a null file because (a) the initial cache lookup failed
        // or (b) the file was out of date.
        if (!pMmFile)
        {
            pMmFile = new MemoryMappedFile(pFile);
            m_Cache.insert(pFile, pMmFile);
        }

        m_CacheLock.release();
    }
    else
    {
        pMmFile = new MemoryMappedFile(pFile);
    }

    if(sizeOverride == 0)
        sizeOverride = pMmFile->getExtent();

    // Now we know that pMmFile is valid, load it into our address space.
    if (!pMmFile->load(address, 0, sizeOverride))
    {
        ERROR_NOLOCK("MemoryMappedFile: load failed in map()");
        return 0;
    }

    pMmFile->increaseRefCount();

    VirtualAddressSpace &va = Processor::information().getVirtualAddressSpace();

    /// \todo This operation should be atomic with respect to threads.

    // Add to the MmFileList for this VA space (if it exists).
    MmFileList *pMmFileList = m_MmFileLists.lookup(&va);
    if (!pMmFileList)
    {
        pMmFileList = new MmFileList();
        m_MmFileLists.insert(&va, pMmFileList);
    }

    MmFile *_pMmFile = new MmFile(address, sizeOverride, offset, pMmFile);
    pMmFileList->pushBack(_pMmFile);

    // Success.
    return pMmFile;
}

void MemoryMappedFileManager::unmap(MemoryMappedFile *pMmFile)
{
    LockGuard<Mutex> guard(m_CacheLock);

    VirtualAddressSpace &va = Processor::information().getVirtualAddressSpace();

    MmFileList *pMmFileList = m_MmFileLists.lookup(&va);
    if (!pMmFileList) return;

    for (List<MmFile*>::Iterator it = pMmFileList->begin();
         it != pMmFileList->end();
         it++)
    {
        if ( (*it)->file == pMmFile )
        {
            (*it)->file->unload( (*it)->offset );
            if ((*it)->file->decreaseRefCount())
            {
                delete (*it)->file;
                m_Cache.remove( (*it)->file->getFile() );
            }
            delete *it;
            pMmFileList->erase(it);
            break;
        }
    }

    if (pMmFileList->count() == 0)
    {
        delete pMmFileList;
        m_MmFileLists.remove(&va);
    }
}

void MemoryMappedFileManager::clone(Process *pProcess)
{
    LockGuard<Mutex> guard(m_CacheLock);

    VirtualAddressSpace &va = Processor::information().getVirtualAddressSpace();

    VirtualAddressSpace *pOtherVa = pProcess->getAddressSpace();

    MmFileList *pMmFileList = m_MmFileLists.lookup(&va);
    if (!pMmFileList) return;

    MmFileList *pMmFileList2 = m_MmFileLists.lookup(pOtherVa);
    if (!pMmFileList2)
    {
        pMmFileList2 = new MmFileList();
        m_MmFileLists.insert(pOtherVa, pMmFileList2);
    }

    for (List<MmFile*>::Iterator it = pMmFileList->begin();
         it != pMmFileList->end();
         it++)
    {
        MmFile *pMmFile = new MmFile( (*it)->offset, (*it)->size, (*it)->fileoffset, (*it)->file );
        pMmFileList2->pushBack(pMmFile);

        (*it)->file->increaseRefCount();
    }
}

void MemoryMappedFileManager::unmapAll()
{
    LockGuard<Mutex> guard(m_CacheLock);

    VirtualAddressSpace &va = Processor::information().getVirtualAddressSpace();

    MmFileList *pMmFileList = m_MmFileLists.lookup(&va);
    if (!pMmFileList) return;

    for (List<MmFile*>::Iterator it = pMmFileList->begin();
         it != pMmFileList->end();
         it = pMmFileList->begin())
    {
        (*it)->file->unload( (*it)->offset );
        if ((*it)->file->decreaseRefCount())
        {
            m_Cache.remove( (*it)->file->getFile() );
            delete (*it)->file;
        }
        delete *it;
        it = pMmFileList->erase(it);
    }

    delete pMmFileList;
    m_MmFileLists.remove(&va);
}

bool MemoryMappedFileManager::trap(uintptr_t address, bool bIsWrite)
{
    //NOTICE_NOLOCK("Trap start: " << address << ", pid:tid " << Processor::information().getCurrentThread()->getParent()->getId() <<":" << Processor::information().getCurrentThread()->getId());
    /// \todo Handle read-write maps.
    if (bIsWrite)
    {
        // return false;
    }

    VirtualAddressSpace &va = Processor::information().getVirtualAddressSpace();

    m_CacheLock.acquire();

    MmFileList *pMmFileList = m_MmFileLists.lookup(&va);
    if (!pMmFileList)
    {
        m_CacheLock.release();
        return false;
    }


    for (List<MmFile*>::Iterator it = pMmFileList->begin();
         it != pMmFileList->end();
         it++)
    {
        MmFile *pMmFile = *it;
        if ( (address >= pMmFile->offset) && (address < pMmFile->offset+pMmFile->size) )
        {
            m_CacheLock.release();
            pMmFile->file->trap(address, pMmFile->offset, pMmFile->fileoffset);

//            NOTICE_NOLOCK("Trap end: " << address << ", pid:tid " << Processor::information().getCurrentThread()->getParent()->getId() <<":" << Processor::information().getCurrentThread()->getId());
            return true;
        }
    }
    m_CacheLock.release();

//    NOTICE_NOLOCK("Trap end (false): " << address << ", pid:tid " << Processor::information().getCurrentThread()->getParent()->getId() <<":" << Processor::information().getCurrentThread()->getId());

    return false;
}
