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

bool MemoryMappedFile::load(uintptr_t &address)
{
    Process *pProcess = Processor::information().getCurrentThread()->getParent();

    if (address == 0)
    {
        if (!pProcess->getSpaceAllocator()->allocate(m_Extent, address))
            return false;
    }
    else
    {
        if (!pProcess->getSpaceAllocator()->allocateSpecific(address, m_Extent))
            return false;
    }

    VirtualAddressSpace &va = Processor::information().getVirtualAddressSpace();
    
    // Add all the V->P mappings we currently posess.
    for (Tree<uintptr_t,uintptr_t>::Iterator it = m_Mappings.begin();
         it != m_Mappings.end();
         it++)
    {
        uintptr_t v = reinterpret_cast<uintptr_t>(it.key()) + address;
        uintptr_t p = reinterpret_cast<uintptr_t>(it.value());

        if (!va.map(p, reinterpret_cast<void*>(v), 0))
        {
            WARNING("MemoryMappedFile: map() failed!");
            return false;
        }
    }

    return true;
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
    if (!va.map(p, reinterpret_cast<void *>(v), VirtualAddressSpace::Write))
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
    VirtualAddressSpace &va = Processor::information().getVirtualAddressSpace();

    MmFileList
}

void MemoryMappedFileManager::clone(Process *pProcess)
{
}

void MemoryMappedFileManager::unmapAll()
{
}

bool MemoryMappedFileManager::trap(uintptr_t address, bool bIsWrite)
{
}
