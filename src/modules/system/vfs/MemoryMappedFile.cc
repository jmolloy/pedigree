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

#include <utilities/assert.h>

MemoryMapManager MemoryMapManager::m_Instance;

physical_uintptr_t AnonymousMemoryMap::m_Zero = 0;

/// \todo number of CPUs
static char g_TrapPage[256][4096] __attribute__((aligned(4096))) = {0};

// #define DEBUG_MMOBJECTS

MemoryMappedObject::~MemoryMappedObject()
{
}

AnonymousMemoryMap::AnonymousMemoryMap(uintptr_t address, size_t length) :
    MemoryMappedObject(address, true, length), m_Mappings()
{
    VirtualAddressSpace &va = Processor::information().getVirtualAddressSpace();

    if(m_Zero == 0)
    {
        m_Zero = PhysicalMemoryManager::instance().allocatePage();
        /// \todo Map in the zero page, zero it.

        va.map(m_Zero, reinterpret_cast<void *>(address), VirtualAddressSpace::Write);
        memset(reinterpret_cast<void *>(address), 0, PhysicalMemoryManager::getPageSize());
        va.unmap(reinterpret_cast<void *>(address));
    }

    // Because mapping in an anonymous mapping does not have the overhead that
    // mapping in a file does (ie, pages etc), we can do so upfront. Writes
    // will still cause an immediate trap.
    for(size_t i = 0; i < length; i += PhysicalMemoryManager::instance().getPageSize())
    {
        void *v = reinterpret_cast<void *>(address + i);
        if(va.isMapped(v))
            va.unmap(v); /// \bug Possible leak: assuming this is overriding a MemoryMappedFile...

        va.map(m_Zero, v, VirtualAddressSpace::Shared);
        PhysicalMemoryManager::instance().pin(m_Zero);
    }
}

MemoryMappedObject *AnonymousMemoryMap::clone()
{
    AnonymousMemoryMap *pResult = new AnonymousMemoryMap(m_Address, m_Length);
    pResult->m_Mappings = m_Mappings;
    return pResult;
}

void AnonymousMemoryMap::unmap()
{
#ifdef DEBUG_MMOBJECTS
    NOTICE("AnonymousMemoryMap::unmap()");
#endif

    VirtualAddressSpace &va = Processor::information().getVirtualAddressSpace();

    for(List<void *>::Iterator it = m_Mappings.begin();
        it != m_Mappings.end();
        ++it)
    {
        void *v = *it;
        if(va.isMapped(v))
        {
            size_t flags;
            physical_uintptr_t phys;

            va.getMapping(v, phys, flags);

            // Clean up. Shared read-only zero page will only have its refcount
            // decreased by this - it will not hit zero.
            va.unmap(v);
            PhysicalMemoryManager::instance().freePage(phys);
        }
    }

    m_Mappings.clear();
}

bool AnonymousMemoryMap::trap(uintptr_t address, bool bWrite)
{
#ifdef DEBUG_MMOBJECTS
    NOTICE("AnonymousMemoryMap::trap(" << address << ", " << bWrite << ")");
#endif

    size_t pageSz = PhysicalMemoryManager::getPageSize();
    VirtualAddressSpace &va = Processor::information().getVirtualAddressSpace();

    // Page-align the trap address
    address = address & ~(pageSz - 1);

    if(!bWrite)
    {
        PhysicalMemoryManager::instance().pin(m_Zero);
        va.map(m_Zero, reinterpret_cast<void *>(address), VirtualAddressSpace::Shared);

        m_Mappings.pushBack(reinterpret_cast<void *>(address));
    }
    else
    {
        physical_uintptr_t newPage = PhysicalMemoryManager::instance().allocatePage();

        if(va.isMapped(reinterpret_cast<void *>(address)))
        {
            va.unmap(reinterpret_cast<void *>(address));

            // Drop the refcount on the zero page.
            PhysicalMemoryManager::instance().freePage(m_Zero);

            m_Mappings.pushBack(reinterpret_cast<void *>(address));
        }
        va.map(newPage, reinterpret_cast<void *>(address), VirtualAddressSpace::Write);

        // "Copy" on write... but not really :)
        memset(reinterpret_cast<void *>(address), 0, PhysicalMemoryManager::getPageSize());
    }

    return true;
}

MemoryMappedFile::MemoryMappedFile(uintptr_t address, size_t length, size_t offset, File *backing, bool bCopyOnWrite) :
    MemoryMappedObject(address, bCopyOnWrite, length), m_pBacking(backing), m_Offset(offset), m_Mappings()
{
    assert(m_pBacking);
}

MemoryMappedObject *MemoryMappedFile::clone()
{
    MemoryMappedFile *pResult = new MemoryMappedFile(m_Address, m_Length, m_Offset, m_pBacking, m_bCopyOnWrite);
    pResult->m_Mappings = m_Mappings;

    for(Tree<uintptr_t, uintptr_t>::Iterator it = m_Mappings.begin();
        it != m_Mappings.end();
        ++it)
    {
        // Bump reference count on backing file page if needed.
        size_t fileOffset = (it.key() - m_Address) + m_Offset;
        if(it.value() == static_cast<physical_uintptr_t>(~0UL))
            m_pBacking->getPhysicalPage(fileOffset);
    }

    return pResult;
}

void MemoryMappedFile::unmap()
{
#ifdef DEBUG_MMOBJECTS
    NOTICE("MemoryMappedFile::unmap()");
#endif

    VirtualAddressSpace &va = Processor::information().getVirtualAddressSpace();

    if(!m_Mappings.count())
        return;

    for(Tree<uintptr_t, uintptr_t>::Iterator it = m_Mappings.begin();
        it != m_Mappings.end();
        ++it)
    {
        void *v = reinterpret_cast<void *>(it.key());
        if(!va.isMapped(v))
            break; // Already unmapped...

        size_t flags = 0;
        physical_uintptr_t phys = 0;
        va.getMapping(v, phys, flags);
        va.unmap(v);

        physical_uintptr_t p = it.value();
        if(p == static_cast<physical_uintptr_t>(~0UL))
            m_pBacking->returnPhysicalPage((it.key() - m_Address) + m_Offset);
        else
            PhysicalMemoryManager::instance().freePage(phys);
    }

    m_Mappings.clear();
}

static physical_uintptr_t getBackingPage(File *pBacking, size_t fileOffset)
{
    size_t pageSz = PhysicalMemoryManager::getPageSize();

    physical_uintptr_t phys = pBacking->getPhysicalPage(fileOffset);
    if(phys == static_cast<physical_uintptr_t>(~0UL))
    {
        // No page found, trigger a read to fix that!
        pBacking->read(fileOffset, pageSz, 0);
        phys = pBacking->getPhysicalPage(fileOffset);
        if(phys == static_cast<physical_uintptr_t>(~0UL))
            ERROR("*** Could not manage to get a physical page for a MemoryMappedFile (" << pBacking->getName() << ")!");
    }

    return phys;
}

bool MemoryMappedFile::trap(uintptr_t address, bool bWrite)
{
#ifdef DEBUG_MMOBJECTS
    NOTICE("MemoryMappedFile::trap(" << address << ", " << bWrite << ")");
#endif

    VirtualAddressSpace &va = Processor::information().getVirtualAddressSpace();
    size_t pageSz = PhysicalMemoryManager::getPageSize();

    // Page-align the trap address
    address = address & ~(pageSz - 1);
    size_t mappingOffset = (address - m_Address);
    size_t fileOffset = m_Offset + mappingOffset;

    bool bWillEof = (mappingOffset + pageSz) > m_Length;
    bool bShouldCopy = m_bCopyOnWrite && (bWillEof || bWrite);

    if(!bShouldCopy)
    {
        physical_uintptr_t phys = getBackingPage(m_pBacking, fileOffset);
        if(phys == static_cast<physical_uintptr_t>(~0UL))
            return false; // Fail.

        size_t flags = VirtualAddressSpace::Shared;
        if(!m_bCopyOnWrite)
            flags |= VirtualAddressSpace::Write;

        va.map(phys, reinterpret_cast<void *>(address), flags);

        m_Mappings.insert(address, ~0);
    }
    else
    {
        m_Mappings.remove(address);

        // Determine the current physical target of this page.
        size_t flags;
        physical_uintptr_t phys;
        if(va.isMapped(reinterpret_cast<void *>(address)))
            va.getMapping(reinterpret_cast<void *>(address), phys, flags);
        else
        {
            // Not yet mapped, bring in the page for a copy.
            phys = getBackingPage(m_pBacking, fileOffset);
            if(phys == static_cast<physical_uintptr_t>(~0UL))
                return false; // Fail.
        }

        // Remap address.
        physical_uintptr_t newPhys = PhysicalMemoryManager::instance().allocatePage();
        if(va.isMapped(reinterpret_cast<void *>(address)))
            va.unmap(reinterpret_cast<void *>(address));
        va.map(newPhys, reinterpret_cast<void *>(address), VirtualAddressSpace::Write);

        // Do not interrupt - we're about to smash the temporary page for this CPU.
        Spinlock lock; LockGuard<Spinlock> guard(lock);

        // Get some space in the virtual address space to prepare this copy.
        /// \todo CPU number here.
        uintptr_t tempVirt = reinterpret_cast<uintptr_t>(g_TrapPage[0]);
        if(va.isMapped(reinterpret_cast<void*>(tempVirt)))
            va.unmap(reinterpret_cast<void *>(tempVirt));
        va.map(phys, reinterpret_cast<void *>(tempVirt), VirtualAddressSpace::KernelMode);

        // Perform the copy.
        memcpy(reinterpret_cast<uint8_t*>(address), reinterpret_cast<void *>(tempVirt), pageSz);

        // Remove the temporary mapping.
        va.unmap(reinterpret_cast<void *>(tempVirt));

        // Handle EOF in the middle of the page we just copied.
        if((mappingOffset + pageSz) > m_Length)
        {
            size_t offset = m_Length - mappingOffset;
            memset(reinterpret_cast<void *>(address + offset), 0, pageSz - offset - 1);
        }

        m_pBacking->returnPhysicalPage(fileOffset);
        m_Mappings.insert(address, newPhys);
    }

    return true;
}

MemoryMapManager::MemoryMapManager() :
    m_MmObjectLists(), m_Lock()
{
    PageFaultHandler::instance().registerHandler(this);
}

MemoryMapManager::~MemoryMapManager()
{
}

MemoryMappedObject *MemoryMapManager::mapFile(File *pFile, uintptr_t &address, size_t length, size_t offset, bool bCopyOnWrite)
{
    VirtualAddressSpace &va = Processor::information().getVirtualAddressSpace();
    size_t pageSz = PhysicalMemoryManager::getPageSize();

    // Make sure the size is page aligned. (we'll fill any space that is past
    // the end of the extent with zeroes).
    size_t actualLength = length;
    if(length & (pageSz - 1))
    {
        length += 0x1000;
        length &= ~(pageSz - 1);
    }

    if(!sanitiseAddress(address, length))
        return 0;

    MemoryMappedFile *pMappedFile = new MemoryMappedFile(
        address, actualLength, offset, pFile, bCopyOnWrite);

    {
        // This operation must appear atomic.
        LockGuard<Mutex> guard(m_Lock);

        MmObjectList *pMmObjectList = m_MmObjectLists.lookup(&va);
        if (!pMmObjectList)
        {
            pMmObjectList = new MmObjectList();
            m_MmObjectLists.insert(&va, pMmObjectList);
        }

        pMmObjectList->pushBack(pMappedFile);
    }

    // Success.
    return pMappedFile;
}

MemoryMappedObject *MemoryMapManager::mapAnon(uintptr_t &address, size_t length)
{
    VirtualAddressSpace &va = Processor::information().getVirtualAddressSpace();
    size_t pageSz = PhysicalMemoryManager::getPageSize();

    // Make sure the size is page aligned. (we'll fill any space that is past
    // the end of the extent with zeroes).
    if(length & (pageSz - 1))
    {
        length += 0x1000;
        length &= ~(pageSz - 1);
    }

    if(!sanitiseAddress(address, length))
        return 0;

    AnonymousMemoryMap *pMap = new AnonymousMemoryMap(address, length);

    {
        // This operation must appear atomic.
        LockGuard<Mutex> guard(m_Lock);

        MmObjectList *pMmObjectList = m_MmObjectLists.lookup(&va);
        if (!pMmObjectList)
        {
            pMmObjectList = new MmObjectList();
            m_MmObjectLists.insert(&va, pMmObjectList);
        }

        pMmObjectList->pushBack(pMap);
    }

    // Success.
    return pMap;
}

void MemoryMapManager::clone(Process *pProcess)
{
    LockGuard<Mutex> guard(m_Lock);

    VirtualAddressSpace &va = Processor::information().getVirtualAddressSpace();
    VirtualAddressSpace *pOtherVa = pProcess->getAddressSpace();

    MmObjectList *pMmObjectList = m_MmObjectLists.lookup(&va);
    if (!pMmObjectList) return;

    MmObjectList *pMmObjectList2 = m_MmObjectLists.lookup(pOtherVa);
    if (!pMmObjectList2)
    {
        pMmObjectList2 = new MmObjectList();
        m_MmObjectLists.insert(pOtherVa, pMmObjectList2);
    }

    for (List<MemoryMappedObject*>::Iterator it = pMmObjectList->begin();
         it != pMmObjectList->end();
         it++)
    {
        MemoryMappedObject *obj = *it;
        MemoryMappedObject *pNewObject = obj->clone();
        pMmObjectList2->pushBack(pNewObject);
    }
}

void MemoryMapManager::unmap(MemoryMappedObject* pObj)
{
    LockGuard<Mutex> guard(m_Lock);

    VirtualAddressSpace &va = Processor::information().getVirtualAddressSpace();

    MmObjectList *pMmObjectList = m_MmObjectLists.lookup(&va);
    if (!pMmObjectList) return;

    for (List<MemoryMappedObject*>::Iterator it = pMmObjectList->begin();
         it != pMmObjectList->end();
         ++it)
    {
        if((*it) != pObj)
            continue;

        (*it)->unmap();
        delete (*it);

        pMmObjectList->erase(it);
        return;
    }
}

void MemoryMapManager::unmapAll()
{
    LockGuard<Mutex> guard(m_Lock);

    VirtualAddressSpace &va = Processor::information().getVirtualAddressSpace();

    MmObjectList *pMmObjectList = m_MmObjectLists.lookup(&va);
    if (!pMmObjectList) return;

    for (List<MemoryMappedObject*>::Iterator it = pMmObjectList->begin();
         it != pMmObjectList->end();
         it = pMmObjectList->begin())
    {
        (*it)->unmap();
        delete (*it);

        pMmObjectList->erase(it);
    }

    delete pMmObjectList;
    m_MmObjectLists.remove(&va);
}

bool MemoryMapManager::trap(uintptr_t address, bool bIsWrite)
{
#ifdef DEBUG_MMOBJECTS
    NOTICE_NOLOCK("Trap start: " << address << ", pid:tid " << Processor::information().getCurrentThread()->getParent()->getId() <<":" << Processor::information().getCurrentThread()->getId());
#endif

    VirtualAddressSpace &va = Processor::information().getVirtualAddressSpace();

    m_Lock.acquire();
#ifdef DEBUG_MMOBJECTS
    NOTICE_NOLOCK("trap: got lock");
#endif

    MmObjectList *pMmObjectList = m_MmObjectLists.lookup(&va);
    if (!pMmObjectList)
    {
        m_Lock.release();
        return false;
    }

#ifdef DEBUG_MMOBJECTS
    NOTICE_NOLOCK("trap: lookup complete " << reinterpret_cast<uintptr_t>(pMmObjectList));
#endif

    for (List<MemoryMappedObject*>::Iterator it = pMmObjectList->begin();
         it != pMmObjectList->end();
         it++)
    {
        MemoryMappedObject *pObject = *it;
#ifdef DEBUG_MMOBJECTS
        NOTICE_NOLOCK("mmobj=" << reinterpret_cast<uintptr_t>(pObject));
        if(!pObject)
        {
            NOTICE_NOLOCK("bad mmobj, should create a real #PF and backtrace");
            break;
        }
#endif

        if(pObject->matches(address))
        {
            m_Lock.release();
            return pObject->trap(address, bIsWrite);
        }
    }

#ifdef DEBUG_MMOBJECTS
    NOTICE_NOLOCK("trap: fell off end");
#endif
    m_Lock.release();

    return false;
}

bool MemoryMapManager::sanitiseAddress(uintptr_t &address, size_t length)
{
    Process *pProcess = Processor::information().getCurrentThread()->getParent();
    size_t pageSz = PhysicalMemoryManager::getPageSize();

    // Can we get some space for this mapping?
    if(address == 0)
    {
        if(!pProcess->getSpaceAllocator().allocate(length + pageSz, address))
            return false;

        if(address & (pageSz - 1))
        {
            address = (address + pageSz) & ~(pageSz - 1);
        }
    }
    else
    {
        // If this fails, we generally assume a reservation has been made.
        /// \todo rework APIs a lot.
        pProcess->getSpaceAllocator().allocateSpecific(address, length);
    }

    return true;
}
