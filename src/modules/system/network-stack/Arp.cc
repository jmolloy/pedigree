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

#include "Arp.h"
#include "Ethernet.h"
#include <Module.h>
#include <Log.h>
#include <processor/Processor.h>

#include "Filter.h"

Arp Arp::arpInstance;

Arp::Arp() :
  RequestQueue(), m_ArpCache(), m_ArpRequests()
{
    // By now threading should be active, as we're a module.
    initialise();
}

Arp::~Arp()
{
    destroy();
}

bool Arp::getFromCache(IpAddress ip, bool resolve, MacAddress* ent, Network* pCard)
{
  // ensure the IP given is valid
  if(ip.getType() == IpAddress::IPv6)
    return false; // ARP isn't for IPv6
  if(!pCard->isConnected())
    return false; // NIC isn't active

  // do we have an entry for it yet?
  arpEntry* arpEnt;
  if((arpEnt = m_ArpCache.lookup(static_cast<uintptr_t>(ip.getIp()))) != 0)
  {
    *ent = arpEnt->mac;
    return true;
  }

  // not found, do we resolve?
  if(!resolve)
    return false;
  ArpRequest* req = new ArpRequest;
  req->destIp = ip;

  // push this onto the list
  m_ArpRequests.pushBack(req);

  bool success = addRequest(0, reinterpret_cast<uint64_t>(req), reinterpret_cast<uint64_t>(pCard));

  // if sucessful, load into the passed MacAddress
  // callers should check the return value to ensure this is valid
  if(success)
    *ent = req->mac;

  // clean up
  delete req;
  return success;
}

void Arp::send(IpAddress req, Network* pCard)
{
  StationInfo cardInfo = pCard->getStationInfo();
  if(cardInfo.ipv4.getIp() == 0)
    return; // Give up on trying to send an ARP request with no IP

  // Allocate a packet to send
  uintptr_t packet = NetworkStack::instance().getMemPool().allocate();

  arpHeader* request = reinterpret_cast<arpHeader*>(packet);

  request->hwType = HOST_TO_BIG16(0x0001); // ethernet
  request->hwSize = 6;

  request->protocol = HOST_TO_BIG16(ETH_IPV4);
  request->protocolSize = 4;

  request->opcode = HOST_TO_BIG16(ARP_OP_REQUEST);

  request->ipSrc = cardInfo.ipv4.getIp();
  request->ipDest = req.getIp();

  NOTICE("arp who-has " << req.toString() << " tell " << cardInfo.ipv4.toString());

  memcpy(request->hwSrc, cardInfo.mac, 6);
  memset(request->hwDest, 0xff, 6); // broadcast

  MacAddress destMac;
  destMac = request->hwDest;

  memset(request->hwDest, 0, 6);

  Ethernet::send(sizeof(arpHeader), packet, pCard, destMac, ETH_ARP);

  NetworkStack::instance().getMemPool().free(packet);
}

uint64_t Arp::executeRequest(uint64_t p1, uint64_t p2, uint64_t p3, uint64_t p4, uint64_t p5,
                             uint64_t p6, uint64_t p7, uint64_t p8)
{
  ArpRequest* req = reinterpret_cast<ArpRequest*>(p1);
  Network *pCard = reinterpret_cast<Network*>(p2);

  // Send the request
  req->success = false;
  send(req->destIp, pCard);

  // Wait for the reply
  req->waitSem.acquire(1, 15);
  return req->success ? 1 : 0;
}

bool Arp::isInCache(IpAddress ip)
{
    return m_ArpCache.lookup(ip.getIp()) != 0;
}

void Arp::insertToCache(IpAddress ip, MacAddress mac)
{
    if(isInCache(ip))
        return;

    arpEntry* ent = new arpEntry;
    ent->valid = true;
    ent->mac = mac;
    ent->ip = ip;
    m_ArpCache.insert(ip.getIp(), ent);
}

void Arp::removeFromCache(IpAddress ip)
{
    /// \todo Implement
    WARNING("ARP: implement removeFromCache");
}

void Arp::receive(size_t nBytes, uintptr_t packet, Network* pCard, uint32_t offset)
{
  if(!packet || !nBytes)
      return;

  // Check for filtering
  if(!NetworkFilter::instance().filter(2, packet + offset, nBytes - offset))
  {
    pCard->droppedPacket();
    return;
  }

  // grab the header
  arpHeader* header = reinterpret_cast<arpHeader*>(packet + offset);

  // sanity checking
  if(header->hwSize != 6 || header->protocolSize != 4)
  {
    WARNING("Arp: Request for either unknown MAC format or non-IPv4 address");
    return;
  }

  StationInfo cardInfo = pCard->getStationInfo();

    // grab the source MAC
    MacAddress sourceMac;
    sourceMac = header->hwSrc;

    // request?
    if(BIG_TO_HOST16(header->opcode) == ARP_OP_REQUEST)
    {
      // Add to local cache even if the request isn't for us
      MacAddress myMac = cardInfo.mac;
      insertToCache(IpAddress(header->ipSrc), sourceMac);

      // We can glean information from ARP requests, but unless they're for us
      // we can't really respond.
      if((cardInfo.ipv4 == header->ipDest))
      {
          // allocate the reply
          uintptr_t packet = NetworkStack::instance().getMemPool().allocate();
          arpHeader* reply = reinterpret_cast<arpHeader*>(packet);
          memcpy(reply, header, sizeof(arpHeader));
          reply->opcode = HOST_TO_BIG16(ARP_OP_REPLY);
          reply->ipSrc = cardInfo.ipv4.getIp();
          reply->ipDest = header->ipSrc;
          memcpy(reply->hwSrc, cardInfo.mac, 6);
          memcpy(reply->hwDest, header->hwSrc, 6);

          // send it out
          Ethernet::send(sizeof(arpHeader), packet, pCard, sourceMac, ETH_ARP);

          // and now that it's sent, destroy the reply
          NetworkStack::instance().getMemPool().free(packet);
      }
    }
    // reply
    else if(BIG_TO_HOST16(header->opcode) == ARP_OP_REPLY)
    {
      NOTICE("arp " << IpAddress(header->ipSrc).toString() << " is at " << sourceMac.toString());

      // add to our local cache, if needed
      insertToCache(IpAddress(header->ipSrc), sourceMac);

      // search all the requests we've made, trigger the first we find
      for(Vector<ArpRequest*>::Iterator it = m_ArpRequests.begin();
          it != m_ArpRequests.end();
          it++)
      {
        ArpRequest* p = *it;
        if(p->destIp.getIp() == header->ipSrc)
        {
          p->mac = header->hwSrc;
          p->waitSem.release();
          p->success = true;
          m_ArpRequests.erase(it);
          break;
        }
      }
    }
    else
    {
      WARNING("Arp: Unknown ARP opcode: " << BIG_TO_HOST16(header->opcode));
      pCard->badPacket();
    }
}
