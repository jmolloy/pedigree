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

#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <syscall.h>

//include "Xterm.h"
//include "FreetypeXterm.h"

#include "Terminal.h"

#include "Font.h"
#include "Png.h"
#include "Header.h"

#define CONSOLE_READ    1
#define CONSOLE_WRITE   2
#define CONSOLE_GETROWS 3
#define CONSOLE_GETCOLS 4
#define CONSOLE_DATA_AVAILABLE 5
#define TUI_CHAR_RECV   99

void log(char *c)
{
    syscall1(TUI_LOG, reinterpret_cast<size_t>(c));
}

void log(const char *c)
{
    syscall1(TUI_LOG, reinterpret_cast<size_t>(c));
}

size_t nextRequest(size_t responseToLast, char *buffer, size_t *sz, size_t buffersz, size_t *terminalId)
{
    size_t ret = syscall5(TUI_NEXT_REQUEST, responseToLast, reinterpret_cast<size_t>(buffer), reinterpret_cast<size_t>(sz), buffersz, reinterpret_cast<size_t>(terminalId));
    // Memory barrier, "sz" will have changed. Reload.
    asm volatile ("" : : : "memory");
    return ret;
}

size_t getFb(Display::ScreenMode *pMode)
{
    return syscall1(TUI_GETFB, reinterpret_cast<size_t>(pMode));
    // Memory barrier, "sz" will have changed. Reload.
    asm volatile ("" : : : "memory");
}

void requestPending()
{
    syscall0(TUI_REQUEST_PENDING);
}

void respondToPending(size_t response, char *buffer, size_t sz)
{
    syscall3(TUI_RESPOND_TO_PENDING, response, reinterpret_cast<size_t>(buffer), sz);
}

void createConsole(size_t tabId, char *pName)
{
    syscall2(TUI_CREATE_CONSOLE, tabId, reinterpret_cast<int>(pName));
}

void setCtty(char *pName)
{
    syscall1(TUI_SET_CTTY, reinterpret_cast<int>(pName));
}

void setCurrentConsole(size_t tabId)
{
    syscall1(TUI_SET_CURRENT_CONSOLE, tabId);
}

struct TerminalList
{
    Terminal *term;
    TerminalList *next, *prev;
};

size_t sz;
TerminalList *g_pTermList = 0;
TerminalList *g_pCurrentTerm = 0;
Header *pHeader = 0;
rgb_t *pBackBuffer;
rgb_t *pBackground;
size_t nWidth, nHeight;
size_t nextConsoleNum = 1;

void selectTerminal(TerminalList *pTL, DirtyRectangle &rect)
{
    if (g_pCurrentTerm)
        g_pCurrentTerm->term->setActive(false, rect);

    g_pCurrentTerm = pTL;
    pHeader->select(pTL->term->getTabId());
    g_pCurrentTerm->term->setActive(true, rect);

    setCurrentConsole(pTL->term->getTabId());

    pHeader->render(rect);
}

Terminal *addTerminal(const char *name, DirtyRectangle &rect)
{
    size_t h = pHeader->getHeight()+1;
    Terminal *pTerm = new Terminal(const_cast<char*>(name), pBackBuffer, nWidth, nHeight-h, pHeader, 0, h, pBackground);

    TerminalList *pTermList = new TerminalList;
    pTermList->term = pTerm;
    pTermList->next = pTermList->prev = 0;

    if (g_pTermList == 0)
        g_pTermList = pTermList;
    else
    {
        TerminalList *_pTermList = g_pTermList;
        while (_pTermList->next)
            _pTermList = _pTermList->next;
        _pTermList->next = pTermList;
        pTermList->prev = _pTermList;
    }

    selectTerminal(pTermList, rect);

    return pTerm;
}

bool checkCommand(uint64_t key, DirtyRectangle &rect)
{
    if ( (key & Keyboard::Ctrl) &&
         (key & Keyboard::Shift) &&
         (key & Keyboard::Special) )
    {
        uint32_t k = key&0xFFFFFFFFULL;
        char str[5];
        memcpy(str, (char*)&k, 4);
        str[4] = 0;

        if (!strcmp(str, "left"))
        {
            if (g_pCurrentTerm->prev)
            {
                selectTerminal(g_pCurrentTerm->prev, rect);
                return true;
            }
        }
        else if (!strcmp(str, "righ"))
        {
            if (g_pCurrentTerm->next)
            {
                selectTerminal(g_pCurrentTerm->next, rect);
                return true;
            }
        }
        else if (!strcmp(str, "ins"))
        {
            // Create a new terminal.
            char *pStr = new char[64];
            sprintf(pStr, "Console%d", nextConsoleNum++);
            Terminal *pT = addTerminal(pStr, rect);
            return true;
        }
    }
    return false;
}

Font *g_NormalFont;
Font *g_BoldFont;

int main (int argc, char **argv)
{
    Display::ScreenMode mode;
    size_t fb = getFb(&mode);

    rgb_t *pBg = new rgb_t[1024*768];
    pBackground = pBg;
    memset(reinterpret_cast<uint8_t*>(pBg), 0x00, 1024*768*3);

    rgb_t *pBuffer = new rgb_t[1024*768];
    memset(reinterpret_cast<uint8_t*>(pBuffer), 0x00, 1024*768*3);

    pBackBuffer = pBuffer;
    nWidth = 1024;
    nHeight = 768;

    rgb_t *pPrevState = new rgb_t[1024*768];
    memset(reinterpret_cast<uint8_t*>(pPrevState), 0x00, 1024*768*3);

    //Png *png = new Png("/background.png");
    //png->render(pBg, 1024-png->getWidth(), 768-png->getHeight(), 1024, 768);

    FILE *stream = fopen("/background.png", "rb");
    if (!stream)
        log("Stream is null");
    else
    {
        if (fseek(stream, 0, SEEK_END) != 0)
        {
            log("fseek failed, 1");
        }
        int a = ftell(stream);
        if (fseek(stream, 0, SEEK_SET) != 0)
        {
            log("fseek failed, 2");
        }

        struct stat st;
        fstat(stream->_file, &st);

        char str2[64];
        sprintf(str2, "tell: %x, %x\n", a, st.st_size);
        log(str2);
    }

    g_NormalFont = new Font(12, "/system/fonts/DejaVuSansMono.ttf", 
                            false, 1024);

    g_BoldFont = new Font(12, "/system/fonts/DejaVuSansMono-Bold.ttf", 
                          false, 1024);


    void *pFb = reinterpret_cast<void*>(fb);        

    pHeader =  new Header(pBuffer, 1024);
    
    pHeader->addTab("Welcome to the Pedigree operating system.", 0);

    DirtyRectangle rect;
    Terminal *pCurrentTerminal = addTerminal("Console0", rect);
//    pCurrentTerminal->setActive(true);
//    pHeader->select(pCurrentTerminal->getTabId());

    pHeader->render(rect);

    rect.point(0, 0);
    rect.point(nWidth-1, nHeight-1);
    
    swapBuffers(pFb, pBuffer, pPrevState, 1024, mode.pf, rect);

    char *buffer = new char[2048];
    char str[64];
    size_t lastResponse = 0;
    size_t tabId;
    while (true)
    {
        size_t cmd = nextRequest(lastResponse, buffer, &sz, 2047, &tabId);
        sprintf(str, "Command %d received. (term %d)", cmd, tabId);
        log(str);

        Terminal *pT = 0;
        TerminalList *pTL = g_pTermList;
        while (pTL)
        {
            if (pTL->term->getTabId() == tabId)
            {
                pT = pTL->term;
                break;
            }
            pTL = pTL->next;
        }
        if (pT == 0)
        {
            sprintf(str, "Completely unrecognised terminal ID %d, aborting.", tabId);
            log (str);
            continue;
        }
        
        // If the current terminal's queue is empty, set the request
        // pending further input.
        if (cmd == CONSOLE_READ && pT->queueLength() == 0)
        {
            pT->setHasPendingRequest(true, sz);
            requestPending();
            continue;
        }
        
        switch(cmd)
        {
            case CONSOLE_WRITE:
            {
                DirtyRectangle rect2;
                buffer[sz] = '\0';
                pT->write(buffer, rect2);
                lastResponse = sz;
                swapBuffers(pFb, pBuffer, pPrevState, 1024, mode.pf, rect2);
                break;
            }

            case TUI_CHAR_RECV:
            {
                DirtyRectangle rect2;
                uint64_t c = * reinterpret_cast<uint64_t*>(buffer);

                if (c == '\n') c = '\r';

                pT->addToQueue(c);
                if(checkCommand(c, rect2))
                    swapBuffers(pFb, pBuffer, pPrevState, 1024, mode.pf, rect2);
                if (!pT->hasPendingRequest() || pT->queueLength() == 0)
                    break;
                sz = pT->getPendingRequestSz();
                // Fall through.
            }

            case CONSOLE_READ:
            {
                char str[64];
                size_t i = 0;
                while (i < sz)
                {
                    char c = pT->getFromQueue();
                    if (c)
                    {
                        sprintf(str, "Read: %x", c);
                        log(str);
                        buffer[i++] = c;
                        continue;
                    }
                    else break;
                }
                lastResponse = i;
                if (pT->hasPendingRequest())
                {
                    respondToPending(lastResponse, buffer, sz);
                    pT->setHasPendingRequest(false, 0);
                }
                break;
            }

            case CONSOLE_DATA_AVAILABLE:
                lastResponse = (pT->queueLength() > 0);
                break;

            case CONSOLE_GETROWS:
                lastResponse = pT->getRows();
                break;

            case CONSOLE_GETCOLS:
                lastResponse = pT->getCols();
                break;

            default:
                log("Unknown command.");
        }

    }

    return 0;
}

