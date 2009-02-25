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
#include "Ip.h"

#include "TcpManager.h"

Tcp Tcp::tcpInstance;

Tcp::Tcp()
{
}

Tcp::~Tcp()
{
}

uint16_t Tcp::tcpChecksum(uint32_t srcip, uint32_t destip, tcpHeader* data, uint16_t len)
{
  // psuedo-header on the front as well, build the packet, and checksum it
  size_t tmpSize = len + sizeof(tcpPsuedoHeaderIpv4);
  uint8_t* tmpPack = new uint8_t[tmpSize];
  uintptr_t tmpPackAddr = reinterpret_cast<uintptr_t>(tmpPack);
  
  tcpPsuedoHeaderIpv4* psuedo = reinterpret_cast<tcpPsuedoHeaderIpv4*>(tmpPackAddr);
  memcpy(reinterpret_cast<void*>(tmpPackAddr + sizeof(tcpPsuedoHeaderIpv4)), data, len);
  
  psuedo->src_addr = srcip;
  psuedo->dest_addr = destip;
  psuedo->zero = 0;
  psuedo->proto = IP_TCP;
  psuedo->tcplen = HOST_TO_BIG16(len);
  
  tcpHeader* tmp = reinterpret_cast<tcpHeader*>(tmpPackAddr + sizeof(tcpPsuedoHeaderIpv4));
  tmp->checksum = 0;
  
  uint16_t checksum = Network::calculateChecksum(tmpPackAddr, tmpSize);
  
  delete tmpPack;
  
  return checksum;
}

bool Tcp::send(IpAddress dest, uint16_t srcPort, uint16_t destPort, uint32_t seqNumber, uint32_t ackNumber, uint8_t flags, uint16_t window, size_t nBytes, uintptr_t payload, Network* pCard)
{
  size_t newSize = nBytes + sizeof(tcpHeader);
  uint8_t* newPacket = new uint8_t[newSize];
  uintptr_t packAddr = reinterpret_cast<uintptr_t>(newPacket);
  
  tcpHeader* header = reinterpret_cast<tcpHeader*>(packAddr);
  header->src_port = HOST_TO_BIG16(srcPort);
  header->dest_port = HOST_TO_BIG16(destPort);
  header->seqnum = HOST_TO_BIG32(seqNumber);
  header->acknum = HOST_TO_BIG32(ackNumber);
  header->offset = sizeof(tcpHeader) / 4;
  
  header->rsvd = 0;
  header->flags = flags;
  header->winsize = HOST_TO_BIG16(window);
  header->urgptr = 0;
  
  header->checksum = 0;
  
  StationInfo me = pCard->getStationInfo();
  
  if(nBytes)
    memcpy(reinterpret_cast<void*>(packAddr + sizeof(tcpHeader)), reinterpret_cast<void*>(payload), nBytes);
  
  header->checksum = Tcp::instance().tcpChecksum(me.ipv4.getIp(), dest.getIp(), header, nBytes + sizeof(tcpHeader));
  
  bool success = Ip::send(dest, me.ipv4, IP_TCP, newSize, packAddr, pCard);
  
  delete newPacket;
  return success;
}

void Tcp::receive(IpAddress from, size_t nBytes, uintptr_t packet, Network* pCard, uint32_t offset)
{  
  // grab the IP header to find the size, so we can skip options and get to the TCP header
  Ip::ipHeader* ip = reinterpret_cast<Ip::ipHeader*>(packet + offset);
  size_t ipHeaderSize = (ip->verlen & 0x0F) * 4; // len is the number of DWORDs
  
  // check if this packet is for us, or if it's a broadcast
  StationInfo cardInfo = pCard->getStationInfo();
  if(cardInfo.ipv4.getIp() != ip->ipDest && ip->ipDest != 0xffffffff)
  {
    // not for us (depending on future requirements, we may need to implement a "catch-all"
    // flag in the same way as UDP)
    return;
  }
  
  // find the size of the TCP header + data
  size_t tcpPayloadSize = BIG_TO_HOST16(ip->len) - ipHeaderSize;
  
  // grab the header now
  tcpHeader* header = reinterpret_cast<tcpHeader*>(packet + offset + ipHeaderSize);
  
  // the size of the header (+ options)
  size_t headerSize = header->offset * 4;
  
  // find the payload and its size - tcpHeader::len is the size of the header + data
  // we use it rather than calculating the size from offsets in order to be able to handle
  // packets that may have been padded (for whatever reason)
  uintptr_t payload = reinterpret_cast<uintptr_t>(header) + (header->offset * 4); // offset is in DWORDs
  size_t payloadSize = tcpPayloadSize - headerSize; //  (packet + nBytes) - payload;
  
  // check the checksum, if it's not zero
  if(header->checksum != 0)
  {
    uint16_t checksum = header->checksum;
    header->checksum = 0;
    uint16_t calcChecksum = tcpChecksum(ip->ipSrc, ip->ipDest, header, tcpPayloadSize);
    header->checksum = checksum;
    
    if(header->checksum != calcChecksum)
    {
      WARNING("TCP Checksum failed on incoming packet. Header checksum is " << header->checksum << " and calculated is " << calcChecksum << "!");
      return;
    }
  }
  else
    return; // must have a checksum
  
  // NOTICE("TCP: Packet has arrived! Dest port is " << Dec << BIG_TO_HOST16(header->dest_port) << Hex << " and flags are " << header->flags << ".");
  
  // either no checksum, or calculation was successful, either way go on to handle it
  TcpManager::instance().receive(from, BIG_TO_HOST16(header->src_port), BIG_TO_HOST16(header->dest_port), header, payload, payloadSize, pCard);
}
