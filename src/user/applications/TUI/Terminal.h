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

#ifndef TERMINAL_H
#define TERMINAL_H

#include "environment.h"

#ifndef NEW_XTERM
# include "_Xterm.h"
#else
# include "Vt100.h"
#endif

/** A Terminal is a wrapper around an Xterm class - it provides UTF-8 <->
    UTF-32 conversion and input queueing. */
class Terminal
{
public:
    friend class Xterm;
    Terminal(char *pName, size_t nWidth, size_t nHeight,
             class Header *pHeader, size_t offsetLeft, size_t offsetTop, rgb_t *pBackground);
    ~Terminal();

    /** Kills and recreates the terminal's buffer, with a change in size. */
    void renewBuffer(size_t nWidth, size_t nHeight);
    
    /** Adds a 64-bit keycode from the Keyboard class. */
    void addToQueue(uint64_t key);
    
    /** Grabs 1 byte of the utf-8 encoded queue. */
    char getFromQueue();

    /** Returns the queue length. */
    size_t queueLength()
    {return m_Len;}

    /** Writes the given UTF-8 sequence to the Xterm. */
    void write(char *pStr, DirtyRectangle &rect);

    void setHasPendingRequest(bool b, size_t sz)
    {
        m_bHasPendingRequest = b;
        m_PendingRequestSz = sz;
    }

    bool hasPendingRequest()
    {
        return m_bHasPendingRequest;
    }
    size_t getPendingRequestSz()
    {
        return m_PendingRequestSz;
    }

    size_t getTabId()
    {return m_TabId;}

    void setTabId(size_t tabId)
    {m_TabId = tabId;}

    void setActive(bool b, DirtyRectangle &rect);

    size_t getRows()
    {
        return m_pXterm->getRows();
    }
    size_t getCols()
    {
        return m_pXterm->getCols();
    }

    rgb_t *getBuffer()
    {
        return m_pBuffer;
    }

    int getPid()
    {
        return m_Pid;
    }

    /** Cancels the current write operation (used by SIGINT handling) */
    void cancel()
    {
        if(m_WriteInProgress)
            m_Cancel = 1;
    }

private:
    Terminal(const Terminal &);
    Terminal &operator = (const Terminal &);

    void addToQueue(char c);


    rgb_t *m_pBuffer;
#ifndef NEW_XTERM
    Xterm *m_pXterm;
#else
    Vt100 *m_pXterm;
#endif

    char m_pQueue[256];
    size_t m_Len;

    char m_pWriteBuffer[4];
    size_t m_WriteBufferLen;

    size_t m_TabId;

    bool m_bHasPendingRequest;
    size_t m_PendingRequestSz;

    int m_Pid;

    size_t m_OffsetLeft, m_OffsetTop;

    int m_Cancel;
    bool m_WriteInProgress;
};

#endif
