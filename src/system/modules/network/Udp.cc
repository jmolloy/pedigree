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
#include "Ip.h"

#include "UdpManager.h"

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
  
  delete tmpPack;
  
  return checksum;

  /*

	uint32_t sum = 0;
  data->checksum = 0;
	uint16_t* buffer = reinterpret_cast<uint16_t*>(data);
  size_t nBytes = BIG_TO_HOST16(data->len);
  //if((nBytes % 2) != 0)
  //  nBytes++;
  NOTICE("data->len is " << BIG_TO_HOST16(data->len) << ", should be " << 13 + 8 << ".");
  NOTICE("IPs are " << srcip << " and " << destip << "...");
  
  size_t i, j;
  for(i = 0, j = 0; i < nBytes; i += 2, j++)
    sum += buffer[j];
  if(nBytes & 0x1)
    sum += buffer[nBytes-1];
  
  // add in the psuedo header
  sum += srcip;
  sum += destip;
  sum += IP_UDP << 8;
  sum += data->len;

	// fold to 16 bits
	while( sum >> 16 )
		sum = ( sum & 0xFFFF ) + ( sum >> 16 );

  uint16_t ret = static_cast<uint16_t>(~sum);
  /// \todo This simply can't be right - figure out what on earth is going on!
  //ret--;
	return ret;*/
}

void Udp::send(IpAddress dest, uint16_t srcPort, uint16_t destPort, size_t nBytes, uintptr_t payload, Network* pCard)
{
  size_t newSize = nBytes + sizeof(udpHeader);
  uint8_t* newPacket = new uint8_t[newSize];
  uintptr_t packAddr = reinterpret_cast<uintptr_t>(newPacket);
  
  udpHeader* header = reinterpret_cast<udpHeader*>(packAddr);
  header->src_port = HOST_TO_BIG16(srcPort);
  header->dest_port = HOST_TO_BIG16(destPort);
  header->checksum = 0;
  header->len = HOST_TO_BIG16(sizeof(udpHeader) + nBytes);
  
  StationInfo me = pCard->getStationInfo();
  
  if(nBytes)
    memcpy(reinterpret_cast<void*>(packAddr + sizeof(udpHeader)), reinterpret_cast<void*>(payload), nBytes);
  
  header->checksum = Udp::instance().udpChecksum(me.ipv4.getIp(), dest.getIp(), header);
  
  Ip::send(dest, IP_UDP, newSize, packAddr, pCard);
  
  delete newPacket;
}

void Udp::receive(IpAddress from, size_t nBytes, uintptr_t packet, Network* pCard, uint32_t offset)
{  
  // grab the IP header to find the size, so we can skip options and get to the UDP header
  Ip::ipHeader* ip = reinterpret_cast<Ip::ipHeader*>(packet + offset);
  size_t ipHeaderSize = (ip->verlen & 0x0F) * 4; // len is the number of DWORDs
  
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
      return;
    }
  }
  
  // either no checksum, or calculation was successful, either way go on to handle it
  UdpManager::instance().receive(from, BIG_TO_HOST16(header->src_port), BIG_TO_HOST16(header->dest_port), payload, payloadSize);
}
