/*
 * Copyright (c) 2010 Matthew Iselin
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

#include "Ipv6.h"
#include <Module.h>
#include <Log.h>

// Protocols we use in IPv6
#include "Ethernet.h"
#include "Arp.h" /// \todo NDP

// Child protocols of IPv6
#include "Icmp.h"
#include "Udp.h"
#include "Tcp.h"

#include "NetManager.h"
#include "RawManager.h"
#include "Endpoint.h"

#include "RoutingTable.h"

#include "Filter.h"

Ipv6 Ipv6::ipInstance;

Ipv6::Ipv6() :
  m_NextIdLock(), m_IpId(0)
{
}

Ipv6::~Ipv6()
{
}

bool Ipv6::send(IpAddress dest, IpAddress from, uint8_t type, size_t nBytes, uintptr_t packet, Network *pCard)
{
    /// \todo Implement
    return false;
}

void Ipv6::receive(size_t nBytes, uintptr_t packet, Network* pCard, uint32_t offset)
{
    // Verify the inputs. Drivers may directly dump this information on us so
    // we cannot ever be too sure.
    if(!packet || !nBytes || !pCard)
        return;

    uintptr_t packetAddress = packet + offset;

    // Check for filtering
    /// \todo Add statistics to NICs
    if(!NetworkFilter::instance().filter(2, packetAddress, nBytes - offset))
        return;

    NOTICE("IPv6: received a packet!");

    // Grab the header
    ip6Header* header = reinterpret_cast<ip6Header*>(packetAddress);

    // Get the destination and source addresses
    IpAddress src(header->sourceAddress);
    IpAddress dest(header->destAddress);

    NOTICE("IPv6: packet from " << src.toString() << " to " << dest.toString());

    // Networking endpoint for the return path to the host sending this packet.
    Endpoint::RemoteEndpoint remoteHost;
    remoteHost.ip = src;

    // Header traversal.....
    uint8_t nextHeader = header->nextHeader;

    {
        switch(nextHeader)
        {
            case IP_TCP:
                NOTICE("TCP over IPv6");
                Tcp::instance().receive(src, nBytes, packetAddress, pCard, 0);
                break;
            case IP_UDP:
                NOTICE("UDP over IPv6");
                Udp::instance().receive(src, nBytes, packetAddress, pCard, 0);
                break;
            case IP_ICMPV6:
                NOTICE("ICMPv6 over IPv6");
                break;
        }
    }

    /// \todo Implement
}
