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

#include "Ethernet.h"
#include "Arp.h"
#include "Ip.h"
#include "RawManager.h"
#include <Module.h>
#include <Log.h>

Ethernet Ethernet::ethernetInstance;

Ethernet::Ethernet()
{
  //
}

Ethernet::~Ethernet()
{
  //
}

void Ethernet::receive(size_t nBytes, uintptr_t packet, Network* pCard, uint32_t offset)
{  
  // grab the header
  ethernetHeader* ethHeader = reinterpret_cast<ethernetHeader*>(packet + offset);

  // dump this packet into the RAW sockets
  NOTICE("Passing on to RawManager");
  RawManager::instance().receive(packet, nBytes, pCard);
  
  // what type is the packet?
  switch(BIG_TO_HOST16(ethHeader->type))
  {
    case ETH_ARP:
      //NOTICE("ARP packet!");
      
      Arp::instance().receive(nBytes, packet, pCard, sizeof(ethernetHeader));
      
      break;
      
    case ETH_RARP:
      NOTICE("RARP packet!");
      break;
      
    case ETH_IP:
      //NOTICE("IP packet!");
      
      Ip::instance().receive(nBytes, packet, pCard, sizeof(ethernetHeader));
      
      break;
      
    default:
      NOTICE("Unknown ethernet packet!");
      break;
  }
}

void Ethernet::send(size_t nBytes, uintptr_t packet, Network* pCard, MacAddress dest, uint16_t type)
{
  // allocate space for the new packet with an ethernet header
  size_t newSize = nBytes + sizeof(ethernetHeader);
  uint8_t* newPacket = new uint8_t[newSize];
  uintptr_t packAddr = reinterpret_cast<uintptr_t>(newPacket);
  
  // get the ethernet header pointer
  ethernetHeader* ethHeader = reinterpret_cast<ethernetHeader*>(newPacket);
  
  // copy in the data
  StationInfo me = pCard->getStationInfo();
  memcpy(ethHeader->destMac, dest.getMac(), 6);
  memcpy(ethHeader->sourceMac, me.mac, 6);
  ethHeader->type = HOST_TO_BIG16(type);
  
  // and then throw in the payload
  memcpy(reinterpret_cast<void*>(packAddr + sizeof(ethernetHeader)), reinterpret_cast<void*>(packet), nBytes);
  
  // send it over the network
  pCard->send(newSize, reinterpret_cast<uintptr_t>(newPacket));
  
  delete newPacket;
}
