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

#include "LockedFile.h"
#include <processor/Processor.h>
#include <Log.h>

LockedFile::LockedFile(File *pFile) : m_File(pFile), m_bLocked(false), m_LockerPid(0)
#ifdef THREADS
    , m_Lock(false)
#endif
{};

LockedFile::LockedFile(LockedFile &c) : m_File(0), m_bLocked(false), m_LockerPid(0)
#ifdef THREADS
    , m_Lock(false)
#endif
{
    m_File = c.m_File;
    m_bLocked = c.m_bLocked;
    m_LockerPid = c.m_LockerPid;

#ifdef THREADS
    if(m_bLocked)
    {
        m_Lock.acquire();
    }
#endif
}

bool LockedFile::lock(bool bBlock)
{
#ifdef THREADS
    if(!bBlock)
    {
        if(!m_Lock.tryAcquire())
        {
            return false;
        }
    }
    else
        m_Lock.acquire();

    // Obtained the lock
    m_bLocked = true;
    m_LockerPid = Processor::information().getCurrentThread()->getParent()->getId();
#endif
    return true;
}

void LockedFile::unlock()
{
#ifdef THREADS
    if(m_bLocked)
    {
        m_bLocked = false;
        m_Lock.release();
    }
#endif
}

File *LockedFile::getFile()
{
#ifdef THREADS
    // If we're locked, and we aren't the locking process, we can't access the file
    // Otherwise, the file is accessible
    if(m_bLocked == true && Processor::information().getCurrentThread()->getParent()->getId() != m_LockerPid)
        return 0;
    else
#endif
        return m_File;
}

size_t LockedFile::getLocker()
{
#ifdef THREADS
    if(m_bLocked)
        return m_LockerPid;
    else
#endif
        return 0;
}
