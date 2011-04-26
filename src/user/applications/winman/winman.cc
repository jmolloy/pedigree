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

#include <graphics/Graphics.h>
#include <input/Input.h>
#include <ipc/Ipc.h>

#include <protocol.h>

using namespace LibUiProtocol;
using namespace PedigreeIpc;

PedigreeGraphics::Framebuffer *g_pTopLevelFramebuffer = 0;

void handleMessage(char *messageData)
{
    WindowManagerMessage *pWinMan = reinterpret_cast<WindowManagerMessage*>(messageData);

    printf("Incoming message at %x\n", messageData);

    if(pWinMan->messageCode == Create)
        printf("Create message!\n");

    printf("Incoming message with code %d\n", pWinMan->messageCode);
}

void checkForMessages(IpcEndpoint *pEndpoint)
{
    if(!pEndpoint)
        return;

    IpcMessage *pRecv = 0;
    if(recv(pEndpoint, &pRecv, false))
    {
        handleMessage(static_cast<char *>(pRecv->getBuffer()));

        delete pRecv;
    }
}

int main(int argc, char *argv[])
{
    printf("Starting up the window manager...\n");

    // Daemonize!
    if(fork() != 0)
    {
        printf("Success.\n");
        return 0;
    }

    // Create the window manager IPC endpoint for libui.
    createEndpoint("pedigree-winman");
    IpcEndpoint *pEndpoint = getEndpoint("pedigree-winman");

    if(!pEndpoint)
    {
        printf("error: couldn't create the pedigree-winman IPC endpoint!\n");
        return 0;
    }

    PedigreeGraphics::Framebuffer *pRootFramebuffer = new PedigreeGraphics::Framebuffer();
    g_pTopLevelFramebuffer = pRootFramebuffer->createChild(512, 32, pRootFramebuffer->getWidth() - 512, pRootFramebuffer->getHeight());

    if(!g_pTopLevelFramebuffer->getRawBuffer())
    {
        printf("error: no top-level framebuffer could be created.\n");
        return -1;
    }

    size_t g_nWidth = g_pTopLevelFramebuffer->getWidth();
    size_t g_nHeight = g_pTopLevelFramebuffer->getHeight();

    g_pTopLevelFramebuffer->rect(0, 0, g_nWidth, g_nHeight, PedigreeGraphics::createRgb(0, 0, 255), PedigreeGraphics::Bits32_Rgb);

    printf("Entering main loop (%d,%d).\n", g_nWidth, g_nHeight);

    // Main loop: logic & message handling goes here!
    while(true)
    {
        checkForMessages(pEndpoint);
        g_pTopLevelFramebuffer->redraw();
    }

    return 0;
}
