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

#include "environment.h"
#include "Terminal.h"
#include "Header.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#include <graphics/Graphics.h>

extern PedigreeGraphics::Framebuffer *g_pFramebuffer;

extern Header *g_pHeader;
extern rgb_t g_MainBackgroundColour;

Terminal::Terminal(char *pName, size_t nWidth, size_t nHeight, Header *pHeader, size_t offsetLeft, size_t offsetTop, rgb_t *pBackground) :
    m_pBuffer(0), m_pFramebuffer(0), m_pXterm(0), m_Len(0), m_WriteBufferLen(0), m_TabId(0), m_bHasPendingRequest(false),
    m_PendingRequestSz(0), m_Pid(0), m_OffsetLeft(offsetLeft), m_OffsetTop(offsetTop), m_Cancel(0), m_WriteInProgress(0)
{
    // Create a new backbuffer.
    // m_pBuffer = Syscall::newBuffer();
    // if (!m_pBuffer) log("Buffer not created correctly!");

    m_pFramebuffer = g_pFramebuffer->createChild(m_OffsetLeft, m_OffsetTop, nWidth, nHeight);
    if((!m_pFramebuffer) || (!m_pFramebuffer->getRawBuffer()))
        return;

    uint32_t backColour = PedigreeGraphics::createRgb(g_MainBackgroundColour.r, g_MainBackgroundColour.g, g_MainBackgroundColour.b);
    m_pFramebuffer->rect(0, 0, nWidth, nHeight, backColour, PedigreeGraphics::Bits24_Rgb);

    size_t tabId = pHeader->addTab(pName, TAB_SELECTABLE);

    setTabId(tabId);

    strcpy(m_pName, pName);

    Syscall::createConsole(tabId, pName);

#ifndef NEW_XTERM
    m_pXterm = new Xterm(m_pFramebuffer, nWidth, nHeight, m_OffsetLeft, m_OffsetTop, this);
#else
    Display::ScreenMode mode;
    mode.width = nWidth;
    mode.height = nHeight-offsetTop;
    mode.pf.mRed = 0xFF;
    mode.pf.mGreen = 0xFF;
    mode.pf.mBlue = 0xFF;
    mode.pf.pRed = 16;
    mode.pf.pGreen = 8;
    mode.pf.pBlue = 0;
    mode.pf.nBpp = 24;
    mode.pf.nPitch = nWidth*3;
    m_pXterm = new Vt100(mode, reinterpret_cast<uint8_t*>(m_pBuffer)+nWidth*offsetTop);
#endif

    // Fire up a shell session.
    int pid = m_Pid = fork();
    if (pid == 0)
    {
        close(0);
        close(1);
        close(2);
        Syscall::setCtty(pName);
        execl("/applications/login", "/applications/login", 0);
        syslog(LOG_ALERT, "Launching login failed (next line is the error in errno...)");
        syslog(LOG_ALERT, strerror(errno));

        DirtyRectangle rect;
        write("Couldn't load 'login' for this terminal... ", rect);
        write(strerror(errno), rect);
        write("\r\n\r\nYour installation of Pedigree may not be complete, or you may have hit a bug.", rect);
        redrawAll(rect);
    }

    m_Pid = pid;
}

Terminal::~Terminal()
{
    delete m_pXterm;
}

void Terminal::renewBuffer(size_t nWidth, size_t nHeight)
{
    if(!m_pFramebuffer)
        return;

    delete m_pFramebuffer;

    m_pFramebuffer = g_pFramebuffer->createChild(m_OffsetLeft, m_OffsetTop, nWidth, nHeight);
    if((!m_pFramebuffer) || (!m_pFramebuffer->getRawBuffer()))
        return;

    uint32_t backColourInt = PedigreeGraphics::createRgb(g_MainBackgroundColour.r, g_MainBackgroundColour.g, g_MainBackgroundColour.b);
    m_pFramebuffer->rect(0, 0, nWidth, nHeight, backColourInt, PedigreeGraphics::Bits24_Rgb);

    m_pXterm->resize(nWidth, nHeight, m_pFramebuffer);
}

void Terminal::addToQueue(uint64_t key)
{
    if (key & Keyboard::Special)
    {
        // Handle specially.
        uint32_t utf32 = key & ~0UL;
        char *str = reinterpret_cast<char*> (&utf32);
        if (!strncmp(str, "left", 4))
        {
            addToQueue('\e');
            addToQueue('[');
            addToQueue('D');
        }
        if (!strncmp(str, "righ", 4))
        {
            addToQueue('\e');
            addToQueue('[');
            addToQueue('C');
        }
        if (!strncmp(str, "up", 2))
        {
            addToQueue('\e');
            addToQueue('[');
            addToQueue('A');
        }
        if (!strncmp(str, "down", 4))
        {
            addToQueue('\e');
            addToQueue('[');
            addToQueue('B');
        }

    }
    else if (key & Keyboard::Alt)
    {
        // Xterm ALT escape = ESC <char>
        addToQueue('\e');
        addToQueue(key&0x7F);
    }
    else
    {
        uint32_t utf32 = key&0xFFFFFFFF;

        // Begin Utf-32 -> Utf-8 conversion.
        char buf[4];
        size_t nbuf = 0;
        if (utf32 <= 0x7F)
        {
            buf[0] = utf32&0x7F;
            nbuf = 1;
        }
        else if (utf32 <= 0x7FF)
        {
            buf[0] = 0xC0 | ((utf32>>6) & 0x1F);
            buf[1] = 0x80 | (utf32 & 0x3F);
            nbuf = 2;
        }
        else if (utf32 <= 0xFFFF)
        {
            buf[0] = 0xE0 | ((utf32>>12) & 0x0F);
            buf[1] = 0x80 | ((utf32>>6) & 0x3F);
            buf[2] = 0x80 | (utf32 & 0x3F);
            nbuf = 3;
        }
        else if (utf32 <= 0x10FFFF)
        {
            buf[0] = 0xE0 | ((utf32>>18) & 0x07);
            buf[1] = 0x80 | ((utf32>>12) & 0x3F);
            buf[2] = 0x80 | ((utf32>>6) & 0x3F);
            buf[3] = 0x80 | (utf32 & 0x3F);
            nbuf = 4;
        }
        // End Utf-32 -> Utf-8 conversion.
        for (size_t i = 0; i < nbuf; i++)
            addToQueue(buf[i]);
    }
}

char Terminal::getFromQueue()
{
    if (m_Len > 0)
    {
        char c = m_pQueue[0];
        for (size_t i = 0; i < m_Len-1; i++)
            m_pQueue[i] = m_pQueue[i+1];
        m_Len--;
        return c;
    }
    else
        return 0;
}


void Terminal::clearQueue()
{
    m_Len = 0;
}

#include <syslog.h>
void Terminal::write(char *pStr, DirtyRectangle &rect)
{
    m_pXterm->hideCursor(rect);

    bool bWasAlreadyRunning = m_WriteInProgress;
    m_WriteInProgress = true;
    //syslog(LOG_NOTICE, "Beginning write...");
    while (!m_Cancel && (*pStr || m_WriteBufferLen))
    {
        // Fill the buffer.
        while (*pStr && !m_Cancel)
        {
            if (m_WriteBufferLen < 4)
                m_pWriteBuffer[m_WriteBufferLen++] = *pStr++;
            else
                break;
        }
        if(m_Cancel) // Check break point from above loop
            break;
        // Begin UTF-8 -> UTF-32 conversion.
        /// \todo Add some checking - every successive byte should start with 0b10.
        uint32_t utf32;
        size_t nBytes;
        if ( (m_pWriteBuffer[0] & 0x80) == 0x00 )
        {
            utf32 = static_cast<uint32_t>(m_pWriteBuffer[0]);
            nBytes = 1;
        }
        else if ( (m_pWriteBuffer[0] & 0xE0) == 0xC0 )
        {
            if (m_WriteBufferLen < 2) return;
            utf32 = ((static_cast<uint32_t>(m_pWriteBuffer[0])&0x1F) << 6) |
                (static_cast<uint32_t>(m_pWriteBuffer[1])&0x3F);
            nBytes = 2;
        }
        else if ( (m_pWriteBuffer[0] & 0xF0) == 0xE0 )
        {
            if (m_WriteBufferLen < 3) return;
            utf32 = ((static_cast<uint32_t>(m_pWriteBuffer[0])&0x0F) << 12) |
                ((static_cast<uint32_t>(m_pWriteBuffer[1])&0x3F) << 6) |
                (static_cast<uint32_t>(m_pWriteBuffer[2])&0x3F);
            nBytes = 3;
        }
        else if ( (m_pWriteBuffer[0] & 0xF8) == 0xF0 )
        {
            if (m_WriteBufferLen < 4) return;
            utf32 = ((static_cast<uint32_t>(m_pWriteBuffer[0])&0x0F) << 18) |
                ((static_cast<uint32_t>(m_pWriteBuffer[1])&0x3F) << 12) |
                ((static_cast<uint32_t>(m_pWriteBuffer[2])&0x3F)<<6) |
                (static_cast<uint32_t>(m_pWriteBuffer[3])&0x3F);
            nBytes = 4;
        }
        else
        {
            m_WriteBufferLen = 0;
            nBytes = 1;
            continue;
        }

        memmove(m_pWriteBuffer, &m_pWriteBuffer[nBytes], 4-nBytes);
        m_WriteBufferLen -= nBytes;

        // End UTF-8 -> UTF-32 conversion.
#ifndef NEW_XTERM
        m_pXterm->write(utf32, rect);
#else
        rect.point(m_OffsetLeft,m_OffsetTop);
        rect.point(m_pXterm->getCols()*8+m_OffsetLeft, m_pXterm->getRows()*16+m_OffsetTop);
        m_pXterm->write(static_cast<uint8_t>(utf32&0xFF));
#endif
    }
    //syslog(LOG_NOTICE, "Completed write [%scancelled]...", m_Cancel ? "" : "not ");

    if(!bWasAlreadyRunning)
    {
        if(m_Cancel)
            m_Cancel = 0;
        m_WriteInProgress = false;
    }

    m_pXterm->showCursor(rect);
}

void Terminal::addToQueue(char c)
{
    // Don't allow keys to be pressed past the buffer's size
    if(m_Len >= 256)
        return;
    m_pQueue[m_Len++] = c;

    // Key available in queue, we should notify the upper layers.
    Syscall::dataAvailable();
}

void Terminal::setActive(bool b, DirtyRectangle &rect)
{
    // Force complete redraw
    m_pFramebuffer->redraw(0, 0, m_pFramebuffer->getWidth(), m_pFramebuffer->getHeight(), false);

    // if (b)
    //    Syscall::setCurrentBuffer(m_pBuffer);
}
