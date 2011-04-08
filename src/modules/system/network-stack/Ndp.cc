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
#include "Ndp.h"
#include "Icmpv6.h"

#include "NetworkStack.h"

Ndp Ndp::ndpInstance;

#define NDP_SOLICIT     135
#define NDP_ADVERT      136

Ndp::Ndp() : m_LookupCache()
{
}

Ndp::~Ndp()
{
}

void Ndp::addEntry(IpAddress addr, MacAddress mac)
{
    if(m_LookupCache.lookup(addr.toString()))
        return;

    MacAddress *pNew = new MacAddress(mac);
    m_LookupCache.insert(addr.toString(), pNew);
}

void Ndp::receive(IpAddress from, IpAddress to, uint8_t icmpType, uint8_t icmpCode, uintptr_t payload, size_t nBytes, Network *pCard)
{
    StationInfo me = pCard->getStationInfo();

    // Handle based on the ICMP code.
    switch(icmpType)
    {
        case NDP_SOLICIT:
            {
                NeighbourSolicitation *pMessage = reinterpret_cast<NeighbourSolicitation*>(payload);

                // Is this a solicitation for us?
                IpAddress packetTarget = pMessage->target;
                NOTICE("NDP: Neighbour solicitation for " << packetTarget.toString());

                bool bMatch = false;
                IpAddress *pMatch = 0;
                for(size_t i = 0; i < me.nIpv6Addresses; i++)
                {
                    if(me.ipv6[i] == packetTarget)
                    {
                        pMatch = &me.ipv6[i];
                        bMatch = true;
                        break;
                    }
                }

                if(!bMatch)
                {
                    // Not for us, ignore.
                    return;
                }

                // Grab the link-layer address of the sender, if any, and add it to our cache.
                Option *pOption = reinterpret_cast<Option*>(payload + sizeof(NeighbourSolicitation));
                if(nBytes > sizeof(NeighbourSolicitation))
                {
                    while(reinterpret_cast<uintptr_t>(pOption) < (payload + nBytes))
                    {
                        // Source/Target link-layer address.
                        if(pOption->type & 0x3)
                        {
                            LinkLayerAddressOption *p = reinterpret_cast<LinkLayerAddressOption*>(pOption);
                            MacAddress mac;
                            mac.setMac(p->address);

                            if(p->type == 1)
                            {
                                // Add to our cache.
                                addEntry(from, mac);
                            }
                        }

                        pOption = reinterpret_cast<Option*>(reinterpret_cast<uintptr_t>(pOption) + (pOption->length * 8));
                    }
                }

                // This is for us, so we want to send an advertisement to that host.
                uintptr_t packet = NetworkStack::instance().getMemPool().allocate();
                NeighbourAdvertisement *pReply = reinterpret_cast<NeighbourAdvertisement*>(packet);
                //from.getIp(pReply->target);
                packetTarget.getIp(pReply->target);
                pReply->flags = NADVERT_FLAGS_SOLICIT << 5;

                // Add the options we need.
                LinkLayerAddressOption *pAddrOption = reinterpret_cast<LinkLayerAddressOption*>(packet + sizeof(NeighbourAdvertisement));
                pAddrOption->type = 2;
                pAddrOption->length = sizeof(LinkLayerAddressOption) / 8;
                memcpy(pAddrOption->address, me.mac.getMac(), 6);

                // Send the response.
                Icmpv6::instance().send(from, *pMatch, NDP_ADVERT, 0, packet, sizeof(NeighbourAdvertisement) + sizeof(LinkLayerAddressOption), pCard);

            }
            break;
    };
}

bool Ndp::neighbourSolicit(IpAddress addr, MacAddress *pMac, Network *pCard)
{
    // Easy lookup?
    MacAddress *p;
    if((p = m_LookupCache.lookup(addr.toString())))
    {
        *pMac = *p;
        return true;
    }

    // Okay, we'll have to send a Neighbour Solicit.
    /// \todo Do that.

    WARNING("NDP: neighbour solicit not yet implemented.");

    return false;
}
