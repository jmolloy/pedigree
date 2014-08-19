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

#include "Console.h"
#include <panic.h>
#include <utilities/assert.h>

UserConsole::UserConsole() :
    RequestQueue(), m_pReq(0), m_pPendingReq(0)
{
}

UserConsole::~UserConsole()
{
}

void UserConsole::requestPending()
{
    // This stops a race where m_pReq is used or changed between setting
    // m_pPendingReq and setting m_pReq to zero.
    m_RequestQueueMutex.acquire();
    m_pPendingReq = m_pReq;
    if(m_pReq)
        m_pReq = m_pReq->next; // 0;
    m_RequestQueueMutex.release();
}

void UserConsole::respondToPending(size_t response, char *buffer, size_t sz)
{
    FATAL("UserConsole::respondToPending is deprecated and needs to go away.");
}

void UserConsole::stopCurrentBlock()
{
    if(!m_RequestQueueSize.getValue())
    {
        m_Stop = 1;
        m_RequestQueueSize.release();
    }
}

size_t UserConsole::nextRequest(size_t responseToLast, char *buffer, size_t *sz, size_t maxBuffSz, size_t *terminalId, bool bAsyncOnly)
{
    FATAL("UserConsole::nextRequest is deprecated and needs to go away.");
    return 0;
}
