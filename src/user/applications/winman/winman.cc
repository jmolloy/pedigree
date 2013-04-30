/*
 * Copyright (c) 2010 James Molloy, Matthew Iselin
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
#define _USE_MATH_DEFINES
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <syslog.h>
#include <string.h>
#include <sched.h>
#include <math.h>
#include <errno.h>

#include <map>
#include <list>

#include <graphics/Graphics.h>
#include <input/Input.h>
#include <ipc/Ipc.h>

#include <cairo/cairo.h>
#include <cairo/cairo-ft.h>

#include <iostream>

#include <protocol.h>

#include "winman.h"
#include "Png.h"

PedigreeGraphics::Framebuffer *g_pTopLevelFramebuffer = 0;

RootContainer *g_pRootContainer = 0;

Window *g_pFocusWindow = 0;

uint8_t *g_pBackbuffer = 0;

std::map<uint64_t, Window*> *g_Windows;

/// \todo Make configurable.
//#define CLIENT_DEFAULT "/applications/tui"
#define CLIENT_DEFAULT "/applications/gears"

void startClient()
{
    if(fork() == 0)
    {
        execl(CLIENT_DEFAULT, CLIENT_DEFAULT, 0);
        exit(1);
    }
}

bool handleMessage(char *messageData, cairo_t *cr)
{
    bool bResult = false;
    LibUiProtocol::WindowManagerMessage *pWinMan =
        reinterpret_cast<LibUiProtocol::WindowManagerMessage*>(messageData);

    size_t totalSize = sizeof(LibUiProtocol::WindowManagerMessage);

    PedigreeIpc::IpcMessage *pIpcResponse = new PedigreeIpc::IpcMessage();
    bool bSuccess = pIpcResponse->initialise();

    if(pWinMan->messageCode == LibUiProtocol::Create)
    {
        syslog(LOG_INFO, "\n**** CREATE ****\n");
        LibUiProtocol::CreateMessage *pCreate =
        reinterpret_cast<LibUiProtocol::CreateMessage*>(messageData + sizeof(LibUiProtocol::WindowManagerMessage));
        totalSize += sizeof(LibUiProtocol::CreateMessageResponse);
        char *responseData = (char *) pIpcResponse->getBuffer();

        PedigreeIpc::IpcEndpoint *pEndpoint = PedigreeIpc::getEndpoint(pCreate->endpoint);

        Container *pParent = g_pRootContainer;
        if(g_pFocusWindow)
        {
            pParent = g_pFocusWindow->getParent();
        }

        Window *pWindow = new Window(pWinMan->widgetHandle, pEndpoint, pParent);
        std::string newTitle(pCreate->title);
        pWindow->setTitle(newTitle);
        pWindow->render(cr);
        bResult = true;

        if(!g_pFocusWindow)
        {
            g_pFocusWindow = pWindow;
            pWindow->focus();
        }

        LibUiProtocol::WindowManagerMessage *pHeader =
            reinterpret_cast<LibUiProtocol::WindowManagerMessage*>(responseData);
        pHeader->messageCode = LibUiProtocol::Create;
        pHeader->widgetHandle = pWinMan->widgetHandle;
        pHeader->messageSize = sizeof(LibUiProtocol::CreateMessageResponse);
        pHeader->isResponse = true;

        LibUiProtocol::CreateMessageResponse *pCreateResp =
            reinterpret_cast<LibUiProtocol::CreateMessageResponse*>(responseData + sizeof(LibUiProtocol::WindowManagerMessage));
        // pCreateResp->provider = pWindow->getContext()->getProvider();

        g_Windows->insert(std::make_pair(pWinMan->widgetHandle, pWindow));

        PedigreeIpc::send(pEndpoint, pIpcResponse, true);
    }
    else if(pWinMan->messageCode == LibUiProtocol::Sync)
    {
        std::map<uint64_t, Window*>::iterator it = g_Windows->find(pWinMan->widgetHandle);
        if(it != g_Windows->end())
        {
            Window *pWindow = it->second;
            char *responseData = (char *) pIpcResponse->getBuffer();

            LibUiProtocol::WindowManagerMessage *pHeader =
                reinterpret_cast<LibUiProtocol::WindowManagerMessage*>(responseData);
            pHeader->messageCode = LibUiProtocol::Sync;
            pHeader->widgetHandle = pWinMan->widgetHandle;
            pHeader->messageSize = sizeof(LibUiProtocol::SyncMessageResponse);
            pHeader->isResponse = true;

            LibUiProtocol::SyncMessageResponse *pSyncResp =
                reinterpret_cast<LibUiProtocol::SyncMessageResponse*>(responseData + sizeof(LibUiProtocol::WindowManagerMessage));
            // pSyncResp->provider = pWindow->getContext()->getProvider();

            PedigreeIpc::send(pWindow->getEndpoint(), pIpcResponse, true);
        }
    }
    else if(pWinMan->messageCode == LibUiProtocol::RequestRedraw)
    {
        /// \todo don't redraw entire window - we get provided a region!
        std::map<uint64_t, Window*>::iterator it = g_Windows->find(pWinMan->widgetHandle);
        if(it != g_Windows->end())
        {
            Window *pWindow = it->second;
            pWindow->render(cr);
            bResult = true;
        }
    }
    else
    {
        syslog(LOG_INFO, "winman: unhandled message type");
    }

    return bResult;
}

bool checkForMessages(PedigreeIpc::IpcEndpoint *pEndpoint, cairo_t *cr)
{
    bool bResult = false;
    if(!pEndpoint)
        return bResult;

    PedigreeIpc::IpcMessage *pRecv = 0;
    if(PedigreeIpc::recv(pEndpoint, &pRecv, true))
    {
        bResult = handleMessage(static_cast<char *>(pRecv->getBuffer()), cr);

        delete pRecv;
    }

    return bResult;
}

#define ALT_KEY (1ULL << 60)
#define SHIFT_KEY (1ULL << 61)

// I am almost completely convinced this is not right.
// Will keymaps change these values???
#define KEY_LEFT    108
#define KEY_UP      117
#define KEY_DOWN    100
#define KEY_RIGHT   114

/// System input callback.
void systemInputCallback(Input::InputNotification &note)
{
    syslog(LOG_INFO, "winman: system input (type=%d)", note.type);
    bool bHandled = false;
    if(note.type & Input::Key)
    {
        uint64_t c = note.data.key.key;

        /// \todo make configurable
        if((c & ALT_KEY) && (g_pFocusWindow != 0))
        {
            bool bShift = (c & SHIFT_KEY);

            c &= 0xFF;
            syslog(LOG_INFO, "ALT-%d %c", (uint32_t) c, (char) c);

            Container *focusParent = g_pFocusWindow->getParent();
            Window *newFocus = 0;
            WObject *sibling = 0;
            if(bShift)
            {
                if(c == 'r')
                {
                    g_pRootContainer->retile();
                    bHandled = true;
                }
                else if(c == 'q')
                {
                    /// \todo this code is completely broken.

                    if(focusParent->getChildCount() > 1)
                    {
                        newFocus = static_cast<Window*>(focusParent->getLeftSibling(g_pFocusWindow));
                        if(!newFocus)
                        {
                            newFocus = static_cast<Window*>(focusParent->getRightSibling(g_pFocusWindow));
                        }
                    }

                    focusParent->removeChild(g_pFocusWindow);

                    if(focusParent->getChildCount() == 0)
                    {
                        // No more children - remove container.
                        /// \todo write me.
                    }

                    g_pFocusWindow = newFocus;

                    bHandled = true;
                }
            }
            else if(c == '\n')
            {
                // Add window to active container.
                startClient();
                bHandled = true;
            }
            else if((c == 'v') || (c == 'h'))
            {
                ::Container::Layout layout;
                if(c == 'v')
                {
                    layout = Container::Stacked;
                }
                else if(c == 'h')
                {
                    layout = Container::SideBySide;
                }
                else
                {
                    layout = Container::SideBySide;
                }

                // Replace focus window with container (Container::replace) in
                // the relevant layout mode.
                if(focusParent->getChildCount() == 1)
                {
                    focusParent->setLayout(layout);
                }
                else if(focusParent->getLayout() != layout)
                {
                    Container *pNewContainer = new Container(focusParent);
                    pNewContainer->addChild(g_pFocusWindow, true);
                    g_pFocusWindow->setParent(pNewContainer);

                    focusParent->replaceChild(g_pFocusWindow, pNewContainer);
                    focusParent->retile();
                    pNewContainer->setLayout(layout);
                }

                bHandled = true;
            }
            /*
            else if(c == 'r')
            {
                // State machine: enter 'resize mode', which allows us to resize
                // the container in which the window with focus is in.
            }
            */
            else if((c == KEY_LEFT) || (c == KEY_RIGHT) || (c == KEY_UP) || (c == KEY_DOWN))
            {
                if(c == KEY_LEFT)
                {
                    sibling = focusParent->getLeft(g_pFocusWindow);
                }
                else if(c == KEY_UP)
                {
                    sibling = focusParent->getUp(g_pFocusWindow);
                }
                else if(c == KEY_DOWN)
                {
                    sibling = focusParent->getDown(g_pFocusWindow);
                }
                else if(c == KEY_RIGHT)
                {
                    sibling = focusParent->getRight(g_pFocusWindow);
                }

                // Not edge of screen?
                if(sibling)
                {
                    while(sibling->getType() == WObject::Container)
                    {
                        // Need to make the focus the first non-Container child of the container.
                        /// \todo Need to have containers remember the last window that was focussed
                        ///       inside it, so we can a) render it differently, and b) not assume
                        ///       getChild(0) is the right thing to do.
                        Container *pContainer = static_cast<Container*>(sibling);
                        sibling = pContainer->getFocusWindow();
                    }

                    if(sibling->getType() == WObject::Window)
                    {
                        newFocus = static_cast<Window*>(sibling);
                    }
                }
            }

            if(newFocus)
            {
                g_pFocusWindow->nofocus();
                g_pFocusWindow = newFocus;
                g_pFocusWindow->focus();
            }

            bHandled = true;
        }
    }

    if((!bHandled) && (g_pFocusWindow))
    {
        // Forward event to focus window.
        size_t totalSize = sizeof(LibUiProtocol::WindowManagerMessage);
        if(note.type & Input::Key)
            totalSize += sizeof(LibUiProtocol::KeyEventMessage);

        PedigreeIpc::IpcMessage *pMessage = new PedigreeIpc::IpcMessage();
        pMessage->initialise();
        char *buffer = (char *) pMessage->getBuffer();

        LibUiProtocol::WindowManagerMessage *pHeader =
            reinterpret_cast<LibUiProtocol::WindowManagerMessage*>(buffer);
        pHeader->messageCode = LibUiProtocol::KeyEvent;
        pHeader->widgetHandle = g_pFocusWindow->getHandle();
        pHeader->messageSize = totalSize - sizeof(*pHeader);
        pHeader->isResponse = false;

        if(note.type & Input::Key)
        {
            LibUiProtocol::KeyEventMessage *pKeyEvent =
                reinterpret_cast<LibUiProtocol::KeyEventMessage*>(buffer + sizeof(LibUiProtocol::WindowManagerMessage));
            pKeyEvent->state = LibUiProtocol::Up; /// \todo 'keydown' messages.
            pKeyEvent->key = note.data.key.key;

            PedigreeIpc::send(g_pFocusWindow->getEndpoint(), pMessage, true);
        }
    }
}

int main(int argc, char *argv[])
{
    syslog(LOG_INFO, "winman: starting up...");
    std::cout << "hello c++" << std::endl;
    syslog(LOG_INFO, "winman: i/o works");

    // Create the window manager IPC endpoint for libui.
    PedigreeIpc::createEndpoint("pedigree-winman");
    PedigreeIpc::IpcEndpoint *pEndpoint = PedigreeIpc::getEndpoint("pedigree-winman");

    if(!pEndpoint)
    {
        syslog(LOG_CRIT, "error: couldn't create the pedigree-winman IPC endpoint!");
        return 0;
    }

    FT_Library font_library;
    FT_Face ft_face;
    int e = FT_Init_FreeType(&font_library);
    if(e)
    {
        syslog(LOG_CRIT, "error: couldn't initialise Freetype");
        return 0;
    }

    e = FT_New_Face(font_library, "/system/fonts/DejaVuSansMono.ttf", 0, &ft_face);

    // Use the root framebuffer.
    PedigreeGraphics::Framebuffer *pRootFramebuffer = new PedigreeGraphics::Framebuffer();
    g_pTopLevelFramebuffer = pRootFramebuffer; // new PedigreeGraphics::Framebuffer(prov); // pRootFramebuffer->createChild(0, 0, pRootFramebuffer->getWidth(), pRootFramebuffer->getHeight());

    if(!g_pTopLevelFramebuffer->getRawBuffer())
    {
        syslog(LOG_CRIT, "error: no top-level framebuffer could be created.");
        return -1;
    }

    size_t g_nWidth = g_pTopLevelFramebuffer->getWidth();
    size_t g_nHeight = g_pTopLevelFramebuffer->getHeight();

    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, g_nWidth, g_nHeight);
    cairo_t *cr = cairo_create(surface);

    g_pBackbuffer = cairo_image_surface_get_data(surface);

    /// \todo Check for cairo error here...

    cairo_user_data_key_t key;
    cairo_font_face_t *font_face;

    font_face = cairo_ft_font_face_create_for_ft_face(ft_face, 0);
    cairo_font_face_set_user_data(font_face, &key,
                                  ft_face, (cairo_destroy_func_t) FT_Done_Face);
    cairo_set_font_face(cr, font_face);

#if 1
    /*
    cairo_surface_t *wallpaper = cairo_image_surface_create_from_png("/system/wallpaper/fields.png");
    syslog(LOG_INFO, "status: %s / %s", cairo_status_to_string(cairo_status(cr)), cairo_status_to_string(cairo_surface_status(wallpaper)));

    int wallpaperWidth = cairo_image_surface_get_width(wallpaper);
    int wallpaperHeight = cairo_image_surface_get_height(wallpaper);

    syslog(LOG_INFO, "w: %d, h: %d", wallpaperWidth, wallpaperHeight);

    cairo_scale(cr, g_nWidth / (double) wallpaperWidth, g_nHeight / (double) wallpaperHeight);

    cairo_set_source_surface(cr, wallpaper, 0, 0);
    cairo_paint(cr);
    cairo_surface_destroy(wallpaper);
    */

    Png *png = new Png("/system/wallpaper/fields.png");
    png->render(cr, 0, 0, g_nWidth, g_nHeight);
#else
    cairo_set_source_rgba(cr, 0, 0, 1.0, 1.0);
    cairo_rectangle(cr, 0, 0, g_nWidth, g_nHeight);
    cairo_stroke_preserve(cr);
    cairo_fill(cr);
#endif

    // g_pTopLevelFramebuffer->rect(0, 0, g_nWidth, g_nHeight, PedigreeGraphics::createRgb(0, 0, 255), PedigreeGraphics::Bits32_Rgb);

    syslog(LOG_INFO, "winman: framebuffer is at %p", g_pTopLevelFramebuffer->getRawBuffer());

    syslog(LOG_INFO, "winman: creating tile root");

    g_pRootContainer = new RootContainer(g_nWidth, g_nHeight);

#if 0
    Container *pMiddle = new Container(g_pRootContainer);
    pMiddle->setLayout(Container::Stacked);

    Container *another = new Container(pMiddle);
    another->setLayout(Container::SideBySide);

    Window *pLeft = new Window(0, 0, g_pRootContainer);
    g_pRootContainer->addChild(pMiddle);
    Window *pRight = new Window(0, 0, g_pRootContainer);

    // Should be evenly split down the screen after retile()
    Window *pTop = new Window(0, 0, pMiddle);
    Window *pMiddleWindow = new Window(0, 0, pMiddle);
    Window *pBottom = new Window(0, 0, pMiddle);

    pMiddle->addChild(another);

    Window *more = new Window(0, 0, another);
    Window *evenmore = new Window(0, 0, another);

    g_pFocusWindow = pLeft;
    pLeft->focus();
#endif

    syslog(LOG_INFO, "winman: entering main loop %d", getpid());

    g_Windows = new std::map<uint64_t, Window*>();

    Input::installCallback(Input::Key | Input::Mouse, systemInputCallback);

    // Render all window decorations and non-client display elements first up.
    g_pRootContainer->render(cr);

    // Load first tile.
    startClient();

    // Main loop: logic & message handling goes here!
    while(true)
    {
        // Check for any messages coming in from windows, asynchronously.
        // checkForMessages returns true if a handled message requires a redraw.
        if(checkForMessages(pEndpoint, cr))
        {
            // Flush the cairo surface, which will ensure the most recent data is
            // in the framebuffer ready to send to the device.
            cairo_surface_flush(surface);

            // Draw the updated backbuffer.
            g_pTopLevelFramebuffer->draw(g_pBackbuffer,
                                         0,
                                         0,
                                         0,
                                         0,
                                         g_nWidth,
                                         g_nHeight,
                                         PedigreeGraphics::Bits32_Rgb);

            // Submit a full redraw to the graphics card.
            /// \todo We need to stop doing this - we should figue out how much we changed.
            g_pTopLevelFramebuffer->redraw();
        }
        else
        {
            // Yield control to the rest of the system. As we do everything we
            // possibly can asynchronously, if we don't yield we'll pound the
            // system and not let any other processes work.
            sched_yield();
        }
    }

    return 0;
}
