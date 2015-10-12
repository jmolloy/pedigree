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

#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>
#include <errno.h>
#include <sched.h>

#include "environment.h"
#include <syslog.h>

#include <pthread.h>

#include <signal.h>

#include "Terminal.h"

#include "Font.h"
#include "Png.h"
#include "Header.h"

#include <graphics/Graphics.h>
#include <input/Input.h>

#include <Widget.h>

#include <cairo/cairo.h>

#define CONSOLE_READ    1
#define CONSOLE_WRITE   2
#define CONSOLE_GETROWS 3
#define CONSOLE_GETCOLS 4
#define CONSOLE_DATA_AVAILABLE 5
#define CONSOLE_REFRESH 10
#define CONSOLE_FLUSH   11

#if 0
#ifdef TARGET_LINUX
#define NORMAL_FONT_PATH    "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf"
#define BOLD_FONT_PATH      "/usr/share/fonts/truetype/dejavu/DejaVuSansMono-Bold.ttf"
#else
#define NORMAL_FONT_PATH    "/system/fonts/DejaVuSansMono.ttf"
#define BOLD_FONT_PATH      "/system/fonts/DejaVuSansMono-Bold.ttf"
#endif
#endif

#define FONT_SIZE           14

#define NORMAL_FONT_PATH    "DejaVu Sans Mono 10"
#define BOLD_FONT_PATH      "DejaVu Sans Mono 10"

/** End code from InputManager */

struct TerminalList
{
    Terminal *term;
    TerminalList *next, *prev;
};

size_t sz;
TerminalList * volatile g_pTermList = 0;
TerminalList * volatile g_pCurrentTerm = 0;
size_t g_nWidth, g_nHeight;
size_t nextConsoleNum = 1;
size_t g_nLastResponse = 0;

bool g_KeyPressed = false;
bool g_bRunning = false;

cairo_t *g_Cairo = 0;
cairo_surface_t *g_Surface = 0;

PedigreeTerminalEmulator *g_pEmu = 0;

PedigreeGraphics::Framebuffer *g_pFramebuffer = 0;

void checkFramebuffer()
{
    if(g_pEmu)
    {
        if(g_Surface)
        {
            cairo_surface_destroy(g_Surface);
            cairo_destroy(g_Cairo);
        }

        // Wipe out the framebuffer before we do much with it.
        int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, g_nWidth);
        memset(g_pEmu->getRawFramebuffer(), 0, g_nHeight * stride);

        g_Surface = cairo_image_surface_create_for_data(
                (uint8_t*) g_pEmu->getRawFramebuffer(),
                CAIRO_FORMAT_ARGB32,
                g_nWidth,
                g_nHeight,
                stride);
        g_Cairo = cairo_create(g_Surface);

        syslog(LOG_INFO, "created cairo f=%p s=%p cr=%p %zdx%zd",
                g_pEmu->getRawFramebuffer(), g_Surface, g_Cairo, g_nWidth,
                g_nHeight);
    }
}

void modeChanged(size_t width, size_t height)
{
    if(!(g_pTermList && g_Cairo))
    {
        // Spurious/early modeChanged.
        return;
    }

    syslog(LOG_ALERT, "w: %zd, h: %zd", width, height);

    g_nWidth = width;
    g_nHeight = height;

    // Wipe out the framebuffer, start over.
    cairo_set_operator(g_Cairo, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_rgba(g_Cairo, 0, 0, 0, 0.8);
    cairo_rectangle(g_Cairo, 0, 0, g_nWidth, g_nHeight);
    cairo_fill(g_Cairo);

    TerminalList *pTL = g_pTermList;
    while (pTL)
    {
        Terminal *pTerm = pTL->term;

        // Kill and renew the buffers.
        pTerm->renewBuffer(width, height);

        DirtyRectangle rect;
        pTerm->redrawAll(rect);
        pTerm->showCursor(rect);

        // Resize any clients.
        kill(pTerm->getPid(), SIGWINCH);

        pTL = pTL->next;
    }

    PedigreeGraphics::Rect rt(0, 0, g_nWidth, g_nHeight);
    g_pEmu->redraw(rt);
}

void selectTerminal(TerminalList *pTL, DirtyRectangle &rect)
{
    if (g_pCurrentTerm)
        g_pCurrentTerm->term->setActive(false, rect);

    g_pCurrentTerm = pTL;

    g_pCurrentTerm->term->setActive(true, rect);

    pTL->term->redrawAll(rect);

    doRedraw(rect);
}

Terminal *addTerminal(const char *name, DirtyRectangle &rect)
{
    size_t h = 0;

    Terminal *pTerm = new Terminal(const_cast<char*>(name), g_nWidth - 3, g_nHeight-h, 3, h, 0);
    if(!pTerm->initialise())
    {
        delete pTerm;
        return 0;
    }

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

Font *g_NormalFont = NULL;
Font *g_BoldFont = NULL;

void sigint(int)
{
    syslog(LOG_NOTICE, "TUI sent SIGINT, oops!");
}

void key_input_handler(uint64_t c)
{
    /** Add the key to the terminal queue */
    if(!g_pCurrentTerm)
        return;

    Terminal *pT = g_pCurrentTerm->term;

    // CTRL + key -> unprintable characters
    if ((c & Keyboard::Ctrl) && !(c & Keyboard::Special))
    {
        c &= 0x1F;
    }

    pT->processKey(c);
}

/**
 * This is the TUI input handler. It is registered with the kernel at startup
 * and handles every keypress that occurs, via an Event sent from the kernel's
 * InputManager object.
 */
void input_handler(Input::InputNotification &note)
{
    if(!g_pCurrentTerm || !g_pCurrentTerm->term) // No terminal yet!
        return;

    if(note.type != Input::Key)
        return;

    uint64_t c = note.data.key.key;
    key_input_handler(c);
}

int tui_do(PedigreeGraphics::Framebuffer *pFramebuffer)
{
    syslog(LOG_INFO, "TUI: preparing for main loop");

    g_pFramebuffer = pFramebuffer;

    g_nWidth = g_pEmu->getWidth();
    g_nHeight = g_pEmu->getHeight();

    if (!g_Cairo)
    {
        syslog(LOG_ALERT, "TUI: g_Cairo is not yet valid!");
        return 1;
    }

    cairo_set_line_cap(g_Cairo, CAIRO_LINE_CAP_SQUARE);
    cairo_set_line_join(g_Cairo, CAIRO_LINE_JOIN_MITER);
    cairo_set_antialias(g_Cairo, CAIRO_ANTIALIAS_NONE);
    cairo_set_line_width(g_Cairo, 1.0);

    cairo_set_operator(g_Cairo, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_rgba(g_Cairo, 0, 0, 0, 1.0);
    cairo_paint(g_Cairo);

    g_NormalFont = new Font(FONT_SIZE, NORMAL_FONT_PATH,
                            true, g_nWidth);
    if (!g_NormalFont)
    {
        syslog(LOG_EMERG, "Error: Font '%s' not loaded!", NORMAL_FONT_PATH);
        return 0;
    }
    g_BoldFont = new Font(FONT_SIZE, BOLD_FONT_PATH,
                          true, g_nWidth);
    if (!g_BoldFont)
    {
        syslog(LOG_EMERG, "Error: Font '%s' not loaded!", BOLD_FONT_PATH);
        return 0;
    }

    DirtyRectangle rect;
    char newTermName[256];
    sprintf(newTermName, "Console%d", getpid());
    Terminal *pCurrentTerminal = addTerminal(newTermName, rect);
    rect.point(0, 0);
    rect.point(g_nWidth, g_nHeight);

    if(!pCurrentTerminal)
    {
        syslog(LOG_ALERT, "TUI: couldn't start up a terminal - failing gracefully...");
        g_BoldFont->render("There are no pseudo-terminals available.", 5, 5, 0xFFFFFF, 0x000000, false);
        g_BoldFont->render("Press any key to close this window.", 5, g_BoldFont->getHeight() + 5, 0xFFFFFF, 0x000000, false);

        doRedraw(rect);

        g_KeyPressed = false;
        while(!g_KeyPressed)
        {
            Widget::checkForEvents(false);
        }

        return 0;
    }

    doRedraw(rect);

    size_t maxBuffSz = 32768;
    char buffer[32768];
    g_bRunning = true;
    while (g_bRunning)
    {
        int n = 0;

        fd_set fds;
        FD_ZERO(&fds);

        n = std::max(g_pEmu->getSocket(), n);
        FD_SET(g_pEmu->getSocket(), &fds);

        TerminalList *pTL = g_pTermList;
        while (pTL)
        {
            if (!pTL->term->isAlive())
            {
                g_bRunning = false;
                break;
            }

            int fd = pTL->term->getSelectFd();
            FD_SET(fd, &fds);
            n = std::max(fd, n);
            pTL = pTL->next;
        }

        if (!g_bRunning)
            continue;

        int nReady = select(n + 1, &fds, NULL, NULL, 0);

        if(nReady <= 0)
            continue;

        // Check for widget events.
        if(FD_ISSET(g_pEmu->getSocket(), &fds))
        {
            // Dispatch callbacks.
            Widget::checkForEvents(true);

            // Don't do redraw processing if this was the only descriptor
            // that was found readable.
            if(nReady == 1)
                continue;
        }

        bool bShouldRedraw = false;

        DirtyRectangle dirtyRect;
        pTL = g_pTermList;
        while (pTL)
        {
            int fd = pTL->term->getSelectFd();
            if(FD_ISSET(fd, &fds))
            {
                // Something to read.
                ssize_t len = read(fd, buffer, maxBuffSz);
                if (len > 0)
                {
                    buffer[len] = 0;
                    pTL->term->write(buffer, dirtyRect);
                    bShouldRedraw = true;
                }
            }

            pTL = pTL->next;
        }

        if(bShouldRedraw)
            doRedraw(dirtyRect);
    }

    syslog(LOG_INFO, "TUI shutting down cleanly.");

    // Clean up.
    delete pCurrentTerminal;

    delete g_BoldFont;
    delete g_NormalFont;
    g_BoldFont = g_NormalFont = 0;

    cairo_surface_destroy(g_Surface);
    cairo_destroy(g_Cairo);

    return 0;
}

bool callback(WidgetMessages message, size_t msgSize, const void *msgData)
{
    DirtyRectangle dirty;

    switch(message)
    {
        case Reposition:
            {
                syslog(LOG_INFO, "-- REPOSITION --");
                const PedigreeGraphics::Rect *rt = reinterpret_cast<const PedigreeGraphics::Rect*>(msgData);
                syslog(LOG_INFO, " -> handling...");
                g_pEmu->handleReposition(*rt);
                g_nWidth = g_pEmu->getWidth();
                g_nHeight = g_pEmu->getHeight();
                syslog(LOG_INFO, " -> new extents are %zdx%zd", g_nWidth,
                       g_nHeight);
                syslog(LOG_INFO, " -> creating new framebuffer");
                checkFramebuffer();
                syslog(LOG_INFO, " -> registering the mode change");
                modeChanged(rt->getW(), rt->getH());
                syslog(LOG_INFO, " -> reposition complete!");
            }
            break;
        case KeyUp:
            {
                g_KeyPressed = true;
                key_input_handler(*reinterpret_cast<const uint64_t*>(msgData));
            }
            break;
        case Focus:
            {
                if(g_pCurrentTerm)
                {
                    g_pCurrentTerm->term->setCursorStyle(true);
                    g_pCurrentTerm->term->showCursor(dirty);
                }
            }
            break;
        case NoFocus:
            {
                if(g_pCurrentTerm)
                {
                    g_pCurrentTerm->term->setCursorStyle(false);
                    g_pCurrentTerm->term->showCursor(dirty);
                }
            }
            break;
        case RawKeyDown:
        case RawKeyUp:
            // Ignore.
            break;
        case Terminate:
            syslog(LOG_INFO, "TUI: termination request");
            g_bRunning = false;
            break;
        default:
            syslog(LOG_INFO, "TUI: unhandled callback");
    }

    doRedraw(dirty);
    return true;
}

int main(int argc, char *argv[])
{
#ifdef TARGET_LINUX
    openlog("tui", LOG_PID, LOG_USER);
#endif

    syslog(LOG_INFO, "I am %d", getpid());

    char endpoint[256];
    sprintf(endpoint, "tui.%d", getpid());

    PedigreeGraphics::Rect rt;

    g_pEmu = new PedigreeTerminalEmulator();
    syslog(LOG_INFO, "TUI: constructing widget '%s'...", endpoint);
    if(!g_pEmu->construct(endpoint, "Pedigree xterm Emulator", callback, rt))
    {
        syslog(LOG_ERR, "tui: couldn't construct widget");
        delete g_pEmu;
        return 1;
    }
    syslog(LOG_INFO, "TUI: widget constructed!");

    signal(SIGINT, sigint);

    // Handle initial reposition event.
    Widget::checkForEvents(true);

    tui_do(0);

    delete g_pEmu;

    return 0;
}
