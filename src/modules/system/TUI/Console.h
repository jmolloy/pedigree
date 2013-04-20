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
#ifndef SUBSYS_CONSOLE_H
#define SUBSYS_CONSOLE_H

#include <console/Console.h>

class UserConsole : public RequestQueue
{
public:
    UserConsole();
    ~UserConsole();

    /** Block and wait for a request. */
    size_t nextRequest(size_t responseToLast, char *buffer, size_t *sz, size_t buffsz, size_t *tabId, bool bAsyncOnly = false);

    /** Set the currently executing request as "pending" - its value will be
        set later. */
    void requestPending();

    /** Respond to the previously set pending request. */
    void respondToPending(size_t response, char *buffer, size_t sz);

    /** Stops the current block for a request, if necessary */
    void stopCurrentBlock();

private:
    virtual uint64_t executeRequest(uint64_t p1, uint64_t p2, uint64_t p3, uint64_t p4, uint64_t p5, uint64_t p6, uint64_t p7, uint64_t p8)
    {return 0;}

    UserConsole(const UserConsole&);
    const UserConsole& operator = (const UserConsole&);

    Request *m_pReq;
    Request *m_pPendingReq;
};

#endif
