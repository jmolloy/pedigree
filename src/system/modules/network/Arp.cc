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

Arp Arp::arpInstance;

Arp::Arp() :
  m_ArpCache(), m_ArpRequests()
{
}

Arp::~Arp()
{
}

bool Arp::getFromCache(IpAddress ip, bool resolve, MacAddress* ent, Network* pCard)
{  
  // ensure the IP given is valid
  if(ip.getType() == IpAddress::IPv6)
    return false; // ARP isn't for IPv6
  
  // do we have an entry for it yet?
  arpEntry* arpEnt;
  if((arpEnt = m_ArpCache.lookup(ip.getIp())) != 0)
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
  
  // send the request
  send(req->destIp, pCard);
  
  // wait for the reply
  Timer* t = Machine::instance().getTimer();
  if(t)
    t->registerHandler(req);
  req->waitSem.acquire();
  bool success = req->success;
  
  // we've returned, stop the handler being called
  if(t)
    t->unregisterHandler(req);
  
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
  
  arpHeader* request = new arpHeader;
  
  request->hwType = HOST_TO_BIG16(0x0001); // ethernet
  request->hwSize = 6;
  
  request->protocol = HOST_TO_BIG16(ETH_IP);
  request->protocolSize = 4;
  
  request->opcode = HOST_TO_BIG16(ARP_OP_REQUEST);
  
  request->ipSrc = cardInfo.ipv4.getIp();
  request->ipDest = req.getIp();
  
  memcpy(request->hwSrc, cardInfo.mac, 6);
  memset(request->hwDest, 0xff, 6); // broadcast
  
  MacAddress destMac;
  destMac = request->hwDest;
  
  memset(request->hwDest, 0, 6);
  
  Ethernet::send(sizeof(arpHeader), reinterpret_cast<uintptr_t>(request), pCard, destMac, ETH_ARP);
  
  delete request;
}

void Arp::receive(size_t nBytes, uintptr_t packet, Network* pCard, uint32_t offset)
{  
  // grab the header
  arpHeader* header = reinterpret_cast<arpHeader*>(packet + offset);

  // sanity checking
  if(header->hwSize != 6 || header->protocolSize != 4)
  {
    WARNING("Arp: Request for either unknown MAC format or non-IPv4 address");
    return;
  }
  
  // is this for us?
  StationInfo cardInfo = pCard->getStationInfo();
  if(cardInfo.ipv4 == header->ipDest)
  {    
    // grab the source MAC
    MacAddress sourceMac;
    sourceMac = header->hwSrc;
    
    // request?
    if(BIG_TO_HOST16(header->opcode) == ARP_OP_REQUEST)
    {      
      MacAddress myMac = cardInfo.mac;
      
      // add to our local cache, if needed
      if(m_ArpCache.lookup(header->ipSrc) == 0)
      {
        arpEntry* ent = new arpEntry;
        ent->valid = true;
        ent->mac = sourceMac;
        ent->ip.setIp(header->ipSrc);
        m_ArpCache.insert(header->ipSrc, ent);
        //m_ArpCache.pushBack(ent);
      }
      
      // allocate the reply
      arpHeader* reply = new arpHeader;
      memcpy(reply, header, sizeof(arpHeader));
      reply->opcode = HOST_TO_BIG16(ARP_OP_REPLY);
      reply->ipSrc = cardInfo.ipv4.getIp();
      reply->ipDest = header->ipSrc;
      memcpy(reply->hwSrc, cardInfo.mac, 6);
      memcpy(reply->hwDest, header->hwSrc, 6);
      
      // send it out
      Ethernet::send(sizeof(arpHeader), reinterpret_cast<uintptr_t>(reply), pCard, sourceMac, ETH_ARP);
      
      // and now that it's sent, destroy the reply
      delete reply;
    }
    // reply
    else if(BIG_TO_HOST16(header->opcode) == ARP_OP_REPLY)
    {
      // add to our local cache, if needed
      if(m_ArpCache.lookup(header->ipSrc) == 0)
      {
        arpEntry* ent = new arpEntry;
        ent->valid = true;
        ent->mac = sourceMac;
        ent->ip.setIp(header->ipSrc);
        m_ArpCache.insert(header->ipSrc, ent);
        //m_ArpCache.pushBack(ent);
      }
      
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
    }
  }
}
