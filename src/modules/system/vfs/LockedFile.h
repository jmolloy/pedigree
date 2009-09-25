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

#ifndef LOCKED_FILE_H
#define LOCKED_FILE_H

#include <processor/types.h>
#include <process/Scheduler.h>
#include <process/Mutex.h>
#include <Log.h>

#include "File.h"

/** LockedFile is a wrapper around a standard File with the ability to lock
 * access to it. Locked access is in the form of a Mutex that allows only
 * one thread exclusive access to the file.
 */
class LockedFile
{
public:

    /** Standard wrapper constructor */
    LockedFile(File *pFile) : m_File(pFile), m_bLocked(false), m_LockerPid(0), m_Lock(false)
    {};

    /** Copy constructor */
    LockedFile(LockedFile &c) : m_File(0), m_bLocked(false), m_LockerPid(0), m_Lock(false)
    {
        m_File = c.m_File;
        m_bLocked = c.m_bLocked;
        m_LockerPid = c.m_LockerPid;

        if(m_bLocked)
            m_Lock.acquire();
    }

    /** Operator = */
    /// \todo Write me!
    LockedFile & operator = (const LockedFile& c);

    /** Attempts to obtain the lock (exclusively) */
    bool lock(bool bBlock = false)
    {
        if(!bBlock)
        {
            if(!m_Lock.tryAcquire())
                return false;
        }
        else
            m_Lock.acquire();

        // Obtained the lock
        m_bLocked = true;
        m_LockerPid = Processor::information().getCurrentThread()->getParent()->getId();
        return true;
    }

    /** Releases the lock */
    void unlock()
    {
        if(m_bLocked)
        {
            m_bLocked = false;
            m_Lock.release();
        }
    }

    /** To enforce mandatory locking, use this function to obtain a File to
     * work with. If the file is locked and you don't own the lock, you'll
     * get a NULL File. Otherwise you'll get the wrapped File ready for I/O.
     */
    File *getFile()
    {
        // If we're locked, and we aren't the locking process, we can't access the file
        // Otherwise, the file is accessible
        if(m_bLocked == true && Processor::information().getCurrentThread()->getParent()->getId() != m_LockerPid)
            return 0;
        else
            return m_File;
    }

    /** Who's locking the file? */
    size_t getLocker()
    {
        if(m_bLocked)
            return m_LockerPid;
        else
            return 0;
    }

private:

    /** Default constructor, not to be used */
    LockedFile();

    /** Is a range locked? */

    /** The file that we're wrapping */
    File *m_File;

    /** Is this file locked? */
    bool m_bLocked;

    /** Locker PID */
    size_t m_LockerPid;

    /** Our lock */
    Mutex m_Lock;
};

#endif
