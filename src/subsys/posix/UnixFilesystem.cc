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

#include "UnixFilesystem.h"
#include <LockGuard.h>

UnixSocket::UnixSocket(String name, Filesystem *pFs, File *pParent) :
    File(name, 0, 0, 0, 0, pFs, 0, pParent),
    m_RingBuffer(MAX_UNIX_DGRAM_BACKLOG)
{
}

UnixSocket::~UnixSocket()
{
}

int UnixSocket::select(bool bWriting, int timeout)
{
    if(timeout)
    {
        while(!m_RingBuffer.waitFor(bWriting ? RingBufferWait::Writing : RingBufferWait::Reading));
        return 1;
    }
    else if(bWriting)
    {
        return m_RingBuffer.canWrite();
    }
    else
    {
        return m_RingBuffer.dataReady();
    }
}

uint64_t UnixSocket::read(uint64_t location, uint64_t size, uintptr_t buffer, bool bCanBlock)
{
    if(bCanBlock)
    {
        if(!m_RingBuffer.waitFor(RingBufferWait::Reading))
            return 0; // Interrupted.
    }
    else if(!m_RingBuffer.dataReady())
    {
        return 0;
    }

    struct buf *b = m_RingBuffer.read();
    if(size > b->len)
        size = b->len;
    MemoryCopy(reinterpret_cast<void *>(buffer), b->pBuffer, size);
    if(b->remotePath)
    {
        if(location)
        {
            StringCopyN(reinterpret_cast<char *>(location), b->remotePath, 255);
        }

        delete [] b->remotePath;
    }
    delete [] b->pBuffer;
    delete b;

    return size;
}

uint64_t UnixSocket::write(uint64_t location, uint64_t size, uintptr_t buffer, bool bCanBlock)
{
    if(bCanBlock)
    {
        if(!m_RingBuffer.waitFor(RingBufferWait::Writing))
            return 0; // Interrupted.
    }
    else if(!m_RingBuffer.canWrite())
    {
        return 0;
    }

    struct buf *b = new struct buf;
    b->pBuffer = new char[size];
    MemoryCopy(b->pBuffer, reinterpret_cast<void*>(buffer), size);
    b->len = size;
    b->remotePath = 0;
    if(location)
    {
        b->remotePath = new char[255];
        StringCopyN(b->remotePath, reinterpret_cast<char *>(location), 255);
    }
    m_RingBuffer.write(b);

    dataChanged();

    return size;
}

UnixDirectory::UnixDirectory(String name, Filesystem *pFs, File *pParent) :
    Directory(name, 0, 0, 0, 0, pFs, 0, pParent), m_Lock(false)
{
    cacheDirectoryContents();
}

UnixDirectory::~UnixDirectory()
{
}

bool UnixDirectory::addEntry(String filename, File *pFile)
{
    LockGuard<Mutex> guard(m_Lock);
    getCache().insert(filename, pFile);
    return true;
}

bool UnixDirectory::removeEntry(File *pFile)
{
    String filename = pFile->getName();

    LockGuard<Mutex> guard(m_Lock);
    getCache().remove(filename);
    return true;
}

void UnixDirectory::cacheDirectoryContents()
{
    m_bCachePopulated = true;
}

UnixFilesystem::UnixFilesystem() :
    Filesystem(), m_pRoot(0)
{
    UnixDirectory *pRoot = new UnixDirectory(String(""), this, 0);
    pRoot->addEntry(String("."), pRoot);
    pRoot->addEntry(String(".."), pRoot);

    m_pRoot = pRoot;
}

UnixFilesystem::~UnixFilesystem()
{
    delete m_pRoot;
}

bool UnixFilesystem::createFile(File *parent, String filename, uint32_t mask)
{
    UnixDirectory *pParent = static_cast<UnixDirectory *>(Directory::fromFile(parent));

    UnixSocket *pSocket = new UnixSocket(filename, this, parent);
    if(!pParent->addEntry(filename, pSocket))
    {
        delete pSocket;
        return false;
    }

    return true;
}

bool UnixFilesystem::createDirectory(File *parent, String filename)
{
    UnixDirectory *pParent = static_cast<UnixDirectory *>(Directory::fromFile(parent));

    UnixDirectory *pChild = new UnixDirectory(filename, this, parent);
    if(!pParent->addEntry(filename, pChild))
    {
        delete pChild;
        return false;
    }

    pChild->addEntry(String("."), pChild);
    pChild->addEntry(String(".."), pParent);

    return true;
}

bool UnixFilesystem::remove(File *parent, File *file)
{
    UnixDirectory *pParent = static_cast<UnixDirectory *>(Directory::fromFile(parent));
    return pParent->removeEntry(file);
}

