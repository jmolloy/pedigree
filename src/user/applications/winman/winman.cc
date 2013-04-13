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

Window *g_pFocusWindow = 0;

class WindowMetadata {
    public:
        PedigreeGraphics::Framebuffer *pFramebuffer;
        char endpoint[256];
};

std::map<uint64_t, WindowMetadata*> *g_Windows;

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

        WindowMetadata *pMetadata = new WindowMetadata;
        strcpy(pMetadata->endpoint, pCreate->endpoint);

        /// \todo Create a new window PROPERLY.
        syslog(LOG_INFO, "winman: create %dx%d", pCreate->minWidth, pCreate->minHeight);
        pMetadata->pFramebuffer = g_pTopLevelFramebuffer->createChild(0, 0, pCreate->minWidth, pCreate->minHeight);

        LibUiProtocol::WindowManagerMessage *pHeader =
            reinterpret_cast<LibUiProtocol::WindowManagerMessage*>(responseData);
        pHeader->messageCode = LibUiProtocol::Create;
        pHeader->widgetHandle = pWinMan->widgetHandle;
        pHeader->messageSize = sizeof(LibUiProtocol::CreateMessageResponse);
        pHeader->isResponse = true;

        LibUiProtocol::CreateMessageResponse *pCreateResp =
            reinterpret_cast<LibUiProtocol::CreateMessageResponse*>(responseData + sizeof(LibUiProtocol::WindowManagerMessage));
        pCreateResp->provider = pMetadata->pFramebuffer->getProvider();

        syslog(LOG_INFO, "winman: create message!");
        g_Windows->insert(std::make_pair(pWinMan->widgetHandle, pMetadata));
        syslog(LOG_INFO, "winman: metadata entered");

        syslog(LOG_INFO, "winman: getting endpoint for IPC");
        PedigreeIpc::IpcEndpoint *pEndpoint = PedigreeIpc::getEndpoint(pMetadata->endpoint);
        syslog(LOG_INFO, "winman: endpoint retrieved [%s] %p", pMetadata->endpoint, pEndpoint);

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

    /*
    // Make a program to test window creation etc...
    if(fork() == 0)
    {
        syslog(LOG_INFO, "winman: child alive %d", getpid());
        char *const new_argv[] = {"/applications/gears"};
        execv(new_argv[0], new_argv);
        return 0;
    }
    */

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

    RootContainer *pRootContainer = new RootContainer(g_nWidth, g_nHeight);

    Window *pLeft = new Window(pRootContainer, g_pTopLevelFramebuffer);
    Window *pRight = new Window(pRootContainer, g_pTopLevelFramebuffer);

    Container *pMiddle = new Container(pRootContainer);
    pMiddle->setLayout(Container::Stacked);

    Container *pMiddleAgain = new Container(pMiddle);

    Window *pTop = new Window(pMiddle, g_pTopLevelFramebuffer);
    Window *pMiddleWindow = new Window(pMiddle, g_pTopLevelFramebuffer);
    Window *pBottom = new Window(pMiddle, g_pTopLevelFramebuffer);

    Window *pTopAgain = new Window(pMiddleAgain, g_pTopLevelFramebuffer);
    Window *pMiddleWindowAgain = new Window(pMiddleAgain, g_pTopLevelFramebuffer);
    Window *pBottomAgain = new Window(pMiddleAgain, g_pTopLevelFramebuffer);

    // Should be evenly split down the screen after retile()
    pRootContainer->addChild(pLeft);
    pRootContainer->addChild(pMiddle);
    pRootContainer->addChild(pRight);

    // Should be evenly split in the middle tile after retile()
    pMiddle->addChild(pTop);
    pMiddle->addChild(pMiddleWindow);
    pMiddle->addChild(pBottom);
    pMiddle->addChild(pMiddleAgain);

    pMiddleAgain->addChild(pTopAgain);
    pMiddleAgain->addChild(pMiddleWindowAgain);
    pMiddleAgain->addChild(pBottomAgain);

    g_pFocusWindow = pLeft;
    pLeft->focus();

    syslog(LOG_INFO, "winman: pushing middle %p out 128 pixels", static_cast<WObject*>(pMiddle));
    pMiddleAgain->resize(128, 0);

    syslog(LOG_INFO, "winman: entering main loop %d", getpid());

    g_Windows = new std::map<uint64_t, WindowMetadata*>();

    // Main loop: logic & message handling goes here!
    while(true)
    {
        //checkForMessages(pEndpoint);
        pRootContainer->render();
        g_pTopLevelFramebuffer->redraw();

        sched_yield();
    }

    return 0;
}
