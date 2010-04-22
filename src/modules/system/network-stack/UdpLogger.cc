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
#include "UdpLogger.h"
#include <network/IpAddress.h>
#include "NetworkStack.h"
#include "UdpManager.h"
#include "RoutingTable.h"

UdpLogger::~UdpLogger()
{
    if(m_pEndpoint)
    {
        UdpManager::instance().returnEndpoint(m_pEndpoint);
        m_pEndpoint = 0;
    }
}

bool UdpLogger::initialise(IpAddress remote, uint16_t port)
{
    if(m_pEndpoint)
        return true;
        
    m_LoggingServer.ip = remote;
    m_LoggingServer.remotePort = port;
    
    m_pEndpoint = static_cast<ConnectionlessEndpoint*>(UdpManager::instance().getEndpoint(remote, 0, port));
    
    if(!m_pEndpoint)
        return false;
    
    // Will perform an ARP lookup, which will fill the ARP cache. This is done
    // before the callback is actually installed (after we return) because ARP
    // writes to the log, which calls the callback (and then hangs on ARP)
    callback("UDP logger now active");
    
    return true;
}

void UdpLogger::callback(const char *str)
{
    if(!m_pEndpoint)
        return;
    
    m_pEndpoint->send(strlen(str), reinterpret_cast<uintptr_t>(str), m_LoggingServer, false);
}

