/*
 * Copyright (c) 2011 Matthew Iselin, Heights College
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
#include "Icmpv6.h"
#include "IpCommon.h"
#include "Ipv6.h"
#include "Ndp.h"

#include "RoutingTable.h"

#include <network/IpAddress.h>

#define ICMPV6_ECHOREQ      128
#define ICMPV6_ECHORESP     129

#define ICMPV6_RSOLICIT     133
#define ICMPV6_RADVERT      134
#define ICMPV6_NSOLICIT     135
#define ICMPV6_NADVERT      136

Icmpv6 Icmpv6::icmpInstance;

Icmpv6::Icmpv6()
{
}

Icmpv6::~Icmpv6()
{
}

void Icmpv6::receive(IpAddress from, IpAddress to, uintptr_t packet, size_t nBytes, IpBase *pIp, Network* pCard)
{
    icmpv6Header *pHeader = reinterpret_cast<icmpv6Header*>(packet);

    uint16_t checksum = Ipv6::instance().ipChecksum(from, to, IP_ICMPV6, packet, nBytes);
    if(checksum)
    {
        WARNING("ICMPv6: checksum incorrect on incoming packet from " << from.toString());
        pCard->badPacket();
        return;
    }

    switch(pHeader->type)
    {
        case ICMPV6_ECHOREQ:
            // Echo request. Turn it around and send it right back!
            /// \todo Verify 'to' is unicast.
            send(from, to, ICMPV6_ECHORESP, pHeader->code, packet + sizeof(icmpv6Header), nBytes - sizeof(icmpv6Header), pCard);
            break;

        case ICMPV6_RSOLICIT:
        case ICMPV6_RADVERT:
        case ICMPV6_NSOLICIT:
        case ICMPV6_NADVERT:
            Ndp::instance().receive(from, to, pHeader->type, pHeader->code, packet + sizeof(icmpv6Header), nBytes - sizeof(icmpv6Header), pCard);
            break;
    }
}

void Icmpv6::send(IpAddress dest, IpAddress from, uint8_t type, uint8_t code, uintptr_t payload, size_t nBytes, Network *pCard)
{
    if(dest.isMulticast() && !pCard)
    {
        WARNING("ICMPv6: Packet had a multicast destination, but no given network interface. Can't figure out where to send this!");
        return;
    }

    // This use of the routing table is merely to determine which NIC to send
    // on, so we can change the from address if needed. IPv6::send does the
    // actual routing of packets for external access.
    if(dest.isUnicast() && !dest.isLinkLocal())
    {
      IpAddress tmp = dest;
      pCard = RoutingTable::instance().DetermineRoute(&tmp);
      if(!pCard)
      {
        WARNING("ICMPv6: Couldn't find a route for destination '" << dest.toString() << "'.");
        return;
      }
    }

    if(dest.isLinkLocal())
      pCard = RoutingTable::instance().DefaultRouteV6();

    StationInfo me = pCard->getStationInfo();
    if(!me.nIpv6Addresses)
        return; // We're not configured yet.

    uintptr_t packet = NetworkStack::instance().getMemPool().allocate();

    icmpv6Header* header = reinterpret_cast<icmpv6Header*>(packet);
    header->type = type;
    header->code = code;

    if(nBytes)
        memcpy(reinterpret_cast<void*>(packet + sizeof(icmpv6Header)), reinterpret_cast<void*>(payload), nBytes);

    header->checksum = 0;
    header->checksum = Ipv6::instance().ipChecksum(from, dest, IP_ICMPV6, packet, sizeof(icmpv6Header) + nBytes);

    Ipv6::instance().send(dest, from, IP_ICMPV6, nBytes + sizeof(icmpv6Header), packet, pCard);

    NetworkStack::instance().getMemPool().free(packet);
}
