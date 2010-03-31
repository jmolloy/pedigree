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

void Icmp::send(IpAddress dest, uint8_t type, uint8_t code, uint16_t id, uint16_t seq, size_t nBytes, uintptr_t payload)
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

  Ipv4::send(dest, Network::convertToIpv4(0, 0, 0, 0), IP_ICMP, newSize, packAddr);

  delete [] newPacket;
}

void Icmp::receive(IpAddress from, size_t nBytes, uintptr_t packet, Network* pCard, uint32_t offset)
{
  if(!packet || !nBytes)
      return;

  // grab the IP header to find the size, so we can skip options and get to the TCP header
  Ipv4::ipHeader* ip = reinterpret_cast<Ipv4::ipHeader*>(packet + offset);
  size_t ipHeaderSize = (ip->header_len) * 4; // len is the number of DWORDs
  size_t payloadSize = BIG_TO_HOST16(ip->len) - ipHeaderSize;
  
  // grab the header
  icmpHeader* header = reinterpret_cast<icmpHeader*>(packet + offset + ipHeaderSize);

#ifdef DEBUG_ICMP  
  NOTICE("ICMP type=" << header->type << ", code=" << header->code << ", checksum=" << header->checksum);
  NOTICE("ICMP id=" << header->id << ", seq=" << header->seq);
#endif

  // check the checksum
  uint16_t checksum = header->checksum;
  header->checksum = 0;
  uint16_t calcChecksum = Network::calculateChecksum(reinterpret_cast<uintptr_t>(header), payloadSize); //nBytes - offset - sizeof(Ipv4::ipHeader));
  header->checksum = checksum;

#ifdef DEBUG_ICMP
  NOTICE("ICMP calculated checksum is " << calcChecksum << ", packet checksum = " << checksum);
#endif
  
  if(checksum == calcChecksum)
  {
    // what's come in?
    switch(header->type)
    {
      case ICMP_ECHO_REQUEST:
        {

#ifdef DEBUG_ICMP
        NOTICE("ICMP: Echo request");
#endif

        // send the reply
        send(
          from,
          ICMP_ECHO_REPLY,
          header->code,
          BIG_TO_HOST16(header->id),
          BIG_TO_HOST16(header->seq),
          payloadSize - sizeof(icmpHeader),
          packet + offset + ipHeaderSize + sizeof(icmpHeader)
        );

        }
        break;

      default:

        // Now that things can be moved out to user applications thanks to SOCK_RAW,
        // the kernel doesn't need to implement too much of the ICMP suite.

        NOTICE("ICMP: Unhandled packet - type is " << header->type << ".");
        break;
    }
  }
}
