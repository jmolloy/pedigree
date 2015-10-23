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

#include "Symlink.h"
#include "Filesystem.h"

Symlink::Symlink() :
    File(), m_pCachedSymlink(0)
{
}

Symlink::Symlink(String name, Time::Timestamp accessedTime, Time::Timestamp modifiedTime, Time::Timestamp creationTime,
                 uintptr_t inode, Filesystem *pFs, size_t size, File *pParent) :
    File(name,accessedTime,modifiedTime,creationTime,inode,pFs,size,pParent),
    m_pCachedSymlink(0), m_sTarget()
{
}

Symlink::~Symlink()
{
}

void Symlink::initialise(bool bForce)
{
    if (m_sTarget.length() && !bForce)
        return;

    size_t sz = getSize();
    if (sz > 0x1000)
        sz = 0x1000;

    // Read symlink target.
    char *pBuffer = new char[sz];
    read(0ULL, sz, reinterpret_cast<uintptr_t>(pBuffer));
    pBuffer[sz] = '\0';

    // Convert to String object, wipe out whitespace.
    m_sTarget = pBuffer;
    m_sTarget.rstrip();

    // Wipe out cached symlink if we're being forced to re-init.
    if (bForce)
        m_pCachedSymlink = 0;
}

File *Symlink::followLink()
{
    if (m_pCachedSymlink)
        return m_pCachedSymlink;

    initialise();

    m_pCachedSymlink = m_pFilesystem->find(m_sTarget, m_pParent);
    return m_pCachedSymlink;
}

int Symlink::followLink(char *pBuffer, size_t bufLen)
{
    initialise();

    if (m_sTarget.length() < bufLen)
        bufLen = m_sTarget.length();

    strncpy(pBuffer, static_cast<const char *>(m_sTarget), bufLen);

    if (bufLen < m_sTarget.length())
        pBuffer[bufLen] = '\0';

    return bufLen;
}
