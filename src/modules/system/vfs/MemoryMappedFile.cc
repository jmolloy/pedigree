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

#include "MemoryMappedFile.h"

#include <processor/PhysicalMemoryManager.h>
#include <process/MemoryPressureManager.h>
#include <Spinlock.h>

#include <utilities/assert.h>

MemoryMapManager MemoryMapManager::m_Instance;

physical_uintptr_t AnonymousMemoryMap::m_Zero = 0;

// #define DEBUG_MMOBJECTS

MemoryMappedObject::~MemoryMappedObject()
{
}

AnonymousMemoryMap::AnonymousMemoryMap(uintptr_t address, size_t length, MemoryMappedObject::Permissions perms) :
    MemoryMappedObject(address, true, length, perms), m_Mappings()
{
    if(m_Zero == 0)
    {
        m_Zero = PhysicalMemoryManager::instance().allocatePage();
        PhysicalMemoryManager::instance().pin(m_Zero);

        VirtualAddressSpace &va = Processor::information().getVirtualAddressSpace();
        va.map(m_Zero, reinterpret_cast<void *>(address), VirtualAddressSpace::Write);
        memset(reinterpret_cast<void *>(address), 0, PhysicalMemoryManager::getPageSize());
        va.unmap(reinterpret_cast<void *>(address));
    }
}

MemoryMappedObject *AnonymousMemoryMap::clone()
{
    AnonymousMemoryMap *pResult = new AnonymousMemoryMap(m_Address, m_Length, m_Permissions);
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
    AnonymousMemoryMap *pResult = new AnonymousMemoryMap(at, oldLength - m_Length, m_Permissions);

    // Fix up mapping metadata.
    for(List<void *>::Iterator it = m_Mappings.begin();
        it != m_Mappings.end();
        )
    {
        uintptr_t v = reinterpret_cast<uintptr_t>(*it);
        if(v >= at)
        {
            pResult->m_Mappings.pushBack(*it);
            it = m_Mappings.erase(it);
        }
        else
            ++it;
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
        )
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

void AnonymousMemoryMap::setPermissions(MemoryMappedObject::Permissions perms)
{
    VirtualAddressSpace &va = Processor::information().getVirtualAddressSpace();

    if(perms == MemoryMappedObject::None)
    {
        unmap();
    }
    else
    {
        size_t newFlags = 0;

        // Remap any existing mappings to use the new flags.
        if(perms & MemoryMappedObject::Write)
            newFlags |= VirtualAddressSpace::Write;
        if(perms & MemoryMappedObject::Exec)
            newFlags |= VirtualAddressSpace::Execute;

        // Adjust any existing mappings in this object.
        for(List<void *>::Iterator it = m_Mappings.begin();
            it != m_Mappings.end();
            ++it)
        {
            void *v = *it;
            if(va.isMapped(v))
            {
                physical_uintptr_t p;
                size_t f;
                va.getMapping(v, p, f);

                // Shared pages will have write/exec added to them when written to.
                if(!(f & VirtualAddressSpace::Shared))
                {
                    // Make sure we remove permissions as well as add them.
                    if(perms & MemoryMappedObject::Write)
                        f |= VirtualAddressSpace::Write;
                    else
                        f &= ~VirtualAddressSpace::Write;

                    if(perms & MemoryMappedObject::Exec)
                        f |= VirtualAddressSpace::Execute;
                    else
                        f &= ~VirtualAddressSpace::Execute;

                    va.setFlags(v, f);
                }
                else if(perms & MemoryMappedObject::Exec)
                {
                    // We can however still make these pages executable.
                    va.setFlags(v, f | VirtualAddressSpace::Execute);
                }
            }
        }
    }

    m_Permissions = perms;
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

    // Skip out on a few things if we can.
    if(bWrite && !(m_Permissions & Write))
    {
#ifdef DEBUG_MMOBJECTS
        NOTICE("  -> no write permission");
#endif
        return false;
    }
    else if((!bWrite) && !(m_Permissions & Read))
    {
#ifdef DEBUG_MMOBJECTS
        NOTICE("  -> no read permission");
#endif
        return false;
    }

    // Add execute flag.
    size_t extraFlags = 0;
    if(m_Permissions & Exec)
        extraFlags |= VirtualAddressSpace::Execute;

    if(!bWrite)
    {
        PhysicalMemoryManager::instance().pin(m_Zero);
        if(!va.map(m_Zero, reinterpret_cast<void *>(address), VirtualAddressSpace::Shared | extraFlags))
            ERROR("map() failed for AnonymousMemoryMap::trap() - read");

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
        if(!va.map(newPage, reinterpret_cast<void *>(address), VirtualAddressSpace::Write | extraFlags))
            ERROR("map() failed in AnonymousMemoryMap::trap() - write");
        memset(reinterpret_cast<void *>(address), 0, PhysicalMemoryManager::getPageSize());
    }

    return true;
}

MemoryMappedFile::MemoryMappedFile(uintptr_t address, size_t length, size_t offset, File *backing, bool bCopyOnWrite, MemoryMappedObject::Permissions perms) :
    MemoryMappedObject(address, bCopyOnWrite, length, perms), m_pBacking(backing), m_Offset(offset), m_Mappings()
{
    assert(m_pBacking);
}

MemoryMappedObject *MemoryMappedFile::clone()
{
    MemoryMappedFile *pResult = new MemoryMappedFile(m_Address, m_Length, m_Offset, m_pBacking, m_bCopyOnWrite, m_Permissions);
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
    MemoryMappedFile *pResult = new MemoryMappedFile(at, oldLength - m_Length, m_Offset + m_Length, m_pBacking, m_bCopyOnWrite, m_Permissions);

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

                // Only sync back to the backing store if the page was actually
                // mapped in writable (ie, shared and not CoW)
                if((flags & VirtualAddressSpace::Write) == VirtualAddressSpace::Write)
                    m_pBacking->sync(fileOffset, true);
            }
            else
                PhysicalMemoryManager::instance().freePage(phys);
        }

        m_Mappings.remove(virt);
    }

    return false;
}

void MemoryMappedFile::setPermissions(MemoryMappedObject::Permissions perms)
{
    VirtualAddressSpace &va = Processor::information().getVirtualAddressSpace();

    if(perms == MemoryMappedObject::None)
    {
        unmap();
    }
    else
    {
        // Adjust any existing mappings in this object.
        for(Tree<uintptr_t, uintptr_t>::Iterator it = m_Mappings.begin();
            it != m_Mappings.end();
            ++it)
        {
            void *v = reinterpret_cast<void *>(it.key());
            if(va.isMapped(v))
            {
                physical_uintptr_t p;
                size_t f;
                va.getMapping(v, p, f);

                // Modify executable state - applies to shared/copied...
                if(perms & MemoryMappedObject::Exec)
                    f |= VirtualAddressSpace::Execute;
                else
                    f &= ~VirtualAddressSpace::Execute;

                if(f & VirtualAddressSpace::Shared)
                {
                    // Shared pages can be written to if not CoW.
                    // If CoW, setting m_Permissions will sort the rest out.
                    if(!m_bCopyOnWrite)
                    {
                        if(perms & MemoryMappedObject::Write)
                            f |= VirtualAddressSpace::Write;
                        else
                            f &= ~VirtualAddressSpace::Write;
                    }
                }
                else
                {
                    // Adjust permissions as needed. Not a shared page, specific
                    // to this address space.
                    if(perms & MemoryMappedObject::Write)
                        f |= VirtualAddressSpace::Write;
                    else
                        f &= ~VirtualAddressSpace::Write;
                }

                va.setFlags(v, f);
            }
        }
    }

    m_Permissions = perms;
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
        {
            m_pBacking->sync(fileOffset, async);
        }
    }
}

void MemoryMappedFile::invalidate(uintptr_t at)
{
    VirtualAddressSpace &va = Processor::information().getVirtualAddressSpace();

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

    // Add execute flag.
    size_t extraFlags = 0;
    if(m_Permissions & Exec)
        extraFlags |= VirtualAddressSpace::Execute;

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
            va.map(newBacking, v, VirtualAddressSpace::Shared | extraFlags);
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

            // Only sync back to the backing store if the page was actually
            // mapped in writable (ie, shared and not CoW)
            if((flags & VirtualAddressSpace::Write) == VirtualAddressSpace::Write)
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

    // Skip out on a few things if we can.
    if(bWrite && !(m_Permissions & Write))
    {
#ifdef DEBUG_MMOBJECTS
        DEBUG_LOG(" -> ignoring, was a write and this is not a writable mapping.");
#endif
        return false;
    }
    else if((!bWrite) && !(m_Permissions & Read))
    {
#ifdef DEBUG_MMOBJECTS
        DEBUG_LOG(" -> ignoring, was a read and this is not a readable mapping.");
#endif
        return false;
    }

#ifdef DEBUG_MMOBJECTS
    DEBUG_LOG(" -> mapping offset is " << mappingOffset << ", file offset: " << fileOffset);
    DEBUG_LOG(" -> will eof: " << bWillEof << ", should copy: " << bShouldCopy);
#endif

    // Add execute flag.
    size_t extraFlags = 0;
    if(m_Permissions & Exec)
        extraFlags |= VirtualAddressSpace::Execute;

    if(!bShouldCopy)
    {
        physical_uintptr_t phys = getBackingPage(m_pBacking, fileOffset);
        if(phys == static_cast<physical_uintptr_t>(~0UL))
        {
            ERROR("MemoryMappedFile::trap couldn't get a backing page");
            return false; // Fail.
        }

        size_t flags = VirtualAddressSpace::Shared;
        if(!m_bCopyOnWrite)
        {
            flags |= VirtualAddressSpace::Write;
        }

        bool r = va.map(phys, reinterpret_cast<void *>(address), flags | extraFlags);
        if(!r)
        {
            ERROR("map() failed in MemoryMappedFile::trap (no-copy)");
            return false;
        }

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
        bool r = va.map(newPhys, reinterpret_cast<void *>(address), VirtualAddressSpace::Write | extraFlags);
        if(!r)
        {
            ERROR("map() failed in MemoryMappedFile::trap (copy)");
            return false;
        }

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

bool MemoryMappedFile::compact()
{
    VirtualAddressSpace &va = Processor::information().getVirtualAddressSpace();
    size_t pageSz = PhysicalMemoryManager::getPageSize();

    uintptr_t base = m_Address;
    uintptr_t end = base + m_Length;

    bool bReleased = false;

    // Is this mapping even writeable?
    // CoW mappings can't be removed as we have no way of saving their data.
    if((m_Permissions & Write) && !m_bCopyOnWrite)
    {
        // Yes - can we get any dirty pages?
        for(uintptr_t addr = base; addr < end; addr += pageSz)
        {
            physical_uintptr_t p = m_Mappings.lookup(addr);
            if(p != static_cast<physical_uintptr_t>(~0UL))
                continue;

            size_t flags = 0;
            physical_uintptr_t phys = 0;
            va.getMapping(reinterpret_cast<void *>(addr), phys, flags);

            // Avoid read-only pages for now.
            if(!(flags & VirtualAddressSpace::Write))
                continue;

            size_t mappingOffset = (addr - m_Address);
            size_t fileOffset = m_Offset + mappingOffset;

            // Sync data back to file, synchronously
            m_pBacking->sync(fileOffset, false);

            // Wipe out the mapping so we have to trap again.
            va.unmap(reinterpret_cast<void *>(addr));

            // Unpin the page, allowing the cache subsystem to evict it.
            m_pBacking->returnPhysicalPage(fileOffset);
            m_Mappings.remove(addr);

            bReleased = true;
        }
    }

    // Read-only page pass.
    if(!bReleased)
    {
        for(uintptr_t addr = base; addr < end; addr += pageSz)
        {
            physical_uintptr_t p = m_Mappings.lookup(addr);
            if(p != static_cast<physical_uintptr_t>(~0UL))
                continue;

            size_t flags = 0;
            physical_uintptr_t phys = 0;
            va.getMapping(reinterpret_cast<void *>(addr), phys, flags);

            // Avoid writeable pages in this pass.
            if((flags & VirtualAddressSpace::Write))
                continue;

            size_t mappingOffset = (addr - m_Address);
            size_t fileOffset = m_Offset + mappingOffset;

            // Wipe out the mapping so we have to trap again.
            va.unmap(reinterpret_cast<void *>(addr));

            // Unpin the page, allowing the cache subsystem to evict it.
            m_pBacking->returnPhysicalPage(fileOffset);
            m_Mappings.remove(addr);

            bReleased = true;
        }
    }

    return bReleased;
}

MemoryMapManager::MemoryMapManager() :
    m_MmObjectLists(), m_Lock()
{
    PageFaultHandler::instance().registerHandler(this);
    MemoryPressureManager::instance().registerHandler(MemoryPressureManager::HighPriority, this);
}

MemoryMapManager::~MemoryMapManager()
{
    MemoryPressureManager::instance().removeHandler(this);
}

MemoryMappedObject *MemoryMapManager::mapFile(File *pFile, uintptr_t &address, size_t length, MemoryMappedObject::Permissions perms, size_t offset, bool bCopyOnWrite)
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
        address, actualLength, offset, pFile, bCopyOnWrite, perms);

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

MemoryMappedObject *MemoryMapManager::mapAnon(uintptr_t &address, size_t length, MemoryMappedObject::Permissions perms)
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
    AnonymousMemoryMap *pMap = new AnonymousMemoryMap(address, length, perms);

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
        )
    {
        MemoryMappedObject *pObject = *it;

        // Whether or not  it = x.erase() was called - because we should not
        // increment an iterator if so.
        bool bErased = false;

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
            ++it;
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
                bErased = true;
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

            it = pMmObjectList->erase(it);
            delete pObject;
            bErased = true;
        }

        // End is within the object, start is before the object.
        else if((pObject->address() > base) && (removeEnd >= pObject->address()) && (removeEnd <= objEnd))
        {
#ifdef DEBUG_MMOBJECTS
            NOTICE("MemoryMapManager::remove() - begin outside, end inside");
#endif
            MemoryMappedObject *pNewObject = pObject->split(removeEnd);

            pObject->unmap();

            it = pMmObjectList->erase(it);
            delete pObject;
            bErased = true;

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
            ++it;
            continue;
        }

        if(!bErased)
        {
            ++it;
        }

        ++nAffected;
    }

    m_Lock.release();

    return nAffected;
}

size_t MemoryMapManager::setPermissions(uintptr_t base, size_t length, MemoryMappedObject::Permissions perms)
{
#ifdef DEBUG_MMOBJECTS
    NOTICE("MemoryMapManager::setPermissions(" << base << ", " << length << ")");
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
        NOTICE("MemoryMapManager::setPermissions() - object at " << pObject->address() << " -> " << objEnd << ".");
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

        // Direct?
        else if(pObject->address() == base)
        {
#ifdef DEBUG_MMOBJECTS
            NOTICE("MemoryMapManager::setPermissions() - a direct set");
#endif
            if(pObject->length() > length)
            {
                // Split needed.
                MemoryMappedObject *pNewObject = pObject->split(base + length);
                pMmObjectList->pushBack(pNewObject);
            }

            pObject->setPermissions(perms);
        }

        // Object fully contains parameters.
        else if((pObject->address() < base) && (removeEnd <= objAlignEnd))
        {
#ifdef DEBUG_MMOBJECTS
            NOTICE("MemoryMapManager::setPermissions() - fully enclosed set");
#endif
            MemoryMappedObject *pNewObject = pObject->split(base);

            if(removeEnd < objAlignEnd)
            {
                MemoryMappedObject *pTailObject = pNewObject->split(removeEnd);
                pMmObjectList->pushBack(pTailObject);
            }

            pNewObject->setPermissions(perms);
            pMmObjectList->pushBack(pNewObject);
        }

        // Object in the middle of the parameters (neither begin or end inside)
        else if((pObject->address() > base) && (objEnd >= base) && (objEnd <= removeEnd))
        {
#ifdef DEBUG_MMOBJECTS
            NOTICE("MemoryMapManager::setPermissions() - begin before start, end after object end");
#endif
            // Outright set.
            pObject->setPermissions(perms);
        }

        // End is within the object, start is before the object.
        else if((pObject->address() > base) && (removeEnd >= pObject->address()) && (removeEnd <= objEnd))
        {
#ifdef DEBUG_MMOBJECTS
            NOTICE("MemoryMapManager::setPermissions() - begin outside, end inside");
#endif
            MemoryMappedObject *pNewObject = pObject->split(removeEnd);

            pObject->setPermissions(perms);
            pMmObjectList->pushBack(pNewObject);
        }

        // Start is within the object, end is past the end of the object.
        else if((pObject->address() < base) && (base < objEnd) && (removeEnd >= objEnd))
        {
#ifdef DEBUG_MMOBJECTS
            NOTICE("MemoryMapManager::setPermissions() - begin inside, end outside");
#endif
            MemoryMappedObject *pNewObject = pObject->split(base);
            pNewObject->setPermissions(perms);
            pMmObjectList->pushBack(pNewObject);
        }

        // Nothing!
        else
        {
#ifdef DEBUG_MMOBJECTS
            NOTICE("MemoryMapManager::setPermissions() - doing nothing!");
#endif
            continue;
        }

        ++nAffected;
    }

    m_Lock.release();

    return nAffected;
}

bool MemoryMapManager::contains(uintptr_t base, size_t length)
{
    VirtualAddressSpace &va = Processor::information().getVirtualAddressSpace();
    size_t pageSz = PhysicalMemoryManager::getPageSize();

    LockGuard<Mutex> guard(m_Lock);

    MmObjectList *pMmObjectList = m_MmObjectLists.lookup(&va);
    if (!pMmObjectList)
    {
        return false;
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
                return true;
            }
        }
    }

    return false;
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
        if(!pProcess->getDynamicSpaceAllocator().allocate(length + pageSz, address))
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
        /// \todo allocateSpecific in the dynamic space allocator if address is
        ///       within that range.
        pProcess->getSpaceAllocator().allocateSpecific(address, length);
    }

    return true;
}

bool MemoryMapManager::compact()
{
    // Track current address space as we need to switch into each known address
    // space in order to compact them.
    VirtualAddressSpace &currva = Processor::information().getVirtualAddressSpace();

    bool bCompact = false;
    for(Tree<VirtualAddressSpace*, MmObjectList*>::Iterator it = m_MmObjectLists.begin();
        it != m_MmObjectLists.end();
        ++it)
    {
        Processor::switchAddressSpace(*it.key());

        for(MmObjectList::Iterator it2 = it.value()->begin();
            it2 != it.value()->end();
            ++it2)
        {
            bCompact = (*it2)->compact();
            if(bCompact)
                break;
        }

        if(bCompact)
            break;
    }

    // Restore old address space now.
    Processor::switchAddressSpace(currva);

    // Memory mapped files tend to un-pin pages for the Cache system to
    // release, so we never return success (as we never actually released
    // pages and therefore didn't resolve any memory pressure).
    if(bCompact)
        NOTICE("    -> success, hoping for Cache eviction...");
    return false;
}
