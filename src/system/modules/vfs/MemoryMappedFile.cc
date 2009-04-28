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

MemoryMappedFile::MemoryMappedFile(File *pFile) :
    m_pFile(pFile), m_Mappings(), m_bMarkedForDeletion(false),
    m_Extent(pFile->getSize()), m_RefCount(0)
{
    if (m_Extent & ~(PhysicalMemoryManager::getPageSize()-1))
    {
        m_Extent = (m_Extent + PhysicalMemoryManager::getPageSize()) &
                        ~(PhysicalMemoryManager::getPageSize()-1);
    }
}

MemoryMappedFile::~MemoryMappedFile()
{
}

bool MemoryMappedFile::load(uintptr_t &address, Process *pProcess)
{
    if (!pProcess)
        pProcess = Processor::information().getCurrentThread()->getParent();

    if (address == 0)
    {
        if (!pProcess->getSpaceAllocator().allocate(m_Extent+PhysicalMemoryManager::getPageSize(), address))
            return false;
        if (address & ~(PhysicalMemoryManager::getPageSize()-1))
        {
            address = (address + PhysicalMemoryManager::getPageSize()) &
                ~(PhysicalMemoryManager::getPageSize()-1);
        }
    }
    else
    {
        if (!pProcess->getSpaceAllocator().allocateSpecific(address, m_Extent))
            return false;
    }

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
        uintptr_t v = reinterpret_cast<uintptr_t>(it.key()) + address;
        uintptr_t p = reinterpret_cast<uintptr_t>(it.value());

        if (!va->map(p, reinterpret_cast<void*>(v), VirtualAddressSpace::Execute))
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
    // Create a spinlock as an easy way of disabling interrupts.
    Spinlock spinlock;
    spinlock.acquire();

    VirtualAddressSpace &va = Processor::information().getVirtualAddressSpace();
    
    // Remove all the V->P mappings we currently posess.
    for (Tree<uintptr_t,uintptr_t>::Iterator it = m_Mappings.begin();
         it != m_Mappings.end();
         it++)
    {
        uintptr_t v = reinterpret_cast<uintptr_t>(it.key()) + address;

        if (va.isMapped(reinterpret_cast<void*>(v)))
        {
            uintptr_t p;
            size_t flags;
            va.getMapping(reinterpret_cast<void*>(v), p, flags);

            va.unmap(reinterpret_cast<void*>(v));

            // If we forked and copied this page, we want to delete the second copy.
            // So, if the physical mapping is not what we have on record, free it.
            if (p != reinterpret_cast<uintptr_t>(it.value()))
                PhysicalMemoryManager::instance().freePage(p);
        }
    }

    spinlock.release();
}

void MemoryMappedFile::trap(uintptr_t address, uintptr_t offset)
{
    // Quick sanity check...
    if (address-offset > m_Extent)
    {
        FATAL("MemoryMappedFile: trap called with invalid address: " << Hex << address);
        return;
    }

    VirtualAddressSpace &va = Processor::information().getVirtualAddressSpace();

    uintptr_t v = address & ~(PhysicalMemoryManager::getPageSize()-1);

    // Allocate a physical page to store the result.
    physical_uintptr_t p = PhysicalMemoryManager::instance().allocatePage();

    // Map it into the address space.
    if (!va.map(p, reinterpret_cast<void *>(v), VirtualAddressSpace::Write | VirtualAddressSpace::Execute))
    {
        FATAL("MemoryMappedFile: map() failed in trap()");
        return;
    }

    // Add the mapping to our list.
    m_Mappings.insert(v-offset, p);

    // We can't just read directly into the new page, as the ATA driver
    // among others use thread-based IPC, which means they run in the
    // kernel address space, not ours. So we must create a buffer and
    // memcpy.
    uint8_t *buffer = new uint8_t[PhysicalMemoryManager::getPageSize()];

    if (m_pFile->read(v-offset, PhysicalMemoryManager::getPageSize(),
        reinterpret_cast<uintptr_t>(buffer)) == 0)
    {
        WARNING("MemoryMappedFile: read() failed in trap()");
        // Non-fatal, continue.
    }

    memcpy(reinterpret_cast<uint8_t*>(v), buffer, PhysicalMemoryManager::getPageSize());

    delete [] buffer;

    // Now that the file is read and memory written, change the mapping
    // to read only.
    va.setFlags(reinterpret_cast<void*>(v), 0);
}

MemoryMappedFileManager::MemoryMappedFileManager() :
    m_MmFileLists(), m_Cache(), m_CacheLock()
{
    PageFaultHandler::instance().registerHandler(this);
}

MemoryMappedFileManager::~MemoryMappedFileManager()
{
}

MemoryMappedFile *MemoryMappedFileManager::map(File *pFile, uintptr_t &address)
{
    LockGuard<Mutex> guard(m_CacheLock);

    // Attempt to find an existing MemoryMappedFile* in the cache.
    MemoryMappedFile *pMmFile = m_Cache.lookup(pFile);

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

    // Now we know that pMmFile is valid, load it into our address space.
    address = 0;
    if (!pMmFile->load(address))
    {
        ERROR("MemoryMappedFile: load failed in map()");
        return 0;
    }

    pMmFile->increaseRefCount();

    VirtualAddressSpace &va = Processor::information().getVirtualAddressSpace();

    // Add to the MmFileList for this VA space (if it exists).
    MmFileList *pMmFileList = m_MmFileLists.lookup(&va);
    if (!pMmFileList)
    {
        pMmFileList = new MmFileList();
        m_MmFileLists.insert(&va, pMmFileList);
    }

    MmFile *_pMmFile = new MmFile(address, pMmFile->getExtent(), pMmFile);
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
            (*it)->file->decreaseRefCount();
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
        MmFile *pMmFile = new MmFile( (*it)->offset, (*it)->size, (*it)->file );
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
        (*it)->file->decreaseRefCount();
        delete *it;
        pMmFileList->erase(it);
    }

    delete pMmFileList;
    m_MmFileLists.remove(&va);
}

bool MemoryMappedFileManager::trap(uintptr_t address, bool bIsWrite)
{
    LockGuard<Mutex> guard(m_CacheLock);

    /// \todo Handle read-write maps.
    if (bIsWrite) return false;

    VirtualAddressSpace &va = Processor::information().getVirtualAddressSpace();

    MmFileList *pMmFileList = m_MmFileLists.lookup(&va);
    if (!pMmFileList) return false;

    for (List<MmFile*>::Iterator it = pMmFileList->begin();
         it != pMmFileList->end();
         it++)
    {
        MmFile *pMmFile = *it;
        if ( (address >= pMmFile->offset) && (address < pMmFile->offset+pMmFile->size) )
        {
            pMmFile->file->trap(address, pMmFile->offset);
            return true;
        }
    }
    
    return false;
}
