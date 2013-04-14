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

        /// \todo Stop giving windows the top-level framebuffer.
        Container *pParent = g_pRootContainer;
        if(g_pFocusWindow)
            pParent = g_pFocusWindow->getParent();
        Window *pWindow = new Window(pWinMan->widgetHandle, pEndpoint, pParent, g_pTopLevelFramebuffer);

        LibUiProtocol::WindowManagerMessage *pHeader =
            reinterpret_cast<LibUiProtocol::WindowManagerMessage*>(responseData);
        pHeader->messageCode = LibUiProtocol::Create;
        pHeader->widgetHandle = pWinMan->widgetHandle;
        pHeader->messageSize = sizeof(LibUiProtocol::CreateMessageResponse);
        pHeader->isResponse = true;

        LibUiProtocol::CreateMessageResponse *pCreateResp =
            reinterpret_cast<LibUiProtocol::CreateMessageResponse*>(responseData + sizeof(LibUiProtocol::WindowManagerMessage));
        pCreateResp->provider = g_pTopLevelFramebuffer->getProvider(); // bad

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

    // delete pIpcResponse;
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
            syslog(LOG_INFO, "ALT-%d %c", c, c);

            Container *focusParent = g_pFocusWindow->getParent();
            Window *newFocus = 0;
            WObject *sibling = 0;
            if(bShift)
            {
                if(c == 'r')
                {
                    g_pRootContainer->retile();
                }
            }
            else if(c == '\n')
            {
                // Window *pWindow = new Window(focusParent, g_pTopLevelFramebuffer);
            }
            else if((c == KEY_LEFT) || (c == KEY_RIGHT) || (c == KEY_UP) || (c == KEY_DOWN))
            {
                if((c == KEY_LEFT) && (focusParent->getLayout() == Container::SideBySide))
                {
                    sibling = focusParent->getLeftSibling(g_pFocusWindow);
                }
                else if((c == KEY_UP) && (focusParent->getLayout() == Container::Stacked))
                {
                    sibling = focusParent->getLeftSibling(g_pFocusWindow);
                }
                else if((c == KEY_DOWN) && (focusParent->getLayout() == Container::Stacked))
                {
                    sibling = focusParent->getRightSibling(g_pFocusWindow);
                }
                else if((c == KEY_RIGHT) && (focusParent->getLayout() == Container::SideBySide))
                {
                    sibling = focusParent->getRightSibling(g_pFocusWindow);
                }

                if(sibling)
                {
                    while(sibling->getType() == WObject::Container)
                    {
                        // Need to make the focus the first non-Container child of the container.
                        Container *pContainer = static_cast<Container*>(sibling);
                        sibling = pContainer->getChild(0);
                    }

                    if(sibling->getType() == WObject::Window)
                    {
                        newFocus = static_cast<Window*>(sibling);
                    }
                }
                else
                {
                    // We need to find a left sibling, and give up if none exists.
                    // That is, perhaps this is the leftmost window inside a container
                    // that has containers to its left. We need to handle that.
                    syslog(LOG_INFO, "need to traverse further to find sibling");
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

    Window *pLeft = new Window(g_pRootContainer, g_pTopLevelFramebuffer);
    Window *pRight = new Window(g_pRootContainer, g_pTopLevelFramebuffer);
    g_pRootContainer->addChild(pMiddle);

    // Should be evenly split down the screen after retile()
    Window *pTop = new Window(pMiddle, g_pTopLevelFramebuffer);
    Window *pMiddleWindow = new Window(pMiddle, g_pTopLevelFramebuffer);
    Window *pBottom = new Window(pMiddle, g_pTopLevelFramebuffer);

    g_pFocusWindow = pLeft;
    pLeft->focus();

    if(pMiddle->getLeftSibling(pMiddleWindow) != pTop)
    {
        syslog(LOG_INFO, "winman: left sibling check is incorrect");
    }
    if(pMiddle->getRightSibling(pMiddleWindow) != pBottom)
    {
        syslog(LOG_INFO, "winman: right sibling check is incorrect");
    }
    if(pMiddle->getLeftSibling(pTop) != 0)
    {
        syslog(LOG_INFO, "winman: left sibling check with no left sibling is incorrect");
    }
    if(pMiddle->getRightSibling(pBottom) != 0)
    {
        syslog(LOG_INFO, "winman: right sibling check with no right sibling is incorrect");
    }
    */

    syslog(LOG_INFO, "winman: entering main loop %d", getpid());

    g_Windows = new std::map<uint64_t, Window*>();

    Input::installCallback(Input::Key | Input::Mouse, systemInputCallback);

    // Main loop: logic & message handling goes here!
    while(true)
    {
        checkForMessages(pEndpoint);
        g_pRootContainer->render();
        g_pTopLevelFramebuffer->redraw();

        sched_yield();
    }

    return 0;
}
