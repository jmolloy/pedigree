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

#include "Icmp.h"
#include <Module.h>
#include <Log.h>

#include "Ethernet.h"
#include "Ip.h"

Icmp Icmp::icmpInstance;

Icmp::Icmp()
{
}

Icmp::~Icmp()
{
}

void Icmp::send(stationInfo dest, uint8_t type, uint8_t code, uint16_t id, uint16_t seq, size_t nBytes, uintptr_t payload, Network* pCard)
{
  size_t newSize = nBytes + sizeof(icmpHeader);
  uint8_t* newPacket = new uint8_t[newSize];
  uintptr_t packAddr = reinterpret_cast<uintptr_t>(newPacket);
  
  icmpHeader* header = reinterpret_cast<icmpHeader*>(packAddr);
  header->type = type;
  header->code = code;
  header->id = HOST_TO_BIG16(id);
  header->seq = HOST_TO_BIG16(seq);
  
  if(nBytes)
    memcpy(reinterpret_cast<void*>(packAddr + sizeof(icmpHeader)), reinterpret_cast<void*>(payload), nBytes);
  
  header->checksum = 0;
  header->checksum = Network::calculateChecksum(packAddr, newSize);
  
  Ip::send(dest, IP_ICMP, newSize, packAddr, pCard);
  
  delete newPacket;
}

void Icmp::receive(stationInfo from, size_t nBytes, uintptr_t packet, Network* pCard, uint32_t offset)
{  
  // grab the header
  icmpHeader* header = reinterpret_cast<icmpHeader*>(packet + offset + sizeof(Ip::ipHeader));
  
  // check the checksum
  uint16_t checksum = header->checksum;
  header->checksum = 0;
  uint16_t calcChecksum = Network::calculateChecksum(reinterpret_cast<uintptr_t>(header), nBytes - offset - sizeof(Ip::ipHeader));
  header->checksum = checksum;
  if(checksum == calcChecksum)
  {
    // what's come in?
    switch(header->type)
    {
      case ICMP_ECHO_REQUEST:
        {
      
        //NOTICE("ICMP: Echo request");
        
        // send the reply
        send(
          from,
          ICMP_ECHO_REPLY,
          header->code,
          BIG_TO_HOST16(header->id),
          BIG_TO_HOST16(header->seq),
          // *ugh* - these two lines are awful!
          nBytes - offset - sizeof(Ip::ipHeader) - sizeof(icmpHeader),
          packet + offset + sizeof(Ip::ipHeader) + sizeof(icmpHeader),
          pCard
        );

        }
        break;
      
      case ICMP_ECHO_REPLY:
      
        // at some point we'll handle sending pings, when we do
        // the break below will be uncommented
        // i'll do this once i learn about some form of timing mechanism
        
        // break;
      
      default:
      
        NOTICE("ICMP: Unhandled packet");
        break;
    }
  }
}
