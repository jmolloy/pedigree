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
        PhysicalMemoryManager::instance().pin(m_Zero);

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

MemoryMappedObject *AnonymousMemoryMap::split(uintptr_t at)
{
    if(at < m_Address || at >= (m_Address + m_Length))
    {
        ERROR("AnonymousMemoryMap::split() given bad at parameter (at=" << at << ", address=" << m_Address << ", end=" << (m_Address + m_Length) << ")");
        return 0;
    }

    if(at == m_Address)
    {
        ERROR("AnonymousMemoryMap::split() misused, at == base address");
        return 0;
    }

    // Change our own object to fit in the new region.
    size_t oldLength = m_Length;
    m_Length = at - m_Address;

    // New object.
    AnonymousMemoryMap *pResult = new AnonymousMemoryMap(at, oldLength - m_Length);

    // Fix up mapping metadata.
    for(List<void *>::Iterator it = m_Mappings.begin();
        it != m_Mappings.end();
        ++it)
    {
        uintptr_t v = reinterpret_cast<uintptr_t>(*it);
        if(v >= at)
        {
            pResult->m_Mappings.pushBack(*it);
            it = m_Mappings.erase(it);
        }
    }

    return pResult;
}

bool AnonymousMemoryMap::remove(size_t length)
{
    VirtualAddressSpace &va = Processor::information().getVirtualAddressSpace();
    size_t pageSz = PhysicalMemoryManager::getPageSize();

    if(length & (pageSz - 1))
    {
        length += 0x1000;
        length &= ~(pageSz - 1);
    }

    if(length >= m_Length)
    {
        unmap();
        return true;
    }

    m_Address += length;
    m_Length -= length;

    // Remove any existing mappings in this range.
    for(List<void *>::Iterator it = m_Mappings.begin();
        it != m_Mappings.end();
        ++it)
    {
        uintptr_t virt = reinterpret_cast<uintptr_t>(*it);
        if(virt >= m_Address)
            break;

        void *v = *it;
        if(va.isMapped(v))
        {
            size_t flags;
            physical_uintptr_t phys;

            va.getMapping(v, phys, flags);

            va.unmap(v);
            PhysicalMemoryManager::instance().freePage(phys);
        }

        it = m_Mappings.erase(it);
    }

    return false;
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
        // Clean up existing page, if any.
        if(va.isMapped(reinterpret_cast<void *>(address)))
        {
            va.unmap(reinterpret_cast<void *>(address));

            // Drop the refcount on the zero page.
            PhysicalMemoryManager::instance().freePage(m_Zero);
        }
        else
        {
            // Write to unpaged - make sure we track this mapping.
            m_Mappings.pushBack(reinterpret_cast<void *>(address));
        }

        // "Copy" on write... but not really :)
        physical_uintptr_t newPage = PhysicalMemoryManager::instance().allocatePage();
        va.map(newPage, reinterpret_cast<void *>(address), VirtualAddressSpace::Write);
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

MemoryMappedObject *MemoryMappedFile::split(uintptr_t at)
{
    size_t pageSz = PhysicalMemoryManager::getPageSize();

    if(at < m_Address || at >= (m_Address + m_Length))
    {
        ERROR("MemoryMappedFile::split() given bad at parameter (at=" << at << ", address=" << m_Address << ", end=" << (m_Address + m_Length) << ")");
        return 0;
    }

    if(at == m_Address)
    {
        ERROR("MemoryMappedFile::split() misused, at == base address");
        return 0;
    }

    uintptr_t oldEnd = m_Address + m_Length;

    // Change our own object to fit in the new region.
    size_t oldLength = m_Length;
    m_Length = at - m_Address;

    // New object.
    MemoryMappedFile *pResult = new MemoryMappedFile(at, oldLength - m_Length, m_Offset + m_Length, m_pBacking, m_bCopyOnWrite);

    // Fix up mapping metadata.
    for(uintptr_t virt = at;
        virt < oldEnd;
        virt += pageSz)
    {
        physical_uintptr_t old = m_Mappings.lookup(virt);
        m_Mappings.remove(virt);

        pResult->m_Mappings.insert(virt, old);
    }

    return pResult;
}

bool MemoryMappedFile::remove(size_t length)
{
    VirtualAddressSpace &va = Processor::information().getVirtualAddressSpace();
    size_t pageSz = PhysicalMemoryManager::getPageSize();

    if(length & (pageSz - 1))
    {
        length += 0x1000;
        length &= ~(pageSz - 1);
    }

    if(length >= m_Length)
    {
        unmap();
        return true;
    }

    uintptr_t oldStart = m_Address;
    size_t oldOffset = m_Offset;

    m_Address += length;
    m_Offset += length;
    m_Length -= length;

    // Remove any existing mappings in this range.
    for(uintptr_t virt = oldStart;
        virt < m_Address;
        virt += pageSz)
    {
        void *v = reinterpret_cast<void *>(virt);
        if(va.isMapped(v))
        {
            size_t flags;
            physical_uintptr_t phys;

            va.getMapping(v, phys, flags);
            va.unmap(v);

            physical_uintptr_t p = m_Mappings.lookup(virt);
            if(p == static_cast<physical_uintptr_t>(~0UL))
            {
                size_t fileOffset = (virt - oldStart) + oldOffset;
                m_pBacking->returnPhysicalPage(fileOffset);
                m_pBacking->sync(fileOffset, true);
            }
            else
                PhysicalMemoryManager::instance().freePage(phys);
        }

        m_Mappings.remove(virt);
    }

    return false;
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

void MemoryMappedFile::sync(uintptr_t at, bool async)
{
    VirtualAddressSpace &va = Processor::information().getVirtualAddressSpace();
    size_t pageSz = PhysicalMemoryManager::getPageSize();

    if(at < m_Address || at >= (m_Address + m_Length))
    {
        ERROR("MemoryMappedFile::sync() given bad at parameter (at=" << at << ", address=" << m_Address << ", end=" << (m_Address + m_Length) << ")");
        return;
    }

    void *v = reinterpret_cast<void *>(at);
    if(va.isMapped(v))
    {
        size_t flags = 0;
        physical_uintptr_t phys = 0;
        va.getMapping(v, phys, flags);
        if((flags & VirtualAddressSpace::Write) == 0)
        {
            // Nothing to sync here! Page not writeable.
            return;
        }

        size_t fileOffset = (at - m_Address) + m_Offset;

        physical_uintptr_t p = m_Mappings.lookup(at);
        if(p == static_cast<physical_uintptr_t>(~0UL))
            m_pBacking->sync(fileOffset, async);
    }
}

void MemoryMappedFile::invalidate(uintptr_t at)
{
    VirtualAddressSpace &va = Processor::information().getVirtualAddressSpace();
    size_t pageSz = PhysicalMemoryManager::getPageSize();

    // If we're not actually CoW, don't bother checking.
    if(!m_bCopyOnWrite)
    {
        return;
    }

    if(at < m_Address || at >= (m_Address + m_Length))
    {
        ERROR("MemoryMappedFile::invalidate() given bad at parameter (at=" << at << ", address=" << m_Address << ", end=" << (m_Address + m_Length) << ")");
        return;
    }

    size_t fileOffset = (at - m_Address) + m_Offset;

    // Check for already-invalidated.
    physical_uintptr_t p = m_Mappings.lookup(at);
    if(p == static_cast<physical_uintptr_t>(~0UL))
        return;
    else
    {
        void *v = reinterpret_cast<void *>(at);

        if(va.isMapped(v))
        {
            // Clean up old...
            va.unmap(v);
            PhysicalMemoryManager::instance().freePage(p);
            m_Mappings.remove(at);

            // Get new...
            physical_uintptr_t newBacking = getBackingPage(m_pBacking, fileOffset);
            if(newBacking == static_cast<physical_uintptr_t>(~0UL))
            {
                ERROR("MemoryMappedFile::invalidate() couldn't bring in new backing page!");
                return; // Fail.
            }

            // Bring in the new backing page.
            va.map(newBacking, v, VirtualAddressSpace::Shared);
            m_Mappings.insert(at, static_cast<physical_uintptr_t>(~0UL));
        }
    }
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
        {
            size_t fileOffset = (it.key() - m_Address) + m_Offset;
            m_pBacking->returnPhysicalPage(fileOffset);
            m_pBacking->sync(fileOffset, true);
        }
        else
            PhysicalMemoryManager::instance().freePage(phys);
    }

    m_Mappings.clear();
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
        {
            flags |= VirtualAddressSpace::Write;
        }

        va.map(phys, reinterpret_cast<void *>(address), flags);

        m_Mappings.insert(address, ~0);
    }
    else
    {
        // Ditch an existing mapping, if needed.
        if(va.isMapped(reinterpret_cast<void *>(address)))
        {
            va.unmap(reinterpret_cast<void *>(address));

            // One less reference to the backing page.
            m_pBacking->returnPhysicalPage(fileOffset);
            m_Mappings.remove(address);
        }

        // Okay, map in the new page, and copy across the backing file data.
        physical_uintptr_t newPhys = PhysicalMemoryManager::instance().allocatePage();
        va.map(newPhys, reinterpret_cast<void *>(address), VirtualAddressSpace::Write);

        size_t nBytes = m_Length - mappingOffset;
        if(nBytes > pageSz)
            nBytes = pageSz;

        size_t nRead = m_pBacking->read(fileOffset, nBytes, address);
        if(nRead < pageSz)
        {
            // Couldn't quite read in a page - zero out what's left.
            memset(reinterpret_cast<void *>(address + nRead), 0, pageSz - nRead - 1);
        }

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

    // Override any existing mappings that might exist.
    remove(address, length);

#ifdef DEBUG_MMOBJECTS
    NOTICE("MemoryMapManager::mapFile: " << address << " length " << actualLength << " for " << pFile->getName());
#endif
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

    // Override any existing mappings that might exist.
    remove(address, length);

#ifdef DEBUG_MMOBJECTS
    NOTICE("MemoryMapManager::mapAnon: " << address << " length " << length);
#endif
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

size_t MemoryMapManager::remove(uintptr_t base, size_t length)
{
#ifdef DEBUG_MMOBJECTS
    NOTICE("MemoryMapManager::remove(" << base << ", " << length << ")");
#endif

    VirtualAddressSpace &va = Processor::information().getVirtualAddressSpace();
    size_t pageSz = PhysicalMemoryManager::getPageSize();

    size_t nAffected = 0;

    if(length & (pageSz - 1))
    {
        length += pageSz;
        length &= ~(pageSz - 1);
    }

    uintptr_t removeEnd = base + length;

    m_Lock.acquire();

    MmObjectList *pMmObjectList = m_MmObjectLists.lookup(&va);
    if(!pMmObjectList)
    {
        m_Lock.release();
        return 0;
    }

    for(List<MemoryMappedObject*>::Iterator it = pMmObjectList->begin();
        it != pMmObjectList->end();
        ++it)
    {
        MemoryMappedObject *pObject = *it;

        uintptr_t objEnd = pObject->address() + pObject->length();

#ifdef DEBUG_MMOBJECTS
        NOTICE("MemoryMapManager::remove() - object at " << pObject->address() << " -> " << objEnd << ".");
#endif

        uintptr_t objAlignEnd = objEnd;
        if(objAlignEnd & (pageSz - 1))
        {
            objAlignEnd += pageSz;
            objAlignEnd &= ~(pageSz - 1);
        }

        // Avoid?
        if(pObject->address() == removeEnd)
        {
            continue;
        }

        // Direct removal?
        else if(pObject->address() == base)
        {
#ifdef DEBUG_MMOBJECTS
            NOTICE("MemoryMapManager::remove() - a direct removal");
#endif
            bool bAll = pObject->remove(length);
            if(bAll)
            {
                it = pMmObjectList->erase(it);
                delete pObject;
            }
        }

        // Object fully contains parameters.
        else if((pObject->address() < base) && (removeEnd <= objAlignEnd))
        {
#ifdef DEBUG_MMOBJECTS
            NOTICE("MemoryMapManager::remove() - fully enclosed removal");
#endif
            MemoryMappedObject *pNewObject = pObject->split(base);
            bool bAll = pNewObject->remove(removeEnd - base);
            if(!bAll)
            {
                // Remainder not fully removed - add to housekeeping.
                pMmObjectList->pushBack(pNewObject);
            }
        }

        // Object in the middle of the parameters (neither begin or end inside)
        else if((pObject->address() > base) && (objEnd >= base) && (objEnd <= removeEnd))
        {
#ifdef DEBUG_MMOBJECTS
            NOTICE("MemoryMapManager::remove() - begin before start, end after object end");
#endif
            // Outright unmap.
            pObject->unmap();
            delete pObject;

            it = pMmObjectList->erase(it);
        }

        // End is within the object, start is before the object.
        else if((pObject->address() > base) && (removeEnd >= pObject->address()) && (removeEnd <= objEnd))
        {
#ifdef DEBUG_MMOBJECTS
            NOTICE("MemoryMapManager::remove() - begin outside, end inside");
#endif
            MemoryMappedObject *pNewObject = pObject->split(removeEnd);

            pObject->unmap();
            delete pObject;

            it = pMmObjectList->erase(it);

            pMmObjectList->pushBack(pNewObject);
        }

        // Start is within the object, end is past the end of the object.
        else if((pObject->address() < base) && (base < objEnd) && (removeEnd >= objEnd))
        {
#ifdef DEBUG_MMOBJECTS
            NOTICE("MemoryMapManager::remove() - begin inside, end outside");
#endif
            MemoryMappedObject *pNewObject = pObject->split(base);
            pNewObject->unmap();
            delete pNewObject;
        }

        // Nothing!
        else
        {
#ifdef DEBUG_MMOBJECTS
            NOTICE("MemoryMapManager::remove() - doing nothing!");
#endif
            continue;
        }

        ++nAffected;
    }

    m_Lock.release();

    return nAffected;
}

void MemoryMapManager::op(MemoryMapManager::Ops what, uintptr_t base, size_t length, bool async)
{
    VirtualAddressSpace &va = Processor::information().getVirtualAddressSpace();
    size_t pageSz = PhysicalMemoryManager::getPageSize();

    m_Lock.acquire();

    MmObjectList *pMmObjectList = m_MmObjectLists.lookup(&va);
    if (!pMmObjectList)
    {
        m_Lock.release();
        return;
    }

    for(uintptr_t address = base; address < (base + length); address += pageSz)
    {
        for (List<MemoryMappedObject*>::Iterator it = pMmObjectList->begin();
             it != pMmObjectList->end();
             it++)
        {
            MemoryMappedObject *pObject = *it;
            if(pObject->matches(address & ~(pageSz - 1)))
            {
                switch(what)
                {
                    case Sync:
                        pObject->sync(address, async);
                        break;
                    case Invalidate:
                        pObject->invalidate(address);
                        break;
                    default:
                        WARNING("Bad 'what' in MemoryMapManager::op()");
                }
            }
        }
    }

    m_Lock.release();
}

void MemoryMapManager::sync(uintptr_t base, size_t length, bool async)
{
    op(Sync, base, length, async);
}

void MemoryMapManager::invalidate(uintptr_t base, size_t length)
{
    op(Invalidate, base, length, false);
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
    size_t pageSz = PhysicalMemoryManager::getPageSize();

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

        // Passing in a page-aligned address means we handle the case where
        // a mapping ends midway through a page and a trap happens after this.
        // Because we map in terms of pages, but store unaligned 'actual'
        // lengths (for proper page zeroing etc), this is necessary.
        if(pObject->matches(address & ~(pageSz - 1)))
        {
            m_Lock.release();
            return pObject->trap(address, bIsWrite);
        }
    }

#ifdef DEBUG_MMOBJECTS
    ERROR("MemoryMapManager::trap() could not find an object for " << address);
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
