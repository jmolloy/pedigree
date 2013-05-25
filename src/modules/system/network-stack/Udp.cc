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

#include "Udp.h"
#include <Module.h>
#include <Log.h>

#include "Ethernet.h"
#include "Ipv4.h"
#include "Ipv6.h"

#include "UdpManager.h"

#include "RoutingTable.h"

#include "Filter.h"

#include "Arp.h"
#include "IpCommon.h"

Udp Udp::udpInstance;

Udp::Udp()
{
}

Udp::~Udp()
{
}

bool Udp::send(IpAddress dest, uint16_t srcPort, uint16_t destPort, size_t nBytes, uintptr_t payload, bool broadcast, Network *pCard)
{
  // IP base for all operations here.
  /// \todo Abstract this out so that we don't have to have this specific code in the protocol.
  IpBase *pIp = &Ipv4::instance();
  if(dest.getType() == IpAddress::IPv6)
    pIp = &Ipv6::instance();

  // Grab the NIC to send on, if we don't already have one.
  /// \note The NIC is grabbed here *as well as* IP because we need to use the
  ///       NIC IP address for the checksum.
  IpAddress tmp = dest;
  if(!pCard)
  {
      if(broadcast)
      {
          WARNING("UDP: No NIC given to send(), and attempting to send a broadcast packet!");
          return false;
      }

      if(!RoutingTable::instance().hasRoutes())
      {
          WARNING("UDP: No NIC given to send(), and no routes available in the routing table");
          return false;
      }

      pCard = RoutingTable::instance().DetermineRoute(&tmp);
      if(!pCard)
      {
          WARNING("UDP: Couldn't find a route for destination '" << dest.toString() << "'.");
          return false;
      }
  }

  // Grab information about ourselves
  StationInfo me = pCard->getStationInfo();
  if(broadcast) /// \note pCard MUST be set for broadcast packets!
    dest = me.broadcast;

  // Source address determination.
  IpAddress src = me.ipv4;
  if(dest.getType() == IpAddress::IPv6)
  {
    // Handle IPv6 source address determination.
    if(!me.nIpv6Addresses)
    {
      WARNING("TCP: can't send to an IPv6 host without an IPv6 address.");
      return false;
    }

    /// \todo Distinguish IPv6 addresses, and prefixes, and provide a way of
    ///       choosing the right one.
    /// \bug Assumes any non-link-local address is fair game.
    size_t i;
    for(i = 0; i < me.nIpv6Addresses; i++)
    {
        if(!me.ipv6[i].isLinkLocal())
        {
            src = me.ipv6[i];
            break;
        }
    }
    if(i == me.nIpv6Addresses)
    {
        WARNING("No IPv6 address available for TCP");
        return false;
    }
  }

  // Allocate a packet to send
  uintptr_t packet = NetworkStack::instance().getMemPool().allocate();

  // Add the UDP header to the packet.
  udpHeader* header = reinterpret_cast<udpHeader*>(packet);
  memset(header, 0, sizeof(udpHeader));
  header->src_port = HOST_TO_BIG16(srcPort);
  header->dest_port = HOST_TO_BIG16(destPort);
  header->len = HOST_TO_BIG16(sizeof(udpHeader) + nBytes);
  header->checksum = 0;

  // Copy in the payload
  if(nBytes)
    memcpy(reinterpret_cast<void*>(packet + sizeof(udpHeader)), reinterpret_cast<void*>(payload), nBytes);

  // Calculate the checksum
  header->checksum = pIp->ipChecksum(src, dest, IP_UDP, reinterpret_cast<uintptr_t>(header), sizeof(udpHeader) + nBytes);

  // Transmit
  bool success = pIp->send(dest, src, IP_UDP, nBytes + sizeof(udpHeader), packet, pCard);

  // Free the created packet
  NetworkStack::instance().getMemPool().free(packet);

  // All done.
  return success;
}

void Udp::receive(IpAddress from, IpAddress to, uintptr_t packet, size_t nBytes, IpBase *pIp, Network* pCard)
{
    if(!packet || !nBytes)
        return;

    // Check for filtering
    if(!NetworkFilter::instance().filter(3, packet, nBytes))
    {
        pCard->droppedPacket();
        return;
    }

    // check if this packet is for us, or if it's a broadcast
    StationInfo cardInfo = pCard->getStationInfo();
    /*if(cardInfo.ipv4.getIp() != ip->ipDest && ip->ipDest != 0xffffffff)
    {
        // not for us, TODO: check a flag to see if we'll accept these sorts of packets
        // as an example, DHCP will need this
        return;
    }*/

    // Grab the header now
    udpHeader* header = reinterpret_cast<udpHeader*>(packet);

    // Find the payload and its size - udpHeader::len is the size of the header + data
    // we use it rather than calculating the size from offsets in order to be able to handle
    // packets that may have been padded (for whatever reason)
    uintptr_t payload = reinterpret_cast<uintptr_t>(header) + sizeof(udpHeader);
    size_t payloadSize = BIG_TO_HOST16(header->len) - sizeof(udpHeader);

    // Check the checksum, if it's not zero
    if(header->checksum != 0)
    {
        uint16_t checksum = pIp->ipChecksum(from, to, IP_UDP, reinterpret_cast<uintptr_t>(header), BIG_TO_HOST16(header->len));
        if(checksum)
        {
            WARNING("UDP Checksum failed on incoming packet [" << header->checksum << ", and " << checksum << " should be zero]!");
            pCard->badPacket();
            return;
        }
    }

    // Either no checksum, or calculation was successful, either way go on to handle it
    UdpManager::instance().receive(from, to, BIG_TO_HOST16(header->src_port), BIG_TO_HOST16(header->dest_port), payload, payloadSize, pCard);
}

