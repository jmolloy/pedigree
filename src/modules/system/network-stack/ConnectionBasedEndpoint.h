/*
 * Copyright (c) 2008 James Molloy, Jörg Pfähler, Matthew Iselin
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
#ifndef _CONNECTION_BASED_ENDPOINT_H
#define _CONNECTION_BASED_ENDPOINT_H

#include "Endpoint.h"

/**
 * ConnectionBasedEndpoint: Endpoint specialisation for connection-based
 * protocols (such as TCP).
 */
class ConnectionBasedEndpoint : public Endpoint
{
private:
    ConnectionBasedEndpoint(const ConnectionBasedEndpoint &e);
    const ConnectionBasedEndpoint& operator = (const ConnectionBasedEndpoint& e);
public:
    ConnectionBasedEndpoint() :
            Endpoint()
    {};
    ConnectionBasedEndpoint(uint16_t local, uint16_t remote) :
            Endpoint(local, remote)
    {};
    ConnectionBasedEndpoint(IpAddress remoteIp, uint16_t local = 0, uint16_t remote = 0) :
            Endpoint(remoteIp, local, remote)
    {};
    virtual ~ConnectionBasedEndpoint() {};

    EndpointType getType()
    {
        return ConnectionBased;
    }

    /** Grabs an integer representation of the connection state */
    virtual int state()
    {
        return 0xff;
    }

    /** Connects to the given remote host */
    virtual bool connect(Endpoint::RemoteEndpoint remoteHost, bool bBlock = true)
    {
        return false;
    }

    /** Closes the connection */
    virtual void close()
    {
    }

    /**
     * Puts the connection into the listening state, waiting for incoming
     * connections.
     */
    virtual void listen()
    {
    }

    /**
     * Blocks until an incoming connection is available, then accepts it
     * and returns an Endpoint for that connection.
     */
    virtual Endpoint* accept()
    {
        return 0;
    }

    /**
     * Sends nBytes of buffer
     * \param nBytes the number of bytes to send
     * \param buffer the buffer to send
     * \returns -1 on failure, the number of bytes sent otherwise
     */
    virtual int send(size_t nBytes, uintptr_t buffer)
    {
        return -1;
    }

    /**
     * Receives from the network into the given buffer
     * \param buffer the buffer to receive into
     * \param maxSize the size of the buffer
     * \param block whether or not to block
     * \param bPeek whether or not to keep messages in the data buffer
     * \returns -1 on failure, the number of bytes received otherwise
     */
    virtual int recv(uintptr_t buffer, size_t maxSize, bool block, bool bPeek)
    {
        return -1;
    }

    /** Retrieves the connection ID for this connection. */
    virtual inline uint32_t getConnId()
    {
        return 0;
    }

    /**
     * Because TCP works with RemoteEndpoints a lot, it's easier to set our
     * internal state using this kind of function rather than several calls
     * to the setXyz functions.
     */
    virtual void setRemoteHost(RemoteEndpoint host)
    {
    }
};

#endif