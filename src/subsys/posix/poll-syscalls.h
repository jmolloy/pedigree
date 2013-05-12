/*
 * Copyright (c) 2013 Matthew Iselin
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

#ifndef POLL_SYSCALLS_H
#define POLL_SYSCALLS_H

#include "file-syscalls.h"
#include <process/Event.h>
#include <process/eventNumbers.h>

#include <poll.h>

/** Event class for passing to File::monitor. */
class PollEvent : public Event
{
public:
    /** The constructor takes a semaphore that it should signal when it fires,
        and an fd_set with an index to set. */
    PollEvent();
    PollEvent(Semaphore *pSemaphore, struct pollfd *fd, int revent, File *pFile);
    ~PollEvent();

    void fire();

    File *getFile()
    {
        return m_pFile;
    }

    //
    // Event interface
    //
    virtual size_t serialize(uint8_t *pBuffer);
    static bool unserialize(uint8_t *pBuffer, PollEvent &event);
    virtual size_t getNumber()
    {
        return EventNumbers::PollEvent;
    }

private:
    Semaphore *m_pSemaphore;
    struct pollfd *m_pFd;
    int        m_nREvent;
    File      *m_pFile;
};

int posix_poll(struct pollfd* fds, unsigned int nfds, int timeout);

#endif
