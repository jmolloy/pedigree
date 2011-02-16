/*
 * Copyright (c) 2011 Matthew Iselin
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
#include <ipc/Ipc.h>

#include <unistd.h>
#include <syslog.h>

using namespace PedigreeIpc;

int main(int argc, char *argv[])
{
    printf("IPC Test: Server, daemonising\n");

    pid_t id = fork();
    if(!id)
    {
        // Grab an endpoint to use.
        createEndpoint("ipc-test");
        IpcEndpoint *pEndpoint = getEndpoint("ipc-test");

        syslog(LOG_NOTICE, "IPC Test: Server started and entering message loop.");

        while(true)
        {
            // Wait for an incoming message.
            IpcMessage *pRecv = 0;
            if(!recv(pEndpoint, &pRecv, false))
            {
                syslog(LOG_WARNING, "IPC Test: Server failed to receive a message.");
                continue;
            }

            // Log it.
            syslog(LOG_NOTICE, "IPC Test: Server got message '%s'.", pRecv->getBuffer());
            delete pRecv;

            // Send a response.
            IpcMessage *pResponse = new IpcMessage();
            pResponse->initialise();
            char *pBuffer = reinterpret_cast<char *>(pResponse->getBuffer());
            if(!pBuffer)
            {
                syslog(LOG_WARNING, "IPC Test: Server message creation failed.");
                continue;
            }
            else
                syslog(LOG_NOTICE, "IPC Test: Server is writing into message %x", pBuffer);

            sprintf(pBuffer, "Server received message successfully!\n");

            if(!send(pEndpoint, pResponse, false))
                syslog(LOG_WARNING, "IPC Test: Server failed to send a response.");

            // Clean up.
            delete pResponse;
        }
    }
    else
        printf("Successfully forked, child has PID %d\n", id);

    return 0;
}
