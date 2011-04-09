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

#include "Tcp.h"
#include <Module.h>
#include <Log.h>

#include "Ethernet.h"
#include "Ipv4.h"
#include "Ipv6.h"

#include "TcpManager.h"

#include "RoutingTable.h"

#include "Filter.h"

#include "Arp.h"
#include "IpCommon.h"

Tcp Tcp::tcpInstance;

Tcp::Tcp()
{
}

Tcp::~Tcp()
{
}

bool Tcp::send(IpAddress dest, uint16_t srcPort, uint16_t destPort, uint32_t seqNumber, uint32_t ackNumber, uint8_t flags, uint16_t window, size_t nBytes, uintptr_t payload)
{
  // IP base for all operations here.
  IpBase *pIp = &Ipv4::instance();
  if(dest.getType() == IpAddress::IPv6)
    pIp = &Ipv6::instance();

  // Grab the NIC to send on.
  /// \note The NIC is grabbed here *as well as* IP because we need to use the
  ///       NIC IP address for the checksum.
  IpAddress tmp = dest;
  Network *pCard = RoutingTable::instance().DetermineRoute(&tmp);
  if(!pCard)
  {
    WARNING("TCP: Couldn't find a route for destination '" << dest.toString() << "'.");
    return false;
  }

  // Grab information about ourselves
  StationInfo me = pCard->getStationInfo();

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

  // Create TCP header
  uintptr_t tcpPacket = packet;
  tcpHeader* header = reinterpret_cast<tcpHeader*>(tcpPacket);
  header->src_port = HOST_TO_BIG16(srcPort);
  header->dest_port = HOST_TO_BIG16(destPort);
  header->seqnum = HOST_TO_BIG32(seqNumber);
  header->acknum = HOST_TO_BIG32(ackNumber);
  header->offset = sizeof(tcpHeader) / 4;

  header->rsvd = 0;
  header->flags = flags;
  header->winsize = HOST_TO_BIG16(window);
  header->urgptr = 0;

  // Inject the payload
  if(payload && nBytes)
    memcpy(reinterpret_cast<void*>(tcpPacket + sizeof(tcpHeader)), reinterpret_cast<void*>(payload), nBytes);

  header->checksum = 0;
  header->checksum = pIp->ipChecksum(src, dest, IP_TCP, tcpPacket, nBytes + sizeof(tcpHeader));

  // Transmit
  bool success = pIp->send(dest, src, IP_TCP, nBytes + sizeof(tcpHeader), packet, pCard);

  // Free the created packet
  NetworkStack::instance().getMemPool().free(packet);

  // All done.
  return success;
}

void Tcp::receive(IpAddress from, IpAddress to, uintptr_t packet, size_t nBytes, IpBase *pIp, Network* pCard)
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
  /// \todo IPv6
  StationInfo cardInfo = pCard->getStationInfo();
  if(from.getType() == IpAddress::IPv4 &&
     cardInfo.ipv4 != to &&
     to.getIp() != 0xffffffff &&
     cardInfo.broadcast != to)
  {
    // not for us (depending on future requirements, we may need to implement a "catch-all"
    // flag in the same way as UDP)
    return;
  }

  // grab the header now
  tcpHeader* header = reinterpret_cast<tcpHeader*>(packet);

  // the size of the header (+ options)
  size_t headerSize = header->offset * 4;

  // find the payload and its size - tcpHeader::len is the size of the header + data
  // we use it rather than calculating the size from offsets in order to be able to handle
  // packets that may have been padded (for whatever reason)
  uintptr_t payload = reinterpret_cast<uintptr_t>(header) + headerSize; // offset is in DWORDs
  size_t payloadSize = nBytes - headerSize;

  // check the checksum, if it's not zero
  if(header->checksum != 0)
  {
    uint16_t checksum = pIp->ipChecksum(from, to, IP_TCP, reinterpret_cast<uintptr_t>(header), nBytes);
    if(checksum)
    {
      WARNING("TCP Checksum failed on incoming packet [dp=" << Dec << BIG_TO_HOST16(header->dest_port) << Hex << "]. Header checksum is " << header->checksum << ", calculated should be zero but is " << checksum << "!");
      pCard->badPacket();
      return;
    }
  }
  else
  {
      WARNING("TCP Packet arrived on port " << Dec << BIG_TO_HOST16(header->dest_port) << Hex << " without a checksum.");
      pCard->badPacket();
      return; // must have a checksum
  }

  // NOTICE("TCP: Packet has arrived! Dest port is " << Dec << BIG_TO_HOST16(header->dest_port) << Hex << " and flags are " << header->flags << ".");

  // either no checksum, or calculation was successful, either way go on to handle it
  TcpManager::instance().receive(from, BIG_TO_HOST16(header->src_port), BIG_TO_HOST16(header->dest_port), header, payload, payloadSize, pCard);
}
