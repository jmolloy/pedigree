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

#include <stdio.h>
#include <native/ipc/Ipc.h>

#include <syslog.h>

using namespace PedigreeIpc;

int main(int argc, char *argv[])
{
    printf("IPC Test: Client\n");

    // Grab an endpoint to use.
    createEndpoint("ipc-test");
    IpcEndpoint *pEndpoint = getEndpoint("ipc-test");

    // Create a "Hello World!" message.
    IpcMessage *pMessage = new IpcMessage();

    if(!pMessage)
        syslog(LOG_ERR, "operator new returned null");

    if(!pMessage->initialise())
    {
        printf("Message couldn't be initialised.\n");
        return 1;
    }

    char *pBuffer = reinterpret_cast<char *>(pMessage->getBuffer());
    if(!pBuffer)
    {
        printf("Message creation failed.\n");
        return 1;
    }
    else
        printf("Writing into message %x\n", pBuffer);

    sprintf(pBuffer, "Hello, world!\n");

    printf("Sending message...\n");

    // Send the message.
    send(pEndpoint, pMessage, false);

    printf("Message has been sent, waiting for response...\n");

    // Wait for a response from the IPC test server.
    IpcMessage *pRecv = 0;
    recv(pEndpoint, &pRecv, false);

    // Display it.
    printf("Got '%s' from the IPC server.\n", pRecv->getBuffer());

    // Clean up.
    delete pMessage;
    delete pRecv;

    // All done.
    printf("IPC Test: Client completed.\n");

    return 0;
}
