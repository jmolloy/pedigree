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

#ifndef TERMINAL_H
#define TERMINAL_H

#include "environment.h"
#include <syslog.h>

#ifndef NEW_XTERM
# include "Xterm.h"
#else
# include "Vt100.h"
#endif

#include <native/graphics/Graphics.h>

/** A Terminal is a wrapper around an Xterm class - it provides UTF-8 <->
    UTF-32 conversion and input queueing. */
class Terminal
{
public:
    friend class Xterm;
    Terminal(char *pName, size_t nWidth, size_t nHeight,
             size_t offsetLeft, size_t offsetTop, rgb_t *pBackground);
    ~Terminal();

    bool initialise();

    /** Verifies whether the terminal and its child are alive. */
    bool isAlive();

    /** Kills and recreates the terminal's buffer, with a change in size. */
    void renewBuffer(size_t nWidth, size_t nHeight);

    /** Adds a 64-bit keycode from the Keyboard class. */
    void processKey(uint64_t key);

    /** Wipes out the queue for this terminal. */
    void clearQueue();

    /** Grabs 1 byte of the utf-8 encoded queue. */
    char getFromQueue();

    /** Returns the queue length. */
    size_t queueLength()
    {return m_Len;}

    /** Gets descriptor to select() on for readability to see if we have pending output. */
    int getSelectFd() const
    {return m_MasterPty;}

    /** Writes the given UTF-8 sequence to the Xterm. */
    void write(const char *pStr, DirtyRectangle &rect);

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

    void setActive(bool b, DirtyRectangle &rect);

    size_t getRows()
    {
        return m_pXterm->getRows();
    }
    size_t getCols()
    {
        return m_pXterm->getCols();
    }

    void redrawAll(DirtyRectangle &rect)
    {
        m_pXterm->renderAll(rect);
    }

    void refresh()
    {
        // Force our buffer to the screen
        if(m_pFramebuffer)
            m_pFramebuffer->redraw(0, 0, m_pFramebuffer->getWidth(), m_pFramebuffer->getHeight(), false);
    }

    rgb_t *getBuffer()
    {
        return m_pBuffer;
    }

    int getPid()
    {
        return m_Pid;
    }

    void showCursor(DirtyRectangle &rect)
    {
        m_pXterm->showCursor(rect);
    }

    void hideCursor(DirtyRectangle &rect)
    {
        m_pXterm->hideCursor(rect);
    }

    void setCursorStyle(bool bFilled = true)
    {
        m_pXterm->setCursorStyle(bFilled);
    }

    /** Cancels the current write operation (used by SIGINT handling) */
    void cancel()
    {
        if(m_WriteInProgress)
        {
            m_Cancel = 1;
        }
    }

private:
    Terminal(const Terminal &);
    Terminal &operator = (const Terminal &);

    void addToQueue(char c, bool bFlush = false);

    rgb_t *m_pBuffer;
    
    PedigreeGraphics::Framebuffer *m_pFramebuffer;
#ifndef NEW_XTERM
    Xterm *m_pXterm;
#else
    Vt100 *m_pXterm;
#endif

    char m_pName[256];

    char m_pQueue[256];
    size_t m_Len;

    char m_pWriteBuffer[4];
    size_t m_WriteBufferLen;

    bool m_bHasPendingRequest;
    size_t m_PendingRequestSz;

    int m_Pid;

    int m_MasterPty;

    size_t m_OffsetLeft, m_OffsetTop;

    volatile char m_Cancel;
    volatile bool m_WriteInProgress;
};

#endif
