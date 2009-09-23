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

#ifndef SELECT_SYSCALLS_H
#define SELECT_SYSCALLS_H

#include "file-syscalls.h"
#include <process/Event.h>
#include <process/eventNumbers.h>

/** Event class for passing to File::monitor. */
class SelectEvent : public Event
{
public:
    /** The constructor takes a semaphore that it should signal when it fires,
        and an fd_set with an index to set. */
    SelectEvent();
    SelectEvent(Semaphore *pSemaphore, fd_set *pFdSet, size_t fdIdx, File *pFile);
    ~SelectEvent();

    void fire();

    File *getFile()
    {
        return m_pFile;
    }

    //
    // Event interface
    //
    virtual size_t serialize(uint8_t *pBuffer);
    static bool unserialize(uint8_t *pBuffer, SelectEvent &event);
    virtual size_t getNumber()
    {
        return EventNumbers::SelectEvent;
    }

private:
    Semaphore *m_pSemaphore;
    fd_set    *m_pFdSet;
    size_t     m_FdIdx;
    File      *m_pFile;
};

int posix_select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *errorfds, timeval *timeout);

#endif
