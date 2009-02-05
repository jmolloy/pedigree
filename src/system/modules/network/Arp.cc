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

bool Arp::getFromCache(uint32_t ipv4, bool resolve, MacAddress* ent, Network* pCard)
{
  // do we have an entry for it yet?
  arpEntry* arpEnt;
  if((arpEnt = m_ArpCache.lookup(ipv4)) != 0)
  {
    *ent = arpEnt->station.mac;
    return true;
  }
  
  // not found, do we resolve?
  if(!resolve)
    return false;
  arpRequest* req = new arpRequest;
  
  /// \todo IPv6
  req->destStation.ipv4 = ipv4;
  
  // push this onto the list
  m_ArpRequests.pushBack(req);
  
  // send the request
  send(req->destStation, pCard);
  
  // wait for the reply
  req->waitSem.acquire();
  
  // load it up
  *ent = req->destStation.mac;
  
  // clean up
  delete req;
  
  // success
  return true;
}

void Arp::send(stationInfo req, Network* pCard)
{
  stationInfo cardInfo = pCard->getStationInfo();
  
  arpHeader* request = new arpHeader;
  
  request->hwType = HOST_TO_BIG16(0x0001); // ethernet
  request->hwSize = 6;
  
  request->protocol = HOST_TO_BIG16(ETH_IP);
  request->protocolSize = 4; /// \todo IPv6
  
  request->opcode = HOST_TO_BIG16(ARP_OP_REQUEST);
  
  request->ipSrc = cardInfo.ipv4;
  request->ipDest = req.ipv4;
  
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
  
  // is this for us?
  stationInfo cardInfo = pCard->getStationInfo();
  if(header->ipDest == cardInfo.ipv4)
  {
    // grab the source MAC
    MacAddress sourceMac;
    sourceMac = header->hwSrc;
    
    // request?
    if(BIG_TO_HOST16(header->opcode) == ARP_OP_REQUEST)
    {
      //NOTICE("Arp: Request");
      
      // sanity checking
      if(header->hwSize != 6 || header->protocolSize != 4)
      {
        WARNING("Arp: Request for either unknown MAC format or non-IPv4 address");
        return;
      }
      
      MacAddress myMac = cardInfo.mac;
      
      // add to our local cache, if needed
      if(m_ArpCache.lookup(header->ipSrc) == 0)
      {
        arpEntry* ent = new arpEntry;
        ent->valid = true;
        ent->station.mac = sourceMac;
        ent->station.ipv4 = header->ipSrc;
        m_ArpCache.insert(header->ipSrc, ent);
        //m_ArpCache.pushBack(ent);
      }
      
      // allocate the reply
      arpHeader* reply = new arpHeader;
      memcpy(reply, header, sizeof(arpHeader));
      reply->opcode = HOST_TO_BIG16(ARP_OP_REPLY);
      reply->ipSrc = cardInfo.ipv4;
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
      //NOTICE("Arp: Reply");
      
      // add to our local cache, if needed
      if(m_ArpCache.lookup(header->ipSrc) == 0)
      {
        arpEntry* ent = new arpEntry;
        ent->valid = true;
        ent->station.mac = sourceMac;
        ent->station.ipv4 = header->ipSrc;
        m_ArpCache.insert(header->ipSrc, ent);
        //m_ArpCache.pushBack(ent);
      }
      
      // search all the requests we've made, trigger the first we find
      for(Vector<arpRequest*>::Iterator it = m_ArpRequests.begin();
          it != m_ArpRequests.end();
          it++)
      {
        if ((*it)->destStation.ipv4 == header->ipSrc)
        {
          (*it)->destStation.mac = header->hwSrc;
          (*it)->waitSem.release();
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
