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
#include "Ndp.h"

// Child protocols of IPv6
#include "Icmpv6.h"
#include "Udp.h"
#include "Tcp.h"

#include "NetManager.h"
#include "RawManager.h"
#include "Endpoint.h"

#include "RoutingTable.h"

#include "Filter.h"

Ipv6 Ipv6::ipInstance;

Ipv6Service *g_pIpv6Service = 0;
ServiceFeatures *g_pIpv6Features = 0;

void Ipv6::getIpv6Eui64(MacAddress mac, uint8_t *eui)
{
    memcpy(eui, mac.getMac(), 3);
    memcpy(eui + 5, mac.getMac() + 3, 3);
    eui[3] = 0xFF; // Middle two bytes are 'FFFE'
    eui[4] = 0xFE;

    // Add the Modified EUI-64 identifier
    eui[0] |= 0x2;
}

bool Ipv6Service::serve(ServiceFeatures::Type type, void *pData, size_t dataLen)
{
    if(!pData)
        return false;

    // Correct type?
    if(g_pIpv6Features->provides(type))
    {
        // We only provide Touch services
        if(type & ServiceFeatures::touch)
        {
            Network *pCard = static_cast<Network*>(pData);

            StationInfo info = pCard->getStationInfo();

            // Create an IPv6-modified EUI-64 out of the MAC address
            uint8_t eui[8];
            Ipv6::getIpv6Eui64(info.mac, eui);

            // Fill in the prefix (link-local)
            uint8_t ipv6[16] = {0};
            ipv6[0] = 0xFE;
            ipv6[1] = 0x80;
            memcpy(ipv6 + 8, eui, 8);

            IpAddress *pNewAddress = 0;

            // Set the card's IPv6 address
            if(!info.nIpv6Addresses)
            {
                info.ipv6 = new IpAddress(ipv6);
                info.ipv6->setIpv6Prefix(64);
                info.nIpv6Addresses = 1;

                pNewAddress = info.ipv6;
            }
            else
            {
                // Need to add a new address.
                size_t currAddresses = info.nIpv6Addresses;
                IpAddress *pNew = new IpAddress[currAddresses + 1];
                for(size_t i = 0; i < currAddresses; i++)
                    pNew[i] = info.ipv6[i];

                pNew[currAddresses].setIp(ipv6);
                pNew[currAddresses].setIpv6Prefix(64);

                delete [] info.ipv6;
                info.ipv6 = pNew;

                info.nIpv6Addresses++;

                pNewAddress = &pNew[currAddresses];
            }

            pCard->setStationInfo(info);

            // Set up a route for this address.
            uint8_t localhost[16] = {0}; localhost[15] = 1;
            RoutingTable::instance().Add(RoutingTable::DestIpv6Sub, *pNewAddress, IpAddress(localhost), String(""), pCard);

            // Additionally, attempt to find any routers on the local network
            // in order to get a routable IPv6 address.
            Ndp::instance().routerSolicit(pCard);

            return true;
        }
    }

    // Not provided by us, fail!
    return false;
}

Ipv6::Ipv6()
{
}

Ipv6::~Ipv6()
{
}

uint16_t Ipv6::ipChecksum(IpAddress &from, IpAddress &to, uint8_t proto, uintptr_t data, uint16_t length)
{
  // Allocate space for the psuedo-header + packet data
  size_t tmpSize = length + sizeof(PsuedoHeader);
  uint8_t* tmpPack = new uint8_t[tmpSize];
  uintptr_t tmpPackAddr = reinterpret_cast<uintptr_t>(tmpPack);

  // Set up the psuedo-header
  PsuedoHeader *pHeader = reinterpret_cast<PsuedoHeader*>(tmpPackAddr);
  from.getIp(pHeader->src_addr);
  to.getIp(pHeader->dest_addr);
  pHeader->nextHeader = proto;
  pHeader->length = HOST_TO_BIG16(length);
  pHeader->zero1 = pHeader->zero2 = 0;

  // Throw in the packet data
  memcpy(reinterpret_cast<void*>(tmpPackAddr + sizeof(PsuedoHeader)),
         reinterpret_cast<void*>(data),
         length);

  // Perform the checksum
  uint16_t checksum = Network::calculateChecksum(tmpPackAddr, tmpSize);

  // Done.
  delete [] tmpPack;
  return checksum;
}

bool Ipv6::send(IpAddress dest, IpAddress from, uint8_t type, size_t nBytes, uintptr_t packet, Network *pCard)
{
    IpAddress realDest = dest;

    if((from.getType() == IpAddress::IPv4) || (dest.getType() == IpAddress::IPv4))
    {
        WARNING("IPv6: IPv4 addresses given to send");
        return false;
    }

    // Grab the address to send to (as well as the NIC to send with)
    // OVERRIDE any given pCard value!
    if(dest.isUnicast() && !dest.isLinkLocal())
    {
      pCard = RoutingTable::instance().DetermineRoute(&realDest);
      if(!pCard)
      {
        WARNING("IPv6: Couldn't find a route for destination '" << dest.toString() << "'.");
        return false;
      }
    }

    if(dest.isLinkLocal())
      pCard = RoutingTable::instance().DefaultRouteV6();

    if(!pCard->isConnected())
        return false; // NIC isn't active

    /// \todo Assumption: given "from" address is accurate.

    // Load the payload.
    memmove(reinterpret_cast<void*>(packet + sizeof(ip6Header)), reinterpret_cast<void*>(packet), nBytes);

    // Fill in the IPv6 header.
    ip6Header *pHeader = reinterpret_cast<ip6Header*>(packet);
    memset(pHeader, 0, sizeof(ip6Header));

    pHeader->verClassFlow = 6 << 4;
    pHeader->payloadLength = HOST_TO_BIG16(nBytes);
    pHeader->nextHeader = type;
    pHeader->hopLimit = 255; /// \todo Configurable hop limit.
    from.getIp(pHeader->sourceAddress);
    dest.getIp(pHeader->destAddress);

    // Get the address to send this packet to.
    MacAddress destMac;
    bool macValid = true;
    if(dest.isMulticast())
    {
        // Need individual octets of the IPv6 address.
        uint8_t ipv6[16];
        dest.getIp(ipv6);

        // Put together a link layer address for the address.
        /// \todo Ethernet-specific
        uint8_t tmp[6] = {0x33, 0x33, ipv6[12], ipv6[13], ipv6[14], ipv6[15]};
        destMac.setMac(tmp);

        macValid = true;
    }
    else
        macValid = Ndp::instance().neighbourSolicit(realDest, &destMac, pCard);

    if(macValid)
        Ethernet::send(nBytes + sizeof(ip6Header), packet, pCard, destMac, dest.getType());

    return macValid;
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

    // Grab the header
    ip6Header* header = reinterpret_cast<ip6Header*>(packetAddress);

    // Get the destination and source addresses
    IpAddress src(header->sourceAddress);
    IpAddress dest(header->destAddress);

    StationInfo me = pCard->getStationInfo();

    // Should we accept the packet?
    if(dest.isUnicast()) /// \todo We don't necessarily subscribe to ALL multicast groups - handle membership here.
    {
        // Match?
        bool bMatch = false;
        for(size_t i = 0; i < me.nIpv6Addresses; i++)
        {
            if(me.ipv6[i] == dest)
            {
                bMatch = true;
                break;
            }
        }

        // Not for us, ignore it.
        if(!bMatch)
        {
            return;
        }
    }

    size_t payloadSize = BIG_TO_HOST16(header->payloadLength);

    // Networking endpoint for the return path to the host sending this packet.
    Endpoint::RemoteEndpoint remoteHost;
    remoteHost.ip = src;

    // Header traversal.....
    uint8_t nextHeader = header->nextHeader;

    {
        switch(nextHeader)
        {
            case IP_TCP:
                // NOTICE("IPv6: TCP");
                Tcp::instance().receive(src, dest, packetAddress + sizeof(ip6Header), payloadSize, this, pCard);
                break;
            case IP_UDP:
                // NOTICE("IPv6: UDP");
                /// \todo Assumes no extension headers.
                Udp::instance().receive(src, dest, packetAddress + sizeof(ip6Header), payloadSize, this, pCard);
                break;
            case IP_ICMPV6:
                // NOTICE("IPv6: ICMPv6");
                Icmpv6::instance().receive(src, dest, packetAddress + sizeof(ip6Header), payloadSize, this, pCard);
                break;
        }
    }

    /// \todo Implement
}
