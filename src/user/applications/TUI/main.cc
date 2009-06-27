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

#include "environment.h"
#include <syscall.h>

#include <signal.h>

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

struct TerminalList
{
    Terminal *term;
    TerminalList *next, *prev;
};

size_t sz;
TerminalList *g_pTermList = 0;
TerminalList *g_pCurrentTerm = 0;
Header *g_pHeader = 0;
size_t g_nWidth, g_nHeight;
size_t nextConsoleNum = 1;

void selectTerminal(TerminalList *pTL, DirtyRectangle &rect)
{
    if (g_pCurrentTerm)
        g_pCurrentTerm->term->setActive(false, rect);

    g_pCurrentTerm = pTL;
    g_pHeader->select(pTL->term->getTabId());
    g_pCurrentTerm->term->setActive(true, rect);

    Syscall::setCurrentConsole(pTL->term->getTabId());

    g_pHeader->render(g_pCurrentTerm->term->getBuffer(), rect);
}

Terminal *addTerminal(const char *name, DirtyRectangle &rect)
{
    size_t h = g_pHeader->getHeight()+1;
    Terminal *pTerm = new Terminal(const_cast<char*>(name), g_nWidth, g_nHeight-h, g_pHeader, 0, h, 0);

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

    g_pHeader->render(g_pCurrentTerm->term->getBuffer(), rect);

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
    size_t fb = Syscall::getFb(&mode);

    g_nWidth = mode.width;
    g_nHeight = mode.height;

    g_NormalFont = new Font(12, "/system/fonts/DejaVuSansMono.ttf", 
                            true, g_nWidth);

    g_BoldFont = new Font(12, "/system/fonts/DejaVuSansMono-Bold.ttf", 
                          true, g_nWidth);


    void *pFb = reinterpret_cast<void*>(fb);        

    g_pHeader =  new Header(g_nWidth);

    g_pHeader->addTab("Welcome to the Pedigree operating system.", 0);

    DirtyRectangle rect;
    Terminal *pCurrentTerminal = addTerminal("Console0", rect);

    rect.point(0, 0);
    rect.point(g_nWidth, g_nHeight);
    
    Syscall::updateBuffer(pCurrentTerminal->getBuffer(), rect);

    size_t maxBuffSz = 32678*2-1;
    char *buffer = new char[maxBuffSz+1];
    char str[64];
    size_t lastResponse = 0;
    size_t tabId;
    while (true)
    {
        size_t cmd = Syscall::nextRequest(lastResponse, buffer, &sz, maxBuffSz, &tabId);
//        sprintf(str, "Command %d received. (term %d, sz %d)", cmd, tabId, sz);
//       log(str);

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
            Syscall::requestPending();
            continue;
        }
        
        switch(cmd)
        {
            case CONSOLE_WRITE:
            {
                DirtyRectangle rect2;
                buffer[sz] = '\0';
                buffer[maxBuffSz] = '\0';
                pT->write(buffer, rect2);
                lastResponse = sz;
                Syscall::updateBuffer(g_pCurrentTerm->term->getBuffer(), rect2);
                break;
            }

            case TUI_CHAR_RECV:
            {
                DirtyRectangle rect2;
                uint64_t c = * reinterpret_cast<uint64_t*>(buffer);

                if (c == '\n') c = '\r';

                // CTRL + key, here only for quick and easy testing
                /// \todo Move into its own function to properly handle (along with proper special character handling)
                /****** NOTE: BELOW IS TEMPORARY, WILL MOVE ******/
                if (c & Keyboard::Ctrl)
                {
                    log("CTRL+key");
                    c &= 0x1F;

                    if(c < 0x1F)
                    {
                        log("Control key passed");
                        if(c == 0x3)
                        {
                            log("Sending SIGINT");
                            kill(g_pCurrentTerm->term->getPid(), SIGINT);
                            log("Just sent SIGINT");
                        }

                        break;
                    }
                }
                /****** NOTE: ABOVE IS TEMPORARY, WILL MOVE ******/

                pT->addToQueue(c);
                if(checkCommand(c, rect2))
                    Syscall::updateBuffer(g_pCurrentTerm->term->getBuffer(), rect2);
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
                    Syscall::respondToPending(lastResponse, buffer, sz);
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

