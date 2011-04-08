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

Ipv6::Ipv6() :
  m_NextIdLock(), m_IpId(0)
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
#if 0
    if(!pCard)
    {
        pCard = RoutingTable::instance().DetermineRoute(&realDest);
        if(!pCard)
        {
            WARNING("IPv4: Couldn't find a route for destination '" << dest.toString() << "'.");
            return false;
        }
    }
#else
#warning IPv6 routing is not yet supported.
#endif
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
    realDest.getIp(pHeader->destAddress);

    // Get the address to send this packet to.
    MacAddress destMac;
    bool macValid = true;
    if(dest.isMulticast())
    {
        /// \todo Multicast.
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

    // Cheat a bit with NDP - glean information from the packet.
    if(src.isUnicast())
    {
        MacAddress e;
        Ethernet::instance().getMacFromPacket(packet, &e);

        NOTICE("IPv6: poisoning cache for " << src.toString() << ": " << e.toString());

        Ndp::instance().addEntry(src, e);
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
                Tcp::instance().receive(src, dest, packetAddress + sizeof(ip6Header), payloadSize, this, pCard);
                break;
            case IP_UDP:
                /// \todo Assumes no extension headers.
                Udp::instance().receive(src, dest, packetAddress + sizeof(ip6Header), payloadSize, this, pCard);
                break;
            case IP_ICMPV6:
                Icmpv6::instance().receive(src, dest, packetAddress + sizeof(ip6Header), payloadSize, this, pCard);
                break;
        }
    }

    /// \todo Implement
}
