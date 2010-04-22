/*
 * Copyright (c) 2010 Matthew Iselin, Heights College
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
#ifndef _NETWORK_UDP_LOGGER_H
#define _NETWORK_UDP_LOGGER_H

#include <processor/types.h>
#include <network/IpAddress.h>
#include "ConnectionlessEndpoint.h"
#include "NetworkStack.h"
#include "UdpManager.h"
#include <Log.h>

/** Defines a UDP-based callback for Log entries. */
class UdpLogger : public Log::LogCallback
{
    public:
        UdpLogger() : m_pEndpoint(0), m_LoggingServer()
        {}
        virtual ~UdpLogger();
        
        bool initialise(IpAddress remote, uint16_t port = 1234);
        
        void callback(const char *str);
    
    private:
        ConnectionlessEndpoint *m_pEndpoint;
        
        Endpoint::RemoteEndpoint m_LoggingServer;
};

#endif
