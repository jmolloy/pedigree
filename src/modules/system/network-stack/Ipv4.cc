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

#include "Ipv4.h"
#include <Module.h>
#include <Log.h>

// Protocols we use in IPv4
#include "Ethernet.h"
#include "Arp.h"

// Child protocols of IPv4
#include "Icmp.h"
#include "Udp.h"
#include "Tcp.h"

#include "NetManager.h"
#include "RawManager.h"
#include "Endpoint.h"

#include "RoutingTable.h"

#include "Filter.h"

Ipv4 Ipv4::ipInstance;

Ipv4::Ipv4() :
  m_NextIdLock(), m_IpId(0), m_Fragments()
{
}

Ipv4::~Ipv4()
{
}

bool Ipv4::send(IpAddress dest, IpAddress from, uint8_t type, size_t nBytes, uintptr_t packet, Network *pCard)
{
  IpAddress realDest = dest;

  // Grab the address to send to (as well as the NIC to send with)
  if(!pCard)
  {
    pCard = RoutingTable::instance().DetermineRoute(&realDest);
    if(!pCard)
    {
      WARNING("IPv4: Couldn't find a route for destination '" << dest.toString() << "'.");
      return false;
    }
  }
  if(!pCard->isConnected())
    return false; // NIC isn't active

  // Fill in the from address if it's not valid. Note that some protocols
  // can't do this as they need the source address in the checksum.
  StationInfo me = pCard->getStationInfo();
  if(from == Network::convertToIpv4(0, 0, 0, 0))
    from = me.ipv4;

  // Allocate space for the new packet with an IP header
  size_t newSize = nBytes + sizeof(ipHeader);
  uint8_t* newPacket = new uint8_t[newSize];
  uintptr_t packAddr = reinterpret_cast<uintptr_t>(newPacket);

  // Grab a pointer for the ip header
  ipHeader* header = reinterpret_cast<ipHeader*>(newPacket);
  memset(header, 0, sizeof(ipHeader));

  // Compose the IPv4 packet header
  header->id = Ipv4::instance().getNextId();

  header->ipDest = dest.getIp();
  header->ipSrc = from.getIp();

  header->len = HOST_TO_BIG16(sizeof(ipHeader) + nBytes);

  header->type = type;

  header->ttl = 128;

  header->ipver = 4; // IPv4, offset is 5 DWORDs
  header->header_len = 5;

  header->checksum = 0;
  header->checksum = Network::calculateChecksum(packAddr, sizeof(ipHeader));

  // copy the payload into the packet
  memcpy(reinterpret_cast<void*>(packAddr + sizeof(ipHeader)), reinterpret_cast<void*>(packet), nBytes);

  // Get the address to send to
  /// \todo Perhaps flag this so if we don't want to automatically resolve the MAC
  ///       it doesn't happen?
  MacAddress destMac;
  bool macValid = true;
  if(dest == me.broadcast)
    destMac.setMac(0xff);
  else
    macValid = Arp::instance().getFromCache(realDest, true, &destMac, pCard);
  if(macValid)
    Ethernet::send(newSize, packAddr, pCard, destMac, ETH_IPV4);

  delete [] newPacket;
  return macValid;
}

void Ipv4::receive(size_t nBytes, uintptr_t packet, Network* pCard, uint32_t offset)
{
  // Verify the inputs. Drivers may directly dump this information on us so
  // we cannot ever be too sure.
  if(!packet || !nBytes || !pCard)
      return;

  // Check for filtering
  /// \todo Add statistics to NICs
  if(!NetworkFilter::instance().filter(2, packet + offset, nBytes - offset))
  {
    pCard->droppedPacket();
    return;
  }

  // grab the header
  ipHeader* header = reinterpret_cast<ipHeader*>(packet + offset);

  // Store the addresses for the packet data
  uintptr_t packetAddress = packet + offset;
  size_t packetSize = nBytes - offset;
  bool wasFragment = false;

  // Verify the checksum
  uint16_t checksum = header->checksum;
  header->checksum = 0;
  uint16_t calcChecksum = Network::calculateChecksum(reinterpret_cast<uintptr_t>(header), sizeof(ipHeader));
  header->checksum = checksum;
  if(checksum == calcChecksum)
  {
    IpAddress from(header->ipSrc);
    Endpoint::RemoteEndpoint remoteHost;
    remoteHost.ip = from;

    uint16_t flags = (BIG_TO_HOST16(header->frag_offset) & 0xF000) >> 12;
    uint16_t frag_offset = (BIG_TO_HOST16(header->frag_offset) & ~0xF000) * 8;

    /// \note This implementation of IP fragmentation makes many assumptions,
    ///       the most important of which is that the fragments come in with
    ///       some sort of order. If the last fragment comes in too early, the
    ///       packet will not be reassembled properly.

    // Determine if the packet is part of a fragment
    if((frag_offset != 0) || (flags & IP_FLAG_MF))
    {
        WARNING("IPv4: An incoming packet was part of a fragment. If this");
        WARNING("message shows up repeatedly you may have a network link");
        WARNING("with an inappropriate MTU.");

        // Find the size of the data section of this packet
        size_t dataLength = BIG_TO_HOST16(header->len);
        dataLength -= header->header_len * 4;

        // Find the offset of the data section of this packet
        size_t dataOffset = offset;
        dataOffset += header->header_len * 4;
        
        // Grab the fragment block for this combination of IP and ID
        Ipv4Identifier id(BIG_TO_HOST16(header->id), from);
        fragmentWrapper *p = m_Fragments.lookup(id);

        // Do we need to create a new wrapper?
        if(!p)
        {
            p = new fragmentWrapper;
            m_Fragments.insert(id, p);

            size_t headerLen = header->header_len * 4;
            char *buff = new char[headerLen];
            memcpy(buff, header, headerLen);
            p->originalIpHeader = buff;
            p->originalIpHeaderLen = headerLen;
        }

        // Insert this fragment to the list
        if(dataLength)
        {
            char *buff = new char[dataLength];
            memcpy(buff, reinterpret_cast<void*>(packet + dataOffset), dataLength);

            fragment *f = new fragment;
            f->data = buff;
            f->length = dataLength;
            p->fragments.insert(frag_offset, f);
        }

        // Was this the last one?
        if(header->frag_offset && !(flags & IP_FLAG_MF))
        {
            // Yes, collate the fragments and send to the upper layers
            size_t copyOffset = p->originalIpHeaderLen;
            size_t fullLength = frag_offset + dataLength + copyOffset;
            char *buff = new char[fullLength];

            // Iterate through the fragment list, copying data into the buffer as we go
            for(Tree<size_t, fragment*>::Iterator it = p->fragments.begin();
                it!= p->fragments.end();
                it++)
            {
                // Copy this one in
                size_t fragment_offset = it.key();
                fragment *f = it.value();
                if(f)
                {
                    char *data = f->data;
                    if(data && fragment_offset < fullLength)
                    {
                        memcpy(buff + (copyOffset + fragment_offset), data, f->length);
                        delete data;
                    }
                    delete f;
                }
            }

            // Dump the header into the packet now
            /// \todo Verify the buffer and length
            memcpy(buff, p->originalIpHeader, p->originalIpHeaderLen);

            // Adjust the IP header a bit
            ipHeader *iphdr = reinterpret_cast<ipHeader*>(buff);
            iphdr->len = HOST_TO_BIG16(frag_offset + dataLength);

            // Clean up now that we're done
            delete p;
            m_Fragments.remove(id);

            // Fall through to the handling of a conventional packet
            packetAddress = reinterpret_cast<uintptr_t>(buff);
            packetSize = fullLength;
            wasFragment = true;
        }
        else
        {
            // Do not handle an incomplete packet, wait for another one to arrive
            return;
        }
    }
    else
    {
        wasFragment = false;
    }

    switch(header->type)
    {
      case IP_ICMP:
        // NOTICE("IP: ICMP packet");

        RawManager::instance().receive(packetAddress, nBytes - offset, &remoteHost, IPPROTO_ICMP, pCard);

        // icmp needs the ip header as well
        Icmp::instance().receive(from, nBytes, packetAddress, pCard, 0);
        break;

      case IP_UDP:
        // NOTICE("IP: UDP packet");

        RawManager::instance().receive(packetAddress, nBytes - offset, &remoteHost, IPPROTO_UDP, pCard);

        // udp needs the ip header as well
        Udp::instance().receive(from, nBytes, packetAddress, pCard, 0);
        break;

      case IP_TCP:
        // NOTICE("IP: TCP packet");

        RawManager::instance().receive(packetAddress, nBytes - offset, &remoteHost, IPPROTO_TCP, pCard);

        // tcp needs the ip header as well
        Tcp::instance().receive(from, nBytes, packetAddress, pCard, 0);
        break;

      default:
        NOTICE("IP: Unknown packet type");
        pCard->badPacket();
        break;
    }
    

    if(wasFragment)
    {
        delete reinterpret_cast<char*>(packetAddress);
    }
  }
  else
  {
    NOTICE("IP: Checksum invalid!");
    pCard->badPacket();
  }
}
