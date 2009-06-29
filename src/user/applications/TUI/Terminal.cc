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

extern Header *g_pHeader;

Terminal::Terminal(char *pName, size_t nWidth, size_t nHeight, Header *pHeader, size_t offsetLeft, size_t offsetTop, rgb_t *pBackground) :
    m_pBuffer(0), m_pXterm(0), m_Len(0), m_WriteBufferLen(0), m_TabId(0), m_bHasPendingRequest(false),
    m_PendingRequestSz(0), m_Pid(0)
{
    // Create a new backbuffer.
    m_pBuffer = Syscall::newBuffer();
    if (!m_pBuffer) log("Buffer not created correctly!");

//    char str[64];
//    sprintf(str, "adTab %x", pHeader);
    //log(str);

    size_t tabId = pHeader->addTab(pName, TAB_SELECTABLE);
    log("After addTab");
    setTabId(tabId);

    Syscall::createConsole(tabId, pName);

    m_pXterm = new Xterm(m_pBuffer, nWidth, nHeight, offsetLeft, offsetTop, pBackground);

    // Fire up a shell session.
    int pid = m_Pid = fork();
    if (pid == 0)
    {
        close(0);
        close(1);
        close(2);
        Syscall::setCtty(pName);
        execl("/applications/login", "/applications/login", 0);
        log("Launching login failed!");
    }

    m_Pid = pid;

}

Terminal::~Terminal()
{
}

void Terminal::addToQueue(uint64_t key)
{
    if (key & Keyboard::Special)
    {
        // Handle specially.
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

void Terminal::write(char *pStr, DirtyRectangle &rect)
{
    while (*pStr || m_WriteBufferLen)
    {
        // Fill the buffer.
        while (*pStr)
        {
            if (m_WriteBufferLen < 4)
                m_pWriteBuffer[m_WriteBufferLen++] = *pStr++;
            else
                break;
        }
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
            char buf[16];
            sprintf(buf, "utf32: %x", utf32);
            log(buf);
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
            nBytes = 1;
            continue;
        }

        memmove(m_pWriteBuffer, &m_pWriteBuffer[nBytes], 4-nBytes);
        m_WriteBufferLen -= nBytes;

        // End UTF-8 -> UTF-32 conversion.
        m_pXterm->write(utf32, rect);
    }
}

void Terminal::addToQueue(char c)
{
    m_pQueue[m_Len++] = c;
}

void Terminal::setActive(bool b, DirtyRectangle &rect)
{
    m_pXterm->setActive(b);
    if (b)
        Syscall::setCurrentBuffer(m_pBuffer);
}
