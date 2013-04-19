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
#include <stdio.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>
#include <sched.h>

#include <map>
#include <list>

#include <graphics/Graphics.h>
#include <input/Input.h>
#include <ipc/Ipc.h>

#include <protocol.h>

#include "winman.h"

PedigreeGraphics::Framebuffer *g_pTopLevelFramebuffer = 0;

RootContainer *g_pRootContainer = 0;

Window *g_pFocusWindow = 0;

std::map<uint64_t, Window*> *g_Windows;

void handleMessage(char *messageData)
{
    LibUiProtocol::WindowManagerMessage *pWinMan =
        reinterpret_cast<LibUiProtocol::WindowManagerMessage*>(messageData);

    syslog(LOG_INFO, "winman: incoming message at %x", messageData);
    syslog(LOG_INFO, "winman: incoming message with code %d", pWinMan->messageCode);

    size_t totalSize = sizeof(LibUiProtocol::WindowManagerMessage);

    PedigreeIpc::IpcMessage *pIpcResponse = new PedigreeIpc::IpcMessage();
    bool bSuccess = pIpcResponse->initialise();
    syslog(LOG_INFO, "winman: creating response: %s", bSuccess ? "good" : "bad");

    if(pWinMan->messageCode == LibUiProtocol::Create)
    {
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
        Window *pWindow = new Window(pWinMan->widgetHandle, pEndpoint, pParent, g_pTopLevelFramebuffer);
        if(!g_pFocusWindow)
        {
            g_pFocusWindow = pWindow;
        }

        LibUiProtocol::WindowManagerMessage *pHeader =
            reinterpret_cast<LibUiProtocol::WindowManagerMessage*>(responseData);
        pHeader->messageCode = LibUiProtocol::Create;
        pHeader->widgetHandle = pWinMan->widgetHandle;
        pHeader->messageSize = sizeof(LibUiProtocol::CreateMessageResponse);
        pHeader->isResponse = true;

        LibUiProtocol::CreateMessageResponse *pCreateResp =
            reinterpret_cast<LibUiProtocol::CreateMessageResponse*>(responseData + sizeof(LibUiProtocol::WindowManagerMessage));
        pCreateResp->provider = pWindow->getContext()->getProvider();

        syslog(LOG_INFO, "winman: create message!");
        g_Windows->insert(std::make_pair(pWinMan->widgetHandle, pWindow));
        syslog(LOG_INFO, "winman: metadata entered");

        syslog(LOG_INFO, "winman: transmitting response");
        PedigreeIpc::send(pEndpoint, pIpcResponse, true);

        syslog(LOG_INFO, "winman: create message response transmitted");
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
    if(PedigreeIpc::recv(pEndpoint, &pRecv, true))
    {
        handleMessage(static_cast<char *>(pRecv->getBuffer()));

        delete pRecv;
    }
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
                }
            }
            else if(c == '\n')
            {
                // Add window to active container.
                /// \todo How do we make this work? We need to run a command,
                ///       and wait for it to send a Create request and shove
                ///       it in here??? Hmm.
                // Perhaps create a process, map its PID, use PID to create
                // window in the correct container when it first sends a
                // Create.
                Window *pWindow = new Window(0, 0, focusParent, g_pTopLevelFramebuffer);
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
                syslog(LOG_INFO, "parent: %p focus: %p", focusParent, g_pFocusWindow);
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
                    syslog(LOG_INFO, "found sibling %p", sibling);
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
        }
    }
}

int main(int argc, char *argv[])
{
    // Create the window manager IPC endpoint for libui.
    PedigreeIpc::createEndpoint("pedigree-winman");
    PedigreeIpc::IpcEndpoint *pEndpoint = PedigreeIpc::getEndpoint("pedigree-winman");

    if(!pEndpoint)
    {
        syslog(LOG_CRIT, "error: couldn't create the pedigree-winman IPC endpoint!");
        return 0;
    }

    // Make a program to test window creation etc...
    if(fork() == 0)
    {
        syslog(LOG_INFO, "winman: child alive %d", getpid());
        char *const new_argv[] = {"/applications/gears"};
        execv(new_argv[0], new_argv);
        return 0;
    }

    // Use the root framebuffer.
    PedigreeGraphics::Framebuffer *pRootFramebuffer = new PedigreeGraphics::Framebuffer();
    g_pTopLevelFramebuffer = pRootFramebuffer->createChild(0, 0, pRootFramebuffer->getWidth(), pRootFramebuffer->getHeight());

    if(!g_pTopLevelFramebuffer->getRawBuffer())
    {
        syslog(LOG_CRIT, "error: no top-level framebuffer could be created.");
        return -1;
    }

    size_t g_nWidth = g_pTopLevelFramebuffer->getWidth();
    size_t g_nHeight = g_pTopLevelFramebuffer->getHeight();

    g_pTopLevelFramebuffer->rect(0, 0, g_nWidth, g_nHeight, PedigreeGraphics::createRgb(0, 0, 255), PedigreeGraphics::Bits32_Rgb);
    g_pTopLevelFramebuffer->redraw(0, 0, g_nWidth, g_nHeight, true);

    syslog(LOG_INFO, "winman: creating tile root");

    g_pRootContainer = new RootContainer(g_nWidth, g_nHeight);

    /*
    Container *pMiddle = new Container(g_pRootContainer);
    pMiddle->setLayout(Container::Stacked);

    Container *another = new Container(pMiddle);
    another->setLayout(Container::SideBySide);

    Window *pLeft = new Window(0, 0, g_pRootContainer, g_pTopLevelFramebuffer);
    g_pRootContainer->addChild(pMiddle);
    Window *pRight = new Window(0, 0, g_pRootContainer, g_pTopLevelFramebuffer);

    // Should be evenly split down the screen after retile()
    Window *pTop = new Window(0, 0, pMiddle, g_pTopLevelFramebuffer);
    Window *pMiddleWindow = new Window(0, 0, pMiddle, g_pTopLevelFramebuffer);
    Window *pBottom = new Window(0, 0, pMiddle, g_pTopLevelFramebuffer);

    pMiddle->addChild(another);

    Window *more = new Window(0, 0, another, g_pTopLevelFramebuffer);
    Window *evenmore = new Window(0, 0, another, g_pTopLevelFramebuffer);

    g_pFocusWindow = pLeft;
    pLeft->focus();
    */

    syslog(LOG_INFO, "winman: entering main loop %d", getpid());

    g_Windows = new std::map<uint64_t, Window*>();

    Input::installCallback(Input::Key | Input::Mouse, systemInputCallback);

    // Main loop: logic & message handling goes here!
    while(true)
    {
        // g_pTopLevelFramebuffer->rect(0, 0, g_nWidth, g_nHeight, PedigreeGraphics::createRgb(0, 0, 255), PedigreeGraphics::Bits32_Rgb);
        checkForMessages(pEndpoint);
        g_pRootContainer->render();
        g_pTopLevelFramebuffer->redraw();

        sched_yield();
    }

    return 0;
}
