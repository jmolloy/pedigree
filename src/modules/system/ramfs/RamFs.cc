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

#include <Log.h>
#include <vfs/VFS.h>
#include <Module.h>
#include "RamFs.h"

RamDir::RamDir(String name, size_t inode, class Filesystem *pFs, File *pParent) :
            Directory(name, 0, 0, 0, inode, pFs, 0, pParent), m_FileTree()
{}

RamDir::~RamDir()
{};

bool RamDir::addEntry(String filename, File *pFile, size_t type)
{
    // This'll get destroyed in the first write
    uint8_t *newFile = new uint8_t;
    pFile->setInode(reinterpret_cast<uintptr_t>(newFile));
    m_Cache.insert(filename, pFile);
    m_bCachePopulated = true;
    return true;
}

bool RamDir::removeEntry(File *pFile)
{
    uint8_t *buff = reinterpret_cast<uint8_t*>(pFile->getInode());
    if (buff)
        delete [] buff;
    m_Cache.remove(pFile->getName());
    return true;
}

RamFs::RamFs() : m_pRoot(0)
{}

RamFs::~RamFs()
{
    if(m_pRoot)
        delete m_pRoot;
}

bool RamFs::initialise(Disk *pDisk)
{
    m_pRoot = new RamDir(String(""), 0, this, 0);
    return true;
}

uint64_t RamFs::read(File *pFile, uint64_t location, uint64_t size, uintptr_t buffer)
{
    if (pFile->isDirectory())
        return 0;

    if (!pFile->getSize())
        return 0;

    size_t nBytes = 0;
    if (location > pFile->getSize())
        return 0;
    else if ((location + size) > pFile->getSize())
        nBytes = pFile->getSize() - location;
    else
        nBytes = size;

    if (nBytes == 0)
        return 0;

    uint8_t *dest = reinterpret_cast<uint8_t*>(buffer);
    uint8_t *src = reinterpret_cast<uint8_t*>(pFile->getInode());

    memcpy(dest, src + location, nBytes);

    return nBytes;
}

uint64_t RamFs::write(File *pFile, uint64_t location, uint64_t size, uintptr_t buffer)
{
    if (pFile->isDirectory())
        return 0;

    size_t nBytes = size;

    // Reallocate if needed
    if ((location + size) > pFile->getSize())
    {
        size_t numExtra = (location + size) - pFile->getSize();
        uint8_t *newBuff = new uint8_t[pFile->getSize() + numExtra];
        delete [] reinterpret_cast<uint8_t*>(pFile->getInode());
        pFile->setInode(reinterpret_cast<uintptr_t>(newBuff));

        pFile->setSize(pFile->getSize() + numExtra);
    }

    uint8_t *dest = reinterpret_cast<uint8_t*>(pFile->getInode());
    uint8_t *src = reinterpret_cast<uint8_t*>(buffer);

    memcpy(dest + location, src, nBytes);

    return nBytes;
}

/// \todo Write this
void RamFs::truncate(File *pFile)
{}

bool RamFs::createFile(File* parent, String filename, uint32_t mask)
{
    if (!parent->isDirectory())
        return false;

    File *f = new File(filename, 0, 0, 0, 0, this, 0, parent);

    RamDir *p = reinterpret_cast<RamDir*>(parent);
    return p->addEntry(filename, f, 0);
}

bool RamFs::createDirectory(File* parent, String filename)
{
    return false;
}

bool RamFs::createSymlink(File* parent, String filename, String value)
{
    return false;
}

bool RamFs::remove(File* parent, File* file)
{
    if (file->isDirectory())
        return false;

    RamDir *p = reinterpret_cast<RamDir*>(parent);
    return p->removeEntry(file);
}

void entry()
{
}

void destroy()
{
}

MODULE_NAME("ramfs");
MODULE_ENTRY(&entry);
MODULE_EXIT(&destroy);
MODULE_DEPENDS("vfs");
