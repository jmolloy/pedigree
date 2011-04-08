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

#include <network/IpAddress.h>

#define ICMPV6_ECHOREQ      128
#define ICMPV6_ECHORESP     129
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
    NOTICE("Incoming ICMPv6 from " << from.toString() << " to " << to.toString());

    icmpv6Header *pHeader = reinterpret_cast<icmpv6Header*>(packet);

    NOTICE("Type: " << pHeader->type << ", Code: " << pHeader->code);

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

        case ICMPV6_NSOLICIT:
            Ndp::instance().receive(from, to, pHeader->type, pHeader->code, packet + sizeof(icmpv6Header), nBytes - sizeof(icmpv6Header), pCard);
            break;
    }
}

void Icmpv6::send(IpAddress dest, IpAddress from, uint8_t type, uint8_t code, uintptr_t payload, size_t nBytes, Network *pCard)
{
    StationInfo me = pCard->getStationInfo();
    if(!me.nIpv6Addresses)
        return; // We're not configured yet.

    uintptr_t packet = NetworkStack::instance().getMemPool().allocate();

    icmpv6Header* header = reinterpret_cast<icmpv6Header*>(packet);
    header->type = type;
    header->code = code;

    if(nBytes)
        memcpy(reinterpret_cast<void*>(packet + sizeof(icmpv6Header)), reinterpret_cast<void*>(payload), nBytes);

    /// \todo Assumption for which IPv6 address to use.
    header->checksum = 0;
    header->checksum = Ipv6::instance().ipChecksum(me.ipv6[0], dest, IP_ICMPV6, packet, sizeof(icmpv6Header) + nBytes);

    Ipv6::instance().send(dest, from, IP_ICMPV6, nBytes + sizeof(icmpv6Header), packet, pCard);

    NetworkStack::instance().getMemPool().free(packet);
}
