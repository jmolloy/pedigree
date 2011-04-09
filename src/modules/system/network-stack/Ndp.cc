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
#include "Ipv6.h"

#include "NetworkStack.h"

#include "RoutingTable.h"

Ndp Ndp::ndpInstance;

#define NDP_RSOLICIT    133
#define NDP_RADVERT     134
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
        // Don't care about router solicit messages, they are not relevant.
        case NDP_RADVERT:
            {
                RouterAdvertisement *pMessage = reinterpret_cast<RouterAdvertisement*>(payload);

                /// \todo Use the "current hop limit" to change IPv6's default
                ///       hop limit.

                // Parse options to get prefix and MTU information.
                Option *pOption = reinterpret_cast<Option*>(payload + sizeof(RouterAdvertisement));
                if(nBytes > sizeof(RouterAdvertisement))
                {
                    while(reinterpret_cast<uintptr_t>(pOption) < (payload + nBytes))
                    {
                        // Prefix information.
                        if(pOption->type == 0x3)
                        {
                            PrefixInformationOption *pPrefix = reinterpret_cast<PrefixInformationOption*>(pOption);

                            IpAddress prefix(pPrefix->prefix);

                            /// \todo Store the router address somewhere. We need it to route packets!

                            // Have we already associated with this router?
                            /// \todo Cache this rather than recalculate on each advertisement.
                            bool bAlreadyAssociated = false;
                            for(size_t i = 0; i < me.nIpv6Addresses; i++)
                            {
                                uint8_t ip[6];
                                me.ipv6[i].getIp(ip);

                                if(!memcmp(ip, pPrefix->prefix, pPrefix->prefixLength / 8))
                                {
                                    // Already got it.
                                    bAlreadyAssociated = true;
                                    break;
                                }
                            }

                            // If we are allowed to allocate autoconfiguration addresses from this prefix, do so.
                            if((!bAlreadyAssociated) && (pPrefix->rsvdFlags & 0x40))
                            {
                                // Create an IPv6-modified EUI64.
                                uint8_t eui[8];
                                Ipv6::getIpv6Eui64(me.mac, eui);

                                // Throw it into the base of the new IPv6 address we are creating here.
                                uint8_t newIpv6[16] = {0};
                                memcpy(newIpv6 + 8, eui, 8);

                                // Add the prefix now.
                                memcpy(newIpv6, pPrefix->prefix, pPrefix->prefixLength / 8);

                                IpAddress *pNewAddress = 0;

                                // Add it to the IPv6 list.
                                if(!me.nIpv6Addresses)
                                {
                                    me.ipv6 = new IpAddress(newIpv6);
                                    me.ipv6->setIpv6Prefix(pPrefix->prefixLength);
                                    me.nIpv6Addresses = 1;

                                    pNewAddress = me.ipv6;
                                }
                                else
                                {
                                    // Need to add a new address.
                                    size_t currAddresses = me.nIpv6Addresses;
                                    IpAddress *pNew = new IpAddress[currAddresses + 1];
                                    for(size_t i = 0; i < currAddresses; i++)
                                        pNew[i] = me.ipv6[i];

                                    pNew[currAddresses] = IpAddress(newIpv6);
                                    pNew[currAddresses].setIpv6Prefix(pPrefix->prefixLength);

                                    delete [] me.ipv6;
                                    me.ipv6 = pNew;

                                    me.nIpv6Addresses++;

                                    pNewAddress = &pNew[currAddresses];
                                }

                                // Update the NIC's information for the rest of the stack to use.
                                pCard->setStationInfo(me);

                                // Add routes for this prefix.
                                uint8_t localhost[16] = {0}; localhost[15] = 1;
                                RoutingTable::instance().Add(RoutingTable::DestIpv6Sub, *pNewAddress, IpAddress(localhost), String(""), pCard);
                                RoutingTable::instance().Add(RoutingTable::DestPrefix, *pNewAddress, IpAddress(), IpAddress(), String(""), pCard);
                                RoutingTable::instance().Add(RoutingTable::DestPrefixComplement, *pNewAddress, IpAddress(), from, String(""), pCard);

                                // Add as a default route if none exists
                                if(!RoutingTable::instance().DefaultRouteV6())
                                    RoutingTable::instance().Add(RoutingTable::NamedV6, IpAddress(), IpAddress(), String("default"), pCard);
                            }
                        }

                        pOption = reinterpret_cast<Option*>(reinterpret_cast<uintptr_t>(pOption) + (pOption->length * 8));
                    }
                }
            }
            break;

        case NDP_SOLICIT:
            {
                NeighbourSolicitation *pMessage = reinterpret_cast<NeighbourSolicitation*>(payload);

                // Is this a solicitation for us?
                IpAddress packetTarget = pMessage->target;

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
                packetTarget.getIp(pReply->target);
                pReply->flags = NADVERT_FLAGS_SOLICIT << 5;

                // Add the options we need.
                LinkLayerAddressOption *pAddrOption = reinterpret_cast<LinkLayerAddressOption*>(packet + sizeof(NeighbourAdvertisement));
                pAddrOption->type = 2;
                pAddrOption->length = sizeof(LinkLayerAddressOption) / 8;
                memcpy(pAddrOption->address, me.mac.getMac(), 6);

                // Send the response.
                Icmpv6::instance().send(from, *pMatch, NDP_ADVERT, 0, packet, sizeof(NeighbourAdvertisement) + sizeof(LinkLayerAddressOption), pCard);
                NetworkStack::instance().getMemPool().free(packet);

            }
            break;
        case NDP_ADVERT:
            {
                NOTICE("NDP Advertisement from " << from.toString() << ".");
            }
            break;
    };
}

bool Ndp::routerSolicit(Network *pCard)
{
    StationInfo me = pCard->getStationInfo();

    // Only need a link-local address for this: don't want a routable address
    // as we're looking for a local router.
    IpAddress from;
    size_t i;
    for(i = 0; i < me.nIpv6Addresses; i++)
    {
        if(me.ipv6[i].isLinkLocal())
        {
            from = me.ipv6[i];
            break;
        }
    }
    if(i == me.nIpv6Addresses)
        return false;

    uintptr_t packet = NetworkStack::instance().getMemPool().allocate();
    RouterSolicitation *solicit = reinterpret_cast<RouterSolicitation*>(packet);
    memset(solicit, 0, sizeof(RouterSolicitation));

    // Broadcast to all routers.
    /// \todo Implement some way of creating "special" IPv6 addresses... neatly.
    uint8_t dest[] = {0xFF, 0x02, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
                       0x0,  0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x2};
    IpAddress to(dest);

    // Send the solicit packet.
    Icmpv6::instance().send(to, from, NDP_RSOLICIT, 0, packet, sizeof(RouterSolicitation), pCard);
    NetworkStack::instance().getMemPool().free(packet);

    // No need to wait: the advertisement will come in shortly (if any) and the
    // receive handler will handle the configuration outcomes.
    return true;
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

    StationInfo me = pCard->getStationInfo();

    /// \todo Find an address with the same PREFIX as the NEIGHBOUR we want to
    ///       solicit. IpAddress needs to store prefix length for IPv6.

    IpAddress from;
    size_t i;
    for(i = 0; i < me.nIpv6Addresses; i++)
    {
        if(!me.ipv6[i].isLinkLocal())
        {
            from = me.ipv6[i];
            break;
        }
    }
    if(i == me.nIpv6Addresses)
        return false;

    // Okay, we'll have to send a Neighbour Solicit.
    uintptr_t packet = NetworkStack::instance().getMemPool().allocate();
    NeighbourSolicitation *solicit = reinterpret_cast<NeighbourSolicitation*>(packet);
    solicit->reserved = 0;
    addr.getIp(solicit->target);

    // Add an option with our MAC address.
    LinkLayerAddressOption *pAddrOption = reinterpret_cast<LinkLayerAddressOption*>(packet + sizeof(NeighbourSolicitation));
    pAddrOption->type = 1;
    pAddrOption->length = sizeof(LinkLayerAddressOption) / 8;
    memcpy(pAddrOption->address, me.mac.getMac(), 6);

    // Broadcast over the local network segment.
    /// \todo Implement some way of creating "special" IPv6 addresses... neatly.
    uint8_t dest[] = {0xFF, 0x02, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
                       0x0,  0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1};
    IpAddress to(dest);

    // Send the solicit packet.
    Icmpv6::instance().send(to, from, NDP_SOLICIT, 0, packet, sizeof(NeighbourSolicitation) + sizeof(LinkLayerAddressOption), pCard);
    NetworkStack::instance().getMemPool().free(packet);

    // Not yet implemented & working properly.
    /// \todo Needs to wait until a response comes in or a timeout occurs.
    return false;
}
