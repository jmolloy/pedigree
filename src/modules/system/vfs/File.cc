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

#include "VFS.h"
#include "File.h"
#include "Symlink.h"
#include "Filesystem.h"
#include <processor/Processor.h>
#include <Log.h>

File::File() :
    m_Name(""), m_AccessedTime(0), m_ModifiedTime(0),
    m_CreationTime(0), m_Inode(0), m_pFilesystem(0), m_Size(0),
    m_pParent(0), m_nWriters(0), m_nReaders(0), m_Uid(0), m_Gid(0), m_Permissions(0), m_DataCache(), m_Lock(), m_MonitorTargets()
{
}

File::File(String name, Time accessedTime, Time modifiedTime, Time creationTime,
           uintptr_t inode, Filesystem *pFs, size_t size, File *pParent) :
    m_Name(name), m_AccessedTime(accessedTime), m_ModifiedTime(modifiedTime),
    m_CreationTime(creationTime), m_Inode(inode), m_pFilesystem(pFs),
    m_Size(size), m_pParent(pParent), m_nWriters(0), m_nReaders(0), m_Uid(0), m_Gid(0), m_Permissions(0), m_DataCache(), m_Lock(), m_MonitorTargets()
{
}

File::~File()
{
}

uint64_t File::read(uint64_t location, uint64_t size, uintptr_t buffer, bool bCanBlock)
{
    if ((location+size) >= m_Size)
    {
        size_t oldSize = size;
        size = m_Size-location;
        if((location + size) > m_Size)
        {
            // Completely broken read parameters.
            ERROR("VFS: even after fixup, read at location " << location << " is larger than file size (" << m_Size << ")");
            ERROR("VFS:    fixup size: " << size << ", original size: " << oldSize);
            return 0;
        }
    }

    size_t blockSize = getBlockSize();
    
    size_t n = 0;
    while (size)
    {
        if (location >= m_Size)
            return n;

        uintptr_t block = location / blockSize;
        uintptr_t offs  = location % blockSize;
        uintptr_t sz    = (size+offs > blockSize) ? blockSize-offs : size;

        m_Lock.acquire();
        uintptr_t buff = m_DataCache.lookup(block*blockSize);
        if (!buff)
        {
            buff = readBlock(block*blockSize);
            m_DataCache.insert(block*blockSize, buff);
        }
        m_Lock.release();

        if(buffer)
        {
            memcpy(reinterpret_cast<void*>(buffer),
                   reinterpret_cast<void*>(buff+offs),
                   sz);
            buffer += sz;
        }
        location += sz;
        size -= sz;
        n += sz;
    }
    return n;
}

uint64_t File::write(uint64_t location, uint64_t size, uintptr_t buffer, bool bCanBlock)
{
    size_t blockSize = getBlockSize();

    // Extend the file before writing it if needed.
    extend(location + size);

    size_t n = 0;
    while (size)
    {
        uintptr_t block = location / blockSize;
        uintptr_t offs  = location % blockSize;
        uintptr_t sz    = (size+offs > blockSize) ? blockSize-offs : size;

        m_Lock.acquire();
        uintptr_t buff = m_DataCache.lookup(block*blockSize);
        if (!buff)
        {
            buff = readBlock(block*blockSize);
            m_DataCache.insert(block*blockSize, buff);
        }
        m_Lock.release();

        memcpy(reinterpret_cast<void*>(buff+offs),
               reinterpret_cast<void*>(buffer),
               sz);
        location += sz;
        buffer += sz;
        size -= sz;
        n += sz;
    }
    if (location >= m_Size)
    {
        m_Size = location;
        fileAttributeChanged();
    }
    return n;
}

physical_uintptr_t File::getPhysicalPage(size_t offset)
{
    // Sanitise input.
    size_t blockSize = getBlockSize();
    offset &= ~(blockSize - 1);

    // Quick and easy exit.
    if(offset > m_Size)
    {
        return static_cast<physical_uintptr_t>(~0UL);
    }

    // Check if we have this page in the cache.
    m_Lock.acquire();
    uintptr_t vaddr = m_DataCache.lookup(offset);
    m_Lock.release();
    if (!vaddr)
    {
        return static_cast<physical_uintptr_t>(~0UL);
    }

    // Look up the page now that we've confirmed it is in the cache.
    VirtualAddressSpace &va = Processor::information().getVirtualAddressSpace();
    if(va.isMapped(reinterpret_cast<void *>(vaddr)))
    {
        physical_uintptr_t phys = 0;
        size_t flags = 0;
        va.getMapping(reinterpret_cast<void *>(vaddr), phys, flags);

        return phys;
    }

    return static_cast<physical_uintptr_t>(~0UL);
}

Time File::getCreationTime()
{
    return m_CreationTime;
}

void File::setCreationTime(Time t)
{
    m_CreationTime = t;
    fileAttributeChanged();
}

Time File::getAccessedTime()
{
    return m_AccessedTime;
}

void File::setAccessedTime(Time t)
{
    m_AccessedTime = t;
    fileAttributeChanged();
}

Time File::getModifiedTime()
{
    return m_ModifiedTime;
}

void File::setModifiedTime(Time t)
{
    m_ModifiedTime = t;
    fileAttributeChanged();
}

String File::getName()
{
    return m_Name;
}

void File::getName(String &s)
{
    s = m_Name;
}

size_t File::getSize()
{
    return m_Size;
}

void File::setSize(size_t sz)
{
    m_Size = sz;
}

void File::truncate()
{
}

void File::dataChanged()
{
    LockGuard<Mutex> guard(m_Lock);

    for (List<MonitorTarget*>::Iterator it = m_MonitorTargets.begin();
         it != m_MonitorTargets.end();
         it++)
    {
        MonitorTarget *pMT = *it;

        pMT->pThread->sendEvent(pMT->pEvent);
        delete pMT;
    }

    m_MonitorTargets.clear();
}

void File::cullMonitorTargets(Thread *pThread)
{
    LockGuard<Mutex> guard(m_Lock);

    for (List<MonitorTarget*>::Iterator it = m_MonitorTargets.begin();
         it != m_MonitorTargets.end();
         it++)
    {
        MonitorTarget *pMT = *it;

        if (pMT->pThread == pThread)
        {
            delete pMT;
            m_MonitorTargets.erase(it);
            it = m_MonitorTargets.begin();
            if (it == m_MonitorTargets.end())
                return;
        }
    }
}
