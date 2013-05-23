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

/// \todo number of CPUs
static char g_TrapPage[256][4096] __attribute__((aligned(4096))) = {0};

MemoryMappedFile::MemoryMappedFile(File *pFile, size_t extentOverride, bool bShared) :
    m_pFile(pFile), m_Mappings(), m_bMarkedForDeletion(false),
    m_Extent(extentOverride ? extentOverride : pFile->getSize() + 1), m_RefCount(0), m_Lock(),
    m_bShared(bShared)
{
    if (m_Extent & ~(PhysicalMemoryManager::getPageSize()-1))
    {
        m_Extent = (m_Extent + PhysicalMemoryManager::getPageSize()) &
            ~(PhysicalMemoryManager::getPageSize()-1);
    }
}

MemoryMappedFile::MemoryMappedFile(size_t anonMapSize) :
    m_pFile(0), m_Mappings(), m_bMarkedForDeletion(false), m_Extent(anonMapSize),
    m_RefCount(1), m_Lock()
{
}

MemoryMappedFile::~MemoryMappedFile()
{
    LockGuard<Mutex> guard(m_Lock);

    /// \note You should call unload() before destroying a MemoryMappedFile.
    ///       as unload() can offset the keys of m_Mappings correctly.
}

bool MemoryMappedFile::load(uintptr_t &address, Process *pProcess, size_t extentOverride)
{
    LockGuard<Mutex> guard(m_Lock);

    size_t extent = m_Extent;
    if(extentOverride)
        m_Extent = extent = extentOverride;

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
        //if (!pProcess->getSpaceAllocator().allocateSpecific(address, extent))
        //    return false;

        // If this fails, we generally assume a reservation has been made.
        /// \todo rework APIs a lot.
        pProcess->getSpaceAllocator().allocateSpecific(address, extent);
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

        // Check for shared page.
        bool bMapWrite = m_bShared;
        if(p == static_cast<physical_uintptr_t>(~0UL))
        {
            p = m_pFile->getPhysicalPage(it.key());
        }
        else
        {
            bMapWrite = true;
        }

        // Check for cloned address space - which can and does break shared mmaps.
        // Need to remove the cloned page and replace it with the real mapping.
        if(va->isMapped(reinterpret_cast<void*>(v)))
        {
            size_t flags = 0;
            physical_uintptr_t phys = 0;
            va->getMapping(reinterpret_cast<void*>(v), phys, flags);

            if(phys == p)
            {
                continue;
            }

            // Cloned address space with new page. Kill it.
            va->unmap(reinterpret_cast<void*>(v));
            PhysicalMemoryManager::instance().freePage(phys);
        }

        if (!va->map(p, reinterpret_cast<void*>(v), VirtualAddressSpace::Execute | (bMapWrite ? VirtualAddressSpace::Write : 0)))
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

    VirtualAddressSpace &va = Processor::information().getVirtualAddressSpace();

    // Remove all the V->P mappings we currently posess.
    for (Tree<uintptr_t,uintptr_t>::Iterator it = m_Mappings.begin();
         it != m_Mappings.end();
         it++)
    {
        uintptr_t v = it.key() + address;

        // Check for shared page.
        bool bFreePages = (it.value() != static_cast<physical_uintptr_t>(~0UL));

        if (va.isMapped(reinterpret_cast<void*>(v)))
        {
            uintptr_t p;
            size_t flags;
            va.getMapping(reinterpret_cast<void*>(v), p, flags);

            va.unmap(reinterpret_cast<void*>(v));

            // If we forked and copied this page, we want to delete the second copy.
            // So, if the physical mapping is not what we have on record, free it.
            if (bFreePages && (p != static_cast<physical_uintptr_t>(~0UL)) && (p != it.value()))
                PhysicalMemoryManager::instance().freePage(p);
        }
    }
    m_Mappings.clear();
}

void MemoryMappedFile::trap(uintptr_t address, uintptr_t offset, uintptr_t fileoffset, bool bIsWrite)
{
    LockGuard<Mutex> guard(m_Lock);
    size_t pageSz = PhysicalMemoryManager::getPageSize();

    // NOTICE_NOLOCK("trap at " << address << "[" << offset << ", " << fileoffset << "] for " << m_pFile->getName());

    // Quick sanity check...
    /*
    if (address-offset > m_Extent)
    {
        FATAL_NOLOCK("MemoryMappedFile: trap called with invalid address: " << Hex << address);
        return;
    }
    */

    VirtualAddressSpace &va = Processor::information().getVirtualAddressSpace();

    uintptr_t v = address & ~(pageSz - 1);

    uintptr_t offsetIntoMap = (v - offset);
    uintptr_t alignedOffsetIntoMap = offsetIntoMap & ~(PhysicalMemoryManager::getPageSize() - 1);

    // Add the mapping to our list.
    uintptr_t readloc = offsetIntoMap + fileoffset;
    // NOTICE_NOLOCK("  -> readloc: " << readloc << " [v=" << v << ", offset=" << offset << ", extent=" << m_Extent << "]");

    bool bShouldCopy = (!m_bShared) && bIsWrite;
    if(!m_bShared)
    {
        // Will we need to memset?
        if((alignedOffsetIntoMap + pageSz) >= m_Extent)
        {
            // Yes. Always force a copy - even if just reading.
            bShouldCopy = true;
        }
    }

    // NOTICE_NOLOCK("  -> will " << (bShouldCopy ? "" : "not ") << "copy");

    // Mapped?
    if(va.isMapped(reinterpret_cast<void *>(v)))
    {
        // NOTICE_NOLOCK("  -> clearing existing mapping");
        if(bShouldCopy)
        {
            // Sane state to be in - just unmap the virtual (CoW)
            m_Mappings.remove(readloc);
            va.unmap(reinterpret_cast<void *>(v));
        }
        else
        {
            FATAL_NOLOCK("MemoryMappedFile: trap() on an already-mapped address");
            return;
        }
    }

    // Grab an existing, or allocate a new, physical page to store the result.
    bool bDoRead = false;
    physical_uintptr_t p = m_pFile->getPhysicalPage(readloc);
    if(p == static_cast<physical_uintptr_t>(~0UL))
    {
        // No page found. Read and try again.
        // NOTICE_NOLOCK("<trap falling back to a no-buffer read for " << m_pFile->getName() << ">");
        m_pFile->read(readloc, PhysicalMemoryManager::getPageSize(), 0);
        p = m_pFile->getPhysicalPage(readloc);
        if(p == static_cast<physical_uintptr_t>(~0UL))
        {
            WARNING_NOLOCK("MemoryMappedFile: read() didn't give us a physical address");
            return;
        }
    }

    // NOTICE_NOLOCK("  -> file page is p" << p);

    physical_uintptr_t mapPhys = p;
    if(bShouldCopy)
    {
        mapPhys = PhysicalMemoryManager::instance().allocatePage();
    }

    // Map the page into the address space.
    // NOTICE_NOLOCK("trap: " << v << " -> " << mapPhys << " for " << m_pFile->getName());
    if (!va.map(mapPhys, reinterpret_cast<void *>(v), ((bIsWrite || m_bShared) ? VirtualAddressSpace::Write : 0) | VirtualAddressSpace::Execute))
    {
        FATAL_NOLOCK("MemoryMappedFile: map() failed in trap()");
        return;
    }

    // We shouldn't ever free physical pages tied to real file data, but we
    // can happily free physical pages related to copied data.
    m_Mappings.insert(readloc, bShouldCopy ? mapPhys : static_cast<physical_uintptr_t>(~0UL));

    // Do we need to do a data copy (for non-shared)?
    if(bShouldCopy)
    {
        // Grab the current process.
        Process *pProcess = Processor::information().getCurrentThread()->getParent();

        // Get some space in the virtual address space to prepare this copy.
        /// \todo CPU number here.
        uintptr_t allocAddress = reinterpret_cast<uintptr_t>(g_TrapPage[0]);

        uintptr_t address = allocAddress;
        // NOTICE_NOLOCK("** trap: pid=" << pProcess->getId() << ", v=" << v << ", allocAddress=" << allocAddress << ", address=" << address);
        if(va.isMapped(reinterpret_cast<void*>(address)))
        {
            // Early startup, most likely.
            // Rip out the old mapping so we can override it.
            va.unmap(reinterpret_cast<void *>(address));
        }
        va.map(p, reinterpret_cast<void *>(address), VirtualAddressSpace::Write | VirtualAddressSpace::KernelMode);

        // Perform the copy.
        memcpy(reinterpret_cast<uint8_t*>(v), reinterpret_cast<void *>(address), pageSz);

        va.unmap(reinterpret_cast<void *>(address));

        // Fudge bytesRead to be logical.
        size_t bytesRead = pageSz;

        // Zero out the rest of the page if we hit EOF halfway through.
        // This is great for things like ELF which can have .bss shoved on the end
        // of .data, where mmap can't quite fix up the last bytes. When the early
        // bytes of .bss are accessed, we get paged in, and the beginning of .bss
        // gets zeroed nicely.
        // The rest of .bss can obviously be mapped to /dev/null or similar.
        if((alignedOffsetIntoMap + bytesRead) >= m_Extent)
        {
            uintptr_t bufferOffset = m_Extent - alignedOffsetIntoMap;
            if(UNLIKELY(bufferOffset >= pageSz))
            {
                WARNING_NOLOCK("MemoryMappedFile::trap - buffer offset larger than a page, not zeroing anything.");
            }
            else
            {
                NOTICE_NOLOCK("  -> zeroing " << (pageSz - bufferOffset) << " bytes (from " << bufferOffset << " onwards) @" << v << ".");
                memset(reinterpret_cast<uint8_t*>(v + bufferOffset), 0, pageSz - 1 - bufferOffset);
            }
        }
    }
    else
    {
        // NOTICE_NOLOCK("<trap only needed to map a physical page " << p << " for " << m_pFile->getName() << ">");
    }

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
            pMmFile = new MemoryMappedFile(pFile, 0, shared);
            m_Cache.insert(pFile, pMmFile);
        }

        m_CacheLock.release();
    }
    else
    {
        pMmFile = new MemoryMappedFile(pFile, 0, shared);
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

    // Make sure the size is page aligned. (we'll fill any space that is past
    // the end of the extent with zeroes).
    if(sizeOverride & (PhysicalMemoryManager::instance().getPageSize() - 1))
    {
        sizeOverride += 0x1000;
        sizeOverride &= ~(PhysicalMemoryManager::instance().getPageSize() - 1);
    }

    {
        // This operation must appear atomic.
        LockGuard<Mutex> guard(m_CacheLock);

        // Add to the MmFileList for this VA space (if it exists).
        MmFileList *pMmFileList = m_MmFileLists.lookup(&va);
        if (!pMmFileList)
        {
            pMmFileList = new MmFileList();
            m_MmFileLists.insert(&va, pMmFileList);
        }

        MmFile *_pMmFile = new MmFile(address, sizeOverride, offset, pMmFile);
        pMmFileList->pushBack(_pMmFile);
    }

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
        pMmFileList->erase(it);
    }

    delete pMmFileList;
    m_MmFileLists.remove(&va);
}

// #define MMFILE_DEBUG

bool MemoryMappedFileManager::trap(uintptr_t address, bool bIsWrite)
{
#ifdef MMFILE_DEBUG
    NOTICE_NOLOCK("Trap start: " << address << ", pid:tid " << Processor::information().getCurrentThread()->getParent()->getId() <<":" << Processor::information().getCurrentThread()->getId());
#endif

    /// \todo Handle read-write maps.
    if (bIsWrite)
    {
        // return false;
    }

    VirtualAddressSpace &va = Processor::information().getVirtualAddressSpace();

    m_CacheLock.acquire();
#ifdef MMFILE_DEBUG
    NOTICE_NOLOCK("trap: got lock");
#endif

    MmFileList *pMmFileList = m_MmFileLists.lookup(&va);
    if (!pMmFileList)
    {
        m_CacheLock.release();
        return false;
    }

#ifdef MMFILE_DEBUG
    NOTICE_NOLOCK("trap: lookup complete " << reinterpret_cast<uintptr_t>(pMmFileList));
#endif

    for (List<MmFile*>::Iterator it = pMmFileList->begin();
         it != pMmFileList->end();
         it++)
    {
        MmFile *pMmFile = *it;
#ifdef MMFILE_DEBUG
        NOTICE_NOLOCK("mmfile=" << reinterpret_cast<uintptr_t>(pMmFile));
        if(!pMmFile)
        {
            NOTICE_NOLOCK("bad mmfile, should create a real #PF and backtrace");
            break;
        }
#endif

        uintptr_t endAddress = pMmFile->offset + pMmFile->size;

        if ( (address >= pMmFile->offset) && (address < endAddress) )
        {
#ifdef MMFILE_DEBUG
            NOTICE_NOLOCK("trap: release lock B");
#endif
            m_CacheLock.release();
            pMmFile->file->trap(address, pMmFile->offset, pMmFile->fileoffset, bIsWrite);

#ifdef MMFILE_DEBUG
            NOTICE_NOLOCK("trap: completed for " << address);
#endif
//            NOTICE_NOLOCK("Trap end: " << address << ", pid:tid " << Processor::information().getCurrentThread()->getParent()->getId() <<":" << Processor::information().getCurrentThread()->getId());
            return true;
        }
    }

#ifdef MMFILE_DEBUG
    NOTICE_NOLOCK("trap: fell off end");
#endif
    m_CacheLock.release();

//    NOTICE_NOLOCK("Trap end (false): " << address << ", pid:tid " << Processor::information().getCurrentThread()->getParent()->getId() <<":" << Processor::information().getCurrentThread()->getId());

    return false;
}
