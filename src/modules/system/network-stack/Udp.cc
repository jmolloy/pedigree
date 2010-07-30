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

uint16_t Udp::udpChecksum(uint32_t srcip, uint32_t destip, udpHeader* data)
{
  // psuedo-header on the front as well, build the packet, and checksum it
  size_t tmpSize = BIG_TO_HOST16(data->len) + sizeof(udpPsuedoHeaderIpv4);
  uint8_t* tmpPack = new uint8_t[tmpSize];
  uintptr_t tmpPackAddr = reinterpret_cast<uintptr_t>(tmpPack);

  udpPsuedoHeaderIpv4* psuedo = reinterpret_cast<udpPsuedoHeaderIpv4*>(tmpPackAddr);
  memcpy(reinterpret_cast<void*>(tmpPackAddr + sizeof(udpPsuedoHeaderIpv4)), data, BIG_TO_HOST16(data->len));

  psuedo->src_addr = srcip;
  psuedo->dest_addr = destip;
  psuedo->zero = 0;
  psuedo->proto = IP_UDP;
  psuedo->udplen = data->len;

  udpHeader* tmp = reinterpret_cast<udpHeader*>(tmpPackAddr + sizeof(udpPsuedoHeaderIpv4));
  tmp->checksum = 0;

  uint16_t checksum = Network::calculateChecksum(tmpPackAddr, tmpSize);

  delete [] tmpPack;

  return checksum;
}

bool Udp::send(IpAddress dest, uint16_t srcPort, uint16_t destPort, size_t nBytes, uintptr_t payload, bool broadcast, Network *pCard)
{
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

  // Lookup the MAC address of our target
  MacAddress destMac;
  bool macValid = true;
  if(dest == me.broadcast)
    destMac.setMac(0xff);
  else
    macValid = Arp::instance().getFromCache(tmp, true, &destMac, pCard);

  if(!macValid)
    return false;
  
  // Allocate a packet to send
  uintptr_t packet = NetworkStack::instance().getMemPool().allocate();

  // Grab the ethernet header
  size_t ethSize = Ethernet::instance().injectHeader(packet, destMac, me.mac, ETH_IPV4);
  if(!ethSize)
  {
    NetworkStack::instance().getMemPool().free(packet);
    return false;
  }

  // Grab the IPv4 header
  size_t ipSize = Ipv4::instance().injectHeader(packet + ethSize, dest, me.ipv4, IP_TCP);
  if(!ipSize)
  {
    NetworkStack::instance().getMemPool().free(packet);
    return false;
  }

  // Set up the UDP object
  udpHeader* header = reinterpret_cast<udpHeader*>(packet + ethSize + ipSize);
  memset(header, 0, sizeof(udpHeader));

  // Configure ports
  header->src_port = HOST_TO_BIG16(srcPort);
  header->dest_port = HOST_TO_BIG16(destPort);

  // Set the length of this packet
  header->checksum = 0;
  header->len = HOST_TO_BIG16(sizeof(udpHeader) + nBytes);

  // Copy in the payload
  if(nBytes)
    memcpy(reinterpret_cast<void*>(packet + ethSize + ipSize + sizeof(udpHeader)), reinterpret_cast<void*>(payload), nBytes);

  // Calculate the checksum
  header->checksum = Udp::instance().udpChecksum(me.ipv4.getIp(), dest.getIp(), header);

  // Perfom an IPv4 checksum over the packet
  Ipv4::instance().injectChecksumAndDataFields(packet + ethSize, nBytes + sizeof(udpHeader));

  // Transmit
  bool success = pCard->send(nBytes + sizeof(udpHeader) + ipSize + ethSize, packet);

  // Free the created packet
  NetworkStack::instance().getMemPool().free(packet);

  // All done.
  return success;
}

void Udp::receive(IpAddress from, size_t nBytes, uintptr_t packet, Network* pCard, uint32_t offset)
{
  if(!packet || !nBytes)
      return;

  // grab the IP header to find the size, so we can skip options and get to the UDP header
  Ipv4::ipHeader* ip = reinterpret_cast<Ipv4::ipHeader*>(packet + offset);
  size_t ipHeaderSize = (ip->header_len) * 4; // len is the number of DWORDs

  // Check for filtering
  /// \todo Add statistics to NICs
  if(!NetworkFilter::instance().filter(3, packet + offset + ipHeaderSize, nBytes - offset - ipHeaderSize))
  {
    pCard->droppedPacket();
    return;
  }

  // check if this packet is for us, or if it's a broadcast
  StationInfo cardInfo = pCard->getStationInfo();
  IpAddress to(ip->ipDest);
  /*if(cardInfo.ipv4.getIp() != ip->ipDest && ip->ipDest != 0xffffffff)
  {
    // not for us, TODO: check a flag to see if we'll accept these sorts of packets
    // as an example, DHCP will need this
    return;
  }*/

  // grab the header now
  udpHeader* header = reinterpret_cast<udpHeader*>(packet + offset + ipHeaderSize);

  // find the payload and its size - udpHeader::len is the size of the header + data
  // we use it rather than calculating the size from offsets in order to be able to handle
  // packets that may have been padded (for whatever reason)
  uintptr_t payload = reinterpret_cast<uintptr_t>(header) + sizeof(udpHeader);
  size_t payloadSize = BIG_TO_HOST16(header->len) - sizeof(udpHeader);

  // check the checksum, if it's not zero
  if(header->checksum != 0)
  {
    uint16_t checksum = header->checksum;
    header->checksum = 0;
    uint16_t calcChecksum = udpChecksum(ip->ipSrc, ip->ipDest, header);
    header->checksum = checksum;

    if(header->checksum != calcChecksum)
    {
      WARNING("UDP Checksum failed on incoming packet!");
      pCard->badPacket();
      return;
    }
  }

  // either no checksum, or calculation was successful, either way go on to handle it
  UdpManager::instance().receive(from, to, BIG_TO_HOST16(header->src_port), BIG_TO_HOST16(header->dest_port), payload, payloadSize, pCard);
}

