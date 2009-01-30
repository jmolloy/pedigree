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

#include "Ip.h"
#include <Module.h>
#include <Log.h>

// protocols we use in IP
#include "Ethernet.h"
#include "Arp.h"

// child protocols of IP
#include "Icmp.h"

Ip Ip::ipInstance;

Ip::Ip()
{
}

Ip::~Ip()
{
}

void Ip::send(stationInfo dest, uint8_t type, size_t nBytes, uintptr_t packet, Network* pCard)
{
  // allocate space for the new packet with an IP header
  size_t newSize = nBytes + sizeof(ipHeader);
  uint8_t* newPacket = new uint8_t[newSize];
  uintptr_t packAddr = reinterpret_cast<uintptr_t>(newPacket);
  
  // grab a pointer for the ip header
  ipHeader* header = reinterpret_cast<ipHeader*>(newPacket);
  memset(header, 0, sizeof(ipHeader));
  
  // do the deed
  stationInfo me = pCard->getStationInfo();
  
  header->ipDest = dest.ipv4; /// \todo IPv6
  header->ipSrc = me.ipv4;
  
  header->len = HOST_TO_BIG16(sizeof(ipHeader) + nBytes);
  
  header->type = type;
  
  header->ttl = 64; /// \note Perhaps this could be customisable one day?
  
  header->verlen = (5 << 0) | (4 << 4); // IPv4, offset is 5 DWORDs
  
  header->checksum = 0;
  header->checksum = Network::calculateChecksum(packAddr, sizeof(ipHeader));
  
  // copy the payload into the packet
  memcpy(reinterpret_cast<void*>(packAddr + sizeof(ipHeader)), reinterpret_cast<void*>(packet), nBytes);
  
  // get the address to send to
  /// \todo Perhaps flag this so if we don't want to automatically resolve the MAC
  ///       it doesn't happen?
  MacAddress destMac;
  bool macValid = Arp::instance().getFromCache(dest.ipv4, true, &destMac, pCard);
  if(macValid)
    Ethernet::send(newSize, packAddr, pCard, destMac, ETH_IP);
  
  delete newPacket;
}


void Ip::receive(size_t nBytes, uintptr_t packet, Network* pCard, uint32_t offset)
{  
  // grab the header
  ipHeader* header = reinterpret_cast<ipHeader*>(packet + offset);
  
  stationInfo cardInfo = pCard->getStationInfo();
  if(cardInfo.ipv4 == header->ipDest)
  {
    /// \todo Handle fragmentation!
    
    // check the checksum :D
    uint16_t checksum = header->checksum;
    header->checksum = 0;
    uint16_t calcChecksum = Network::calculateChecksum(reinterpret_cast<uintptr_t>(header), sizeof(ipHeader));
    header->checksum = checksum;
    if(checksum == calcChecksum)
    {
      // build a stationInfo structure holding information about the system the packet came
      // from
      stationInfo sourceStation;
      sourceStation.ipv4 = header->ipSrc;
      
      switch(header->type)
      {
        case IP_ICMP:
          //NOTICE("IP: ICMP packet");
          
          // icmp needs the ip header as well
          Icmp::instance().receive(sourceStation, nBytes, packet, pCard, offset);
          break;
          
        case IP_UDP:
          NOTICE("IP: UDP packet");
          
          // udp needs the ip header as well
          // Udp::instance().receive(sourceStation, nBytes, packet, pCard, offset);
          break;
          
        case IP_TCP:
          NOTICE("IP: TCP packet");
          
          // tc needs the ip header as well
          // Tcp::instance().receive(sourceStation, nBytes, packet, pCard, offset);
          break;
        
        default:
          NOTICE("IP: Unknown packet type");
          break;
      }
    }
  }
}
