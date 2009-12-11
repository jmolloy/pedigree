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
#ifndef _CONNECTIONLESS_ENDPOINT_H
#define _CONNECTIONLESS_ENDPOINT_H

#include "Endpoint.h"

/**
 * ConnectionlessEndpoint: Endpoint specialisation for connectionless
 * protocols (such as UDP).
 */
class ConnectionlessEndpoint : public Endpoint
{
private:
    ConnectionlessEndpoint(const ConnectionlessEndpoint &e);
    const ConnectionlessEndpoint& operator = (const ConnectionlessEndpoint& e);
public:
    ConnectionlessEndpoint() :
            Endpoint()
    {};
    ConnectionlessEndpoint(uint16_t local, uint16_t remote) :
            Endpoint(local, remote)
    {};
    ConnectionlessEndpoint(IpAddress remoteIp, uint16_t local = 0, uint16_t remote = 0) :
            Endpoint(remoteIp, local, remote)
    {};
    virtual ~ConnectionlessEndpoint() {};

    EndpointType getType()
    {
        return Connectionless;
    }

    /**
     * Sends nBytes of buffer to the given remote host.
     * \param nBytes the number of bytes to send
     * \param buffer the buffer to send
     * \param remoteHost identifies the remote host
     * \param broadcast broadcast over the network
     * \returns -1 on failure, the number of bytes sent otherwise
     */
    virtual int send(size_t nBytes, uintptr_t buffer, RemoteEndpoint remoteHost, bool broadcast, Network *pCard = 0)
    {
        return -1;
    };

    /**
     * Receives from the network into the given buffer.
     * \param buffer the buffer to receive into
     * \param maxSize the size of the buffer
     * \param bBlock whether or not to block for data
     * \param remoteHost on successful return, identifies the remote host
     * \returns -1 on failure, the number of bytes received otherwise
     */
    virtual int recv(uintptr_t buffer, size_t maxSize, bool bBlock, RemoteEndpoint* remoteHost, int nTimeout = 30)
    {
        return -1;
    };

    /**
     * Whether or not to accept incoming communications from any address on
     * this Endpoint. This is used for things like DHCP which don't know an
     * IP for their remote host until they receive the first packet.
     */
    virtual inline bool acceptAnyAddress()
    {
        return false;
    };

    /**
     * This allows the acceptAnyAddress behaviour to be turned on and off.
     */
    virtual inline void acceptAnyAddress(bool accept)
    {
    };
};

#endif
