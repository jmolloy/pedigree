/*
 * 
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

#include "environment.h"
#include "Terminal.h"
#include "Header.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/wait.h>

#include <graphics/Graphics.h>

extern PedigreeGraphics::Framebuffer *g_pFramebuffer;

extern Header *g_pHeader;
extern rgb_t g_MainBackgroundColour;

extern cairo_t *g_Cairo;

Terminal::Terminal(char *pName, size_t nWidth, size_t nHeight, Header *pHeader, size_t offsetLeft, size_t offsetTop, rgb_t *pBackground) :
    m_pBuffer(0), m_pFramebuffer(0), m_pXterm(0), m_Len(0), m_WriteBufferLen(0), m_TabId(0), m_bHasPendingRequest(false),
    m_PendingRequestSz(0), m_Pid(0), m_OffsetLeft(offsetLeft), m_OffsetTop(offsetTop), m_Cancel(0), m_WriteInProgress(0)
{
    cairo_save(g_Cairo);
    cairo_set_operator(g_Cairo, CAIRO_OPERATOR_SOURCE);

    cairo_set_source_rgba(
            g_Cairo,
            g_MainBackgroundColour.r / 256.0,
            g_MainBackgroundColour.g / 256.0,
            g_MainBackgroundColour.b / 256.0,
            0.8);

    cairo_rectangle(g_Cairo, m_OffsetLeft, m_OffsetTop, nWidth, nHeight);
    cairo_fill(g_Cairo);

    cairo_restore(g_Cairo);

    size_t tabId = pHeader->addTab(pName, TAB_SELECTABLE);

    setTabId(tabId);

    strcpy(m_pName, pName);

#ifndef NEW_XTERM
    m_pXterm = new Xterm(0, nWidth, nHeight, m_OffsetLeft, m_OffsetTop, this);
#else
    Display::ScreenMode mode;
    mode.width = nWidth - 1;
    mode.height = nHeight-offsetTop - 1;
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
}

bool Terminal::initialise()
{
    m_MasterPty = posix_openpt(O_RDWR);
    if(m_MasterPty < 0)
    {
        syslog(LOG_INFO, "TUI: Couldn't create terminal: %s", strerror(errno));
        return false;
    }

    char slavename[16] = {0};
    strcpy(slavename, ptsname(m_MasterPty));

    struct winsize ptySize;
    ptySize.ws_row = m_pXterm->getRows();
    ptySize.ws_col = m_pXterm->getCols();
    ioctl(m_MasterPty, TIOCSWINSZ, &ptySize);

    // Fire up a shell session.
    int pid = m_Pid = fork();
    if (pid == 0)
    {
        close(0);
        close(1);
        close(2);

        // Open the slave terminal, this will also set it as our ctty
        int slave = open(slavename, O_RDWR);
        dup2(slave, 1);
        dup2(slave, 2);

        // We emulate an xterm, so ensure that's set in the environment.
        setenv("TERM", "xterm", 1);

        execl("/applications/login", "/applications/login", 0);
        syslog(LOG_ALERT, "Launching login failed (next line is the error in errno...)");
        syslog(LOG_ALERT, strerror(errno));

        DirtyRectangle rect;
        write("Couldn't load 'login' for this terminal... ", rect);
        write(strerror(errno), rect);
        write("\r\n\r\nYour installation of Pedigree may not be complete, or you may have hit a bug.", rect);
        redrawAll(rect);

        exit(1);
    }

    m_Pid = pid;

    return true;
}

Terminal::~Terminal()
{
    // Kill child.
    kill(m_Pid, SIGTERM);

    // Reap child.
    waitpid(m_Pid, 0, 0);

    delete m_pXterm;
}

void Terminal::renewBuffer(size_t nWidth, size_t nHeight)
{
    m_pXterm->resize(nWidth, nHeight, 0);

    /// \todo Send SIGWINCH in console layer.
    struct winsize ptySize;
    ptySize.ws_row = m_pXterm->getRows();
    ptySize.ws_col = m_pXterm->getCols();
    ioctl(m_MasterPty, TIOCSWINSZ, &ptySize);
}

void Terminal::processKey(uint64_t key)
{
    m_pXterm->processKey(key);
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

void Terminal::write(const char *pStr, DirtyRectangle &rect)
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

void Terminal::addToQueue(char c, bool bFlush)
{
    // Don't allow keys to be pressed past the buffer's size
    if(m_Len >= 256)
        return;
    if((c == 0) && (!m_Len))
        return;

    if(c)
        m_pQueue[m_Len++] = c;

    if(bFlush)
    {
        ::write(m_MasterPty, m_pQueue, m_Len);
        m_Len = 0;
    }
}

void Terminal::setActive(bool b, DirtyRectangle &rect)
{
    // Force complete redraw
    // m_pFramebuffer->redraw(0, 0, m_pFramebuffer->getWidth(), m_pFramebuffer->getHeight(), false);

    // if (b)
    //    Syscall::setCurrentBuffer(m_pBuffer);
}
