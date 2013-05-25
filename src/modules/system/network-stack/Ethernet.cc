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
#include "Ipv4.h"
#include "Ipv6.h"
#include "RawManager.h"
#include <Module.h>
#include <Log.h>


#include "Filter.h"

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
  if(!packet || !nBytes || !pCard)
      return;

  // Check for filtering
  if(!NetworkFilter::instance().filter(1, packet, nBytes))
  {
    pCard->droppedPacket();
    return; // Drop the packet.
  }

  // grab the header
  ethernetHeader* ethHeader = reinterpret_cast<ethernetHeader*>(packet + offset);

  // dump this packet into the RAW sockets
  RawManager::instance().receive(packet, nBytes, 0, -1, pCard);

  // what type is the packet?
  switch(BIG_TO_HOST16(ethHeader->type))
  {
    case ETH_ARP:
      // NOTICE("ARP packet!");

      Arp::instance().receive(nBytes, packet, pCard, sizeof(ethernetHeader));

      break;

    case ETH_RARP:
      NOTICE("RARP packet!");
      break;

    case ETH_IPV4:
      // NOTICE("IPv4 packet!");

      Ipv4::instance().receive(nBytes, packet, pCard, sizeof(ethernetHeader));

      break;

    case ETH_IPV6:
      // NOTICE("IPv6 packet!");

      Ipv6::instance().receive(nBytes, packet, pCard, sizeof(ethernetHeader));

      break;

    default:
      NOTICE("Unknown ethernet packet - type is " << BIG_TO_HOST16(ethHeader->type) << "!");
      pCard->badPacket();
      break;
  }
}

size_t Ethernet::injectHeader(uintptr_t packet, MacAddress destMac, MacAddress sourceMac, uint16_t type)
{
    // Basic checks for valid input
    if(!packet || !type)
        return 0;

    // Set up an Ethernet header
    ethernetHeader *pHeader = reinterpret_cast<ethernetHeader*>(packet);

    // Copy in the two MAC addresses
    memcpy(pHeader->destMac, destMac.getMac(), 6);
    memcpy(pHeader->sourceMac, sourceMac.getMac(), 6);

    // Set the packet type
    pHeader->type = HOST_TO_BIG16(type);

    // All done.
    return sizeof(ethernetHeader);
}

void Ethernet::getMacFromPacket(uintptr_t packet, MacAddress *mac)
{
  if(packet && mac)
  {
    // grab the header
    ethernetHeader* ethHeader = reinterpret_cast<ethernetHeader*>(packet);
    *mac = ethHeader->sourceMac;
  }
}

void Ethernet::send(size_t nBytes, uintptr_t packet, Network* pCard, MacAddress dest, uint16_t type)
{
  if(!pCard || !pCard->isConnected())
    return; // NIC isn't active

  // Move the payload for the ethernet header to go in
  memmove(reinterpret_cast<void*>(packet + sizeof(ethernetHeader)), reinterpret_cast<void*>(packet), nBytes);

  // get the ethernet header pointer
  ethernetHeader* ethHeader = reinterpret_cast<ethernetHeader*>(packet);

  // copy in the data
  StationInfo me = pCard->getStationInfo();
  memcpy(ethHeader->destMac, dest.getMac(), 6);
  memcpy(ethHeader->sourceMac, me.mac, 6);
  ethHeader->type = HOST_TO_BIG16(type);

  // send it over the network
  pCard->send(nBytes + sizeof(ethernetHeader), packet);

  // and dump it into any raw sockets (note the -1 for protocol - this means WIRE level endpoints)
  // RawManager::instance().receive(packAddr, newSize, 0, -1, pCard);
}
