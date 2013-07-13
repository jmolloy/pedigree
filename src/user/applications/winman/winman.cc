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
#include <sys/mman.h>

#include <map>
#include <list>
#include <set>

#include <graphics/Graphics.h>
#include <input/Input.h>
#include <ipc/Ipc.h>

#include <cairo/cairo.h>
#include <cairo/cairo-ft.h>

#include <protocol.h>

#include "winman.h"
#include "Png.h"

PedigreeGraphics::Framebuffer *g_pTopLevelFramebuffer = 0;

RootContainer *g_pRootContainer = 0;

Window *g_pFocusWindow = 0;

uint8_t *g_pBackbuffer = 0;

std::map<uint64_t, Window*> *g_Windows;

std::set<WObject*> g_PendingWindows;

PedigreeIpc::IpcEndpoint *g_pEndpoint = 0;

std::string g_StatusField = "";

size_t g_nWidth = 0;
size_t g_nHeight = 0;

/// \todo Make configurable.
#define CLIENT_DEFAULT "/applications/tui"
// #define CLIENT_DEFAULT "/applications/gears"

void startClient()
{
    if(fork() == 0)
    {
        execl(CLIENT_DEFAULT, CLIENT_DEFAULT, 0);
        exit(1);
    }
}

void fps()
{
    static unsigned int frames = 0;
    static unsigned int start_time = 0;

    struct timeval now;
    gettimeofday(&now, NULL);
    frames++;
    if (!start_time)
    {
        start_time = now.tv_sec;
    }
    else if (now.tv_sec - start_time >= 5)
    {
        float seconds = now.tv_sec - start_time;
        float fps = frames / seconds;
        syslog(LOG_INFO, "%d frames in %3.1f seconds = %6.3f FPS", frames, seconds, fps);
        start_time = now.tv_sec;
        frames = 0;
    }
}

void handleMessage(char *messageData)
{
    bool bResult = false;
    LibUiProtocol::WindowManagerMessage *pWinMan =
        reinterpret_cast<LibUiProtocol::WindowManagerMessage*>(messageData);

    size_t totalSize = sizeof(LibUiProtocol::WindowManagerMessage);

    if(pWinMan->messageCode == LibUiProtocol::Create)
    {
        PedigreeIpc::IpcMessage *pIpcResponse = new PedigreeIpc::IpcMessage();
        bool bSuccess = pIpcResponse->initialise();

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
        g_PendingWindows.insert(pWindow);

        if(g_pFocusWindow)
        {
            g_pFocusWindow->nofocus();
        }
        g_pFocusWindow = pWindow;
        pWindow->focus();

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
            PedigreeIpc::IpcMessage *pIpcResponse = new PedigreeIpc::IpcMessage();
            bool bSuccess = pIpcResponse->initialise();

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
        LibUiProtocol::RequestRedrawMessage *pRedrawMsg =
            reinterpret_cast<LibUiProtocol::RequestRedrawMessage*>(messageData + sizeof(LibUiProtocol::WindowManagerMessage));
        PedigreeGraphics::Rect dirty(pRedrawMsg->x, pRedrawMsg->y, pRedrawMsg->width, pRedrawMsg->height);

        std::map<uint64_t, Window*>::iterator it = g_Windows->find(pWinMan->widgetHandle);
        if(it != g_Windows->end())
        {
            Window *pWindow = it->second;
            pWindow->setDirty(dirty);
            g_PendingWindows.insert(pWindow);
        }
    }
    else if(pWinMan->messageCode == LibUiProtocol::Nothing)
    {
        // We inject this to wake up checkForMessages and move on to rendering.
    }
    else
    {
        syslog(LOG_INFO, "winman: unhandled message type");
    }
}

void checkForMessages(PedigreeIpc::IpcEndpoint *pEndpoint)
{
    if(!pEndpoint)
        return;

    PedigreeIpc::IpcMessage *pRecv = 0;
    if(PedigreeIpc::recv(pEndpoint, &pRecv, false))
    {
        handleMessage(static_cast<char *>(pRecv->getBuffer()));

        delete pRecv;
    }
}

#define ALT_KEY (1ULL << 60)
#define SHIFT_KEY (1ULL << 61)
#define SPECIAL_KEY (1ULL << 63)

enum ActualKey
{
    None,
    Left,
    Right,
    Up,
    Down,
};

/// System input callback.
void systemInputCallback(Input::InputNotification &note)
{
    syslog(LOG_INFO, "winman: system input (type=%d)", note.type);
    static bool bResize = false;

    bool bHandled = false;
    bool bWakeup = false;
    if(note.type & Input::Key)
    {
        uint64_t c = note.data.key.key;

        ActualKey realKey = None;

        if(c & SPECIAL_KEY)
        {
            uint32_t k = c & 0xFFFFFFFFULL;
            char str[5];
            memcpy(str, reinterpret_cast<char*>(&k), 4);
            str[4] = 0;

            if(!strcmp(str, "left"))
            {
                realKey = Left;
            }
            else if(!strcmp(str, "righ"))
            {
                realKey = Right;
            }
            else if(!strcmp(str, "up"))
            {
                realKey = Up;
            }
            else if(!strcmp(str, "down"))
            {
                realKey = Down;
            }
        }

        /// \todo make configurable
        if((c & ALT_KEY) && (g_pFocusWindow != 0))
        {
            bool bShift = (c & SHIFT_KEY);

            syslog(LOG_INFO, "ALT-%d [%x%x] %c", (uint32_t) c, (uint32_t) (c >> 32ULL), (uint32_t) c, (char) c);

            c &= 0xFFFFFFFFULL;
            Container *focusParent = g_pFocusWindow->getParent();
            Window *newFocus = 0;
            WObject *sibling = 0;
            if(bShift)
            {
                if(c == 'R')
                {
                    g_pRootContainer->retile();
                    bHandled = true;
                }
                else if(c == 'Q')
                {
                    // Kill the client.
                    uint64_t handle = g_pFocusWindow->getHandle();
                    pid_t pid = (pid_t) ((handle >> 32) & 0xFFFFFFFF);
                    kill(pid, SIGTERM);

                    // Find any siblings if we can.
                    WObject *siblingObject = focusParent->getLeftSibling(g_pFocusWindow);
                    if(!siblingObject)
                    {
                        siblingObject = focusParent->getRightSibling(g_pFocusWindow);
                    }

                    // Will there be any children left?
                    if(focusParent->getChildCount() > 1)
                    {
                        // Yes. Focus adjustment is mostly trivial.
                    }
                    else
                    {
                        // No. Focus adjustment is less trivial.
                        siblingObject = focusParent->getLeft(g_pFocusWindow);
                        if(!siblingObject)
                        {
                            siblingObject = focusParent->getRight(g_pFocusWindow);
                        }
                        if(!siblingObject)
                        {
                            siblingObject = focusParent->getUp(g_pFocusWindow);
                        }
                        if(!siblingObject)
                        {
                            siblingObject = focusParent->getDown(g_pFocusWindow);
                        }
                    }

                    // Remove from the parent container.
                    focusParent->removeChild(g_pFocusWindow);
                    if(focusParent->getChildCount() > 0)
                    {
                        focusParent->retile();
                    }
                    else if(focusParent->getParent())
                    {
                        Container *pContainer = static_cast<Container*>(focusParent->getParent());
                        pContainer->removeChild(focusParent);
                        pContainer->retile();
                        focusParent = pContainer;
                    }

                    // Assign new focus.
                    if(siblingObject)
                    {
                        while(siblingObject->getType() == WObject::Container)
                        {
                            Container *pContainer = static_cast<Container*>(siblingObject);
                            siblingObject = pContainer->getFocusWindow();
                        }

                        if(siblingObject->getType() == WObject::Window)
                        {
                            newFocus = static_cast<Window*>(siblingObject);
                        }
                    }

                    // All children are now pending a redraw.
                    g_PendingWindows.insert(focusParent);

                    // Clean up.
                    // delete g_pFocusWindow;
                    g_pFocusWindow = 0;

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
            else if(c == 'r')
            {
                // State machine: enter 'resize mode', which allows us to resize
                // the container in which the window with focus is in.
                bResize = true;
                bHandled = true;
                bWakeup = true;
                g_StatusField = "<resize mode>";

                // Don't transmit resizes to the client(s) yet.
                g_pRootContainer->norefresh();
            }
            else if(realKey != None)
            {
                if(realKey == Left)
                {
                    sibling = focusParent->getLeft(g_pFocusWindow);
                }
                else if(realKey == Up)
                {
                    sibling = focusParent->getUp(g_pFocusWindow);
                }
                else if(realKey == Down)
                {
                    sibling = focusParent->getDown(g_pFocusWindow);
                }
                else if(realKey == Right)
                {
                    sibling = focusParent->getRight(g_pFocusWindow);
                }

                // Not edge of screen?
                if(sibling)
                {
                    while(sibling->getType() == WObject::Container)
                    {
                        Container *pContainer = static_cast<Container*>(sibling);
                        sibling = pContainer->getFocusWindow();
                    }

                    if(sibling->getType() == WObject::Window)
                    {
                        newFocus = static_cast<Window*>(sibling);
                    }
                }

                bHandled = true;
            }

            if(newFocus)
            {
                if(g_pFocusWindow)
                {
                    g_PendingWindows.insert(g_pFocusWindow);
                    g_pFocusWindow->nofocus();
                }

                g_PendingWindows.insert(newFocus);

                g_pFocusWindow = newFocus;
                g_pFocusWindow->focus();
                bWakeup = true;
            }
        }

        if((!bHandled) && bResize && (g_pFocusWindow))
        {
            bWakeup = true;
            bHandled = true;

            Container *focusParent = g_pFocusWindow->getParent();
            WObject *sibling = 0;
            if(c == '\e')
            {
                g_StatusField = " ";
                bResize = false;

                // Okay, now the client(s) can get a refreshed context.
                g_pRootContainer->yesrefresh();
            }
            else if(realKey == Left)
            {
                sibling = focusParent->getLeft(g_pFocusWindow);
                if(sibling)
                {
                    sibling->resize(-10, 0);
                }
            }
            else if(realKey == Right)
            {
                sibling = focusParent->getRight(g_pFocusWindow);
                if(sibling)
                {
                    g_pFocusWindow->resize(10, 0);
                }
            }
            else if(realKey == Up)
            {
                sibling = focusParent->getUp(g_pFocusWindow);
                if(sibling)
                {
                    sibling->resize(0, -10);
                }
            }
            else if(realKey == Down)
            {
                sibling = focusParent->getDown(g_pFocusWindow);
                if(sibling)
                {
                    g_pFocusWindow->resize(0, 10);
                }
            }
            else
            {
                bWakeup = false;
            }

            if(bWakeup)
            {
                g_PendingWindows.insert(sibling);
                g_PendingWindows.insert(g_pFocusWindow);
            }
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
        else
        {
            delete pMessage;
        }
    }

    // If we need to wake up the main IPC thread, inject a nothing message.
    if(bWakeup)
    {
        PedigreeIpc::IpcMessage *pMessage = new PedigreeIpc::IpcMessage();
        pMessage->initialise();
        char *buffer = (char *) pMessage->getBuffer();

        LibUiProtocol::WindowManagerMessage *pHeader =
            reinterpret_cast<LibUiProtocol::WindowManagerMessage*>(buffer);
        pHeader->messageCode = LibUiProtocol::Nothing;

        PedigreeIpc::send(g_pEndpoint, pMessage, true);
    }
}

void infoPanel(cairo_t *cr)
{
    // Create a nice little bar at the bottom of the screen.
    cairo_save(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);

    cairo_set_source_rgba(cr, 0.2, 0.2, 0.2, 0.8);
    cairo_rectangle(cr, 0, g_nHeight - 24, g_nWidth, 24);
    cairo_fill(cr);

    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

    cairo_set_font_size(cr, 13);
    cairo_font_extents_t extents;
    cairo_font_extents(cr, &extents);

    cairo_move_to(cr, 3, (g_nHeight - 24) + 3 + extents.height);
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0);
    cairo_show_text(cr, "The Pedigree Operating System");

    if(g_StatusField.length())
    {
        cairo_move_to(
                cr,
                g_nWidth - 3 - (extents.max_x_advance * g_StatusField.length()),
                (g_nHeight - 24) + 3 + extents.height);
        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0);
        cairo_show_text(cr, g_StatusField.c_str());
        g_StatusField.clear();
    }

    cairo_restore(cr);
}

int main(int argc, char *argv[])
{
    syslog(LOG_INFO, "winman: starting up...");

    // Create the window manager IPC endpoint for libui.
    PedigreeIpc::createEndpoint("pedigree-winman");
    g_pEndpoint = PedigreeIpc::getEndpoint("pedigree-winman");

    if(!g_pEndpoint)
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
    g_pTopLevelFramebuffer = pRootFramebuffer;

    if(!g_pTopLevelFramebuffer->getRawBuffer())
    {
        syslog(LOG_CRIT, "error: no top-level framebuffer could be created.");
        return -1;
    }

    g_nWidth = g_pTopLevelFramebuffer->getWidth();
    g_nHeight = g_pTopLevelFramebuffer->getHeight();

    cairo_format_t format = CAIRO_FORMAT_ARGB32;
    if(g_pTopLevelFramebuffer->getFormat() == PedigreeGraphics::Bits24_Rgb)
    {
        if(g_pTopLevelFramebuffer->getBytesPerPixel() != 4)
        {
            syslog(LOG_CRIT, "winman: error: incompatible framebuffer format (bytes per pixel)");
            return -1;
        }
    }
    else if(g_pTopLevelFramebuffer->getFormat() == PedigreeGraphics::Bits16_Rgb565)
    {
        format = CAIRO_FORMAT_RGB16_565;
    }
    else if(g_pTopLevelFramebuffer->getFormat() > PedigreeGraphics::Bits32_Rgb)
    {
        syslog(LOG_CRIT, "winman: error: incompatible framebuffer format (possibly BGR or similar)");
        return -1;
    }

    int stride = cairo_format_stride_for_width(format, g_nWidth);

    // Map the framebuffer in to our address space.
    uintptr_t rawBuffer = (uintptr_t) g_pTopLevelFramebuffer->getRawBuffer();
    void *framebufferVirt = mmap(
        0,
        stride * g_nHeight,
        PROT_READ | PROT_WRITE,
        MAP_ANON | MAP_PHYS_OFFSET,
        -1,
        rawBuffer);

    if(framebufferVirt == MAP_FAILED)
    {
        syslog(LOG_CRIT, "winman: couldn't map framebuffer into address space");
        return -1;
    }
    else
    {
        syslog(LOG_INFO, "winman: mapped framebuffer at %p", framebufferVirt);
    }

    cairo_surface_t *surface = cairo_image_surface_create_for_data(
            (uint8_t *) framebufferVirt,
            format,
            g_nWidth,
            g_nHeight,
            stride);
    cairo_t *cr = cairo_create(surface);

    cairo_user_data_key_t key;
    cairo_font_face_t *font_face;

    font_face = cairo_ft_font_face_create_for_ft_face(ft_face, 0);
    cairo_font_face_set_user_data(font_face, &key,
                                  ft_face, (cairo_destroy_func_t) FT_Done_Face);
    cairo_set_font_face(cr, font_face);

    Png *wallpaper = 0;
    FILE *fp = fopen("/system/wallpaper/fields.png", "rb");
    if(fp)
    {
        fclose(fp);

        // A nice wallpaper.
        wallpaper = new Png("/system/wallpaper/fields.png");
        wallpaper->render(cr, 0, 0, g_nWidth, g_nHeight);
    }
    else
    {
        // No PNG found, fall back to a blue background.
        cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
        cairo_set_source_rgba(cr, 0, 0, 1.0, 1.0);
        cairo_rectangle(cr, 0, 0, g_nWidth, g_nHeight);
        cairo_fill(cr);
    }

    syslog(LOG_INFO, "winman: creating tile root");

    g_pRootContainer = new RootContainer(g_nWidth, g_nHeight - 24);

    infoPanel(cr);

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

    syslog(LOG_INFO, "winman: entering main loop pid=%d", getpid());

    g_Windows = new std::map<uint64_t, Window*>();

    // Install our global input callback before we kick off our client.
    Input::installCallback(Input::Key | Input::Mouse, systemInputCallback);

    // Render all window decorations and non-client display elements first up.
    g_pRootContainer->render(cr);

    // Kick off the first render before any windows are open.
    cairo_surface_flush(surface);
    g_pTopLevelFramebuffer->redraw();

    // Load first tile.
    startClient();

    // Main loop: logic & message handling goes here!
    DirtyRectangle renderDirty;
    while(true)
    {
        // Check for any messages coming in from windows, asynchronously.
        checkForMessages(g_pEndpoint);

        // Check for any windows that may need rendering.
        if((!g_PendingWindows.empty()) || g_StatusField.length())
        {
            // Render each window, also ensuring the wallpaper is rendered again
            // so that windows with alpha look correct.
            std::set<WObject*>::iterator it = g_PendingWindows.begin();
            size_t nDirty = g_StatusField.length() ? 1 : 0;
            for(; it != g_PendingWindows.end(); ++it)
            {
                Window *pWindow = 0;
                if((*it)->getType() == WObject::Window)
                {
                    pWindow = static_cast<Window*>(*it);
                }

                if(pWindow && !pWindow->isDirty())
                {
                    continue;
                }
                else
                {
                    ++nDirty;
                }

                PedigreeGraphics::Rect rt = (*it)->getCopyDimensions();
                PedigreeGraphics::Rect dirty = rt;
                if(pWindow)
                {
                    dirty = pWindow->getDirty();
                }

                // Render wallpaper.
                if(wallpaper)
                {
                    wallpaper->renderPartial(
                            cr,
                            rt.getX() + dirty.getX(),
                            rt.getY() + dirty.getY(),
                            0, 0,
                            dirty.getW(), dirty.getH(),
                            g_nWidth, g_nHeight);
                }
                else
                {
                    // Boring background.
                    cairo_set_source_rgba(cr, 0, 0, 1.0, 1.0);
                    cairo_rectangle(
                            cr,
                            rt.getX() + dirty.getX(),
                            rt.getY() + dirty.getY(),
                            dirty.getW(),
                            dirty.getH());
                    cairo_stroke_preserve(cr);
                    cairo_fill(cr);
                }

                // Render window.
                (*it)->render(cr);

                // Update the dirty rectangle.
                renderDirty.point(
                        rt.getX() + dirty.getX(),
                        rt.getX() + dirty.getY());
                renderDirty.point(
                        rt.getX() + dirty.getX() + dirty.getW(),
                        rt.getX() + dirty.getY() + dirty.getH());
            }

            // Empty out the list in full.
            g_PendingWindows.clear();

            // Only do rendering if we actually did some rendering!
            if(!nDirty)
            {
                renderDirty.reset();
                continue;
            }

            if(g_StatusField.length())
            {
                infoPanel(cr);
            }

            // Flush the cairo surface, which will ensure the most recent data is
            // in the framebuffer ready to send to the device.
            cairo_surface_flush(surface);

            // Submit a redraw to the graphics card.
            g_pTopLevelFramebuffer->redraw(renderDirty.getX(), renderDirty.getY(), renderDirty.getWidth(), renderDirty.getHeight());

            // Wipe out the dirty rectangle - we're all done.
            renderDirty.reset();
        }
        else
        {
            // Yield control to the rest of the system. As we do everything we
            // possibly can asynchronously, if we don't yield we'll pound the
            // system and not let any other processes work.
            // sched_yield();
        }
    }

    return 0;
}
