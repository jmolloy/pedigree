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
#include "protocol.h"
#include <ipc/Ipc.h>

/// \todo GTFO libc!
#include <string.h>

using namespace PedigreeIpc;
using namespace LibUiProtocol;

bool LibUiProtocol::sendMessage(void *pMessage, size_t messageLength)
{
    // Grab the endpoint for the window manager.
    IpcEndpoint *pEndpoint = getEndpoint("pedigree-winman");
    if(!pEndpoint)
    {
        /// \todo Log the error somewhere.
        return false;
    }

    // Create an initial message. This will be used for a handshake if the actual
    // message is longer than 4 KB in size, and for the actual message if not.
    if(messageLength > 0x1000)
    {
        /// \todo Implement buffer-negotiate handshake.
        return false;
    }
    IpcMessage *pIpcMessage = new IpcMessage();
    if(!pIpcMessage->initialise())
    {
        return false;
    }

    // Fill the message with the data to transmit. Callers should have the proper
    // window manager message structures in the buffer already.
    void *pDest = pIpcMessage->getBuffer();
    if(!pDest)
    {
        delete pIpcMessage;
        return false;
    }
    memcpy(pDest, pMessage, messageLength);

    // Transmit the message.
    send(pEndpoint, pIpcMessage, false);

    // Clean up.
    // delete pIpcMessage;

    return true;
}

bool LibUiProtocol::recvMessage(const char *endpoint, void *pBuffer, size_t maxSize)
{
    /// \todo Handle messages > 4 KB in size!
    if(maxSize > 0x1000)
        return false;

    // Grab the endpoint for the window manager.
    IpcEndpoint *pEndpoint = getEndpoint(endpoint);
    if(!pEndpoint)
    {
        /// \todo Log the error somewhere.
        return false;
    }

    // Block and wait for a message to appear on the endpoint.
    /// \todo We need to take a widget handle here so we can resend messages
    ///       that we don't actually want (eg, those for another process'
    ///       widgets!)
    IpcMessage *pRecv = 0;
    recv(pEndpoint, &pRecv, false);

    // Verify.
    if((!pRecv) || (!pRecv->getBuffer()))
    {
        return false;
    }

    // Copy the message across.
    /// \todo When messages > 4 KB are made possible, sanitise maxSize here!
    memcpy(pBuffer, pRecv->getBuffer(), maxSize);

    // Clean up.
    // delete pRecv;

    return true;
}
