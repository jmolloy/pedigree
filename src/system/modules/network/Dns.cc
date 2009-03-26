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

#include "Dns.h"
#include "Ethernet.h"
#include <Module.h>
#include <Log.h>

#include "UdpManager.h"
#include <processor/Processor.h>

Dns Dns::dnsInstance;
uint16_t Dns::m_NextId = 0;

Dns::Dns() :
  m_DnsCache(), m_DnsRequests(), m_Endpoint(0)
{
  new Thread(Processor::information().getCurrentThread()->getParent(),
             reinterpret_cast<Thread::ThreadStartFunc>(&trampoline),
             reinterpret_cast<void*>(this));
  m_Endpoint = UdpManager::instance().getEndpoint(IpAddress(), 0, 53);
  m_Endpoint->acceptAnyAddress(true);
}

Dns::Dns(const Dns& ent) :
  m_DnsCache(ent.m_DnsCache), m_DnsRequests(ent.m_DnsRequests), m_Endpoint(ent.m_Endpoint)
{
  new Thread(Processor::information().getCurrentThread()->getParent(),
             reinterpret_cast<Thread::ThreadStartFunc>(&trampoline),
             reinterpret_cast<void*>(this));
}

Dns::~Dns()
{
}

int Dns::trampoline(void* p)
{
  Dns *pDns = reinterpret_cast<Dns*>(p);
  pDns->mainThread();
  return 0;
}

void Dns::mainThread()
{
  uint8_t* buff = new uint8_t[512];
  uintptr_t buffLoc = reinterpret_cast<uintptr_t>(buff);
  memset(buff, 0, 512);
  
  IpAddress addr(Network::convertToIpv4(0, 0, 0, 0));
  Endpoint* e = m_Endpoint; //UdpManager::instance().getEndpoint(addr, 53, 53);
  
  Endpoint::RemoteEndpoint remoteHost;
  
  while(true)
  {
    if(e->dataReady(true))
    {
      // read the packet
      int n = e->recv(buffLoc, 512, &remoteHost);
      if(n <= 0)
        continue;
      
      // grab the header
      DnsHeader* head = reinterpret_cast<DnsHeader*>(buffLoc);
      
      // look for the ID in our request list
      bool bFound = false;
      DnsRequest* req = 0;
      for(Vector<DnsRequest*>::Iterator it = m_DnsRequests.begin(); it != m_DnsRequests.end(); it++)
      {
        if((*it)->id == head->id)
        {
          req = (*it);
          bFound = true;
          
          m_DnsRequests.erase(it);
          break;
        }
      }
      
      if(!bFound)
        continue;
      
      uint16_t ansCount = BIG_TO_HOST16(head->aCount);
      
      // read the hostname to find the start of the question and answer structures
      String hostname;
      char* tmp = reinterpret_cast<char*>(buffLoc + sizeof(DnsHeader));
      
      size_t hostLen = 1;
      size_t currOffset = 0;
      size_t secSize = *tmp++;
      while(true)
      {
        hostLen++;
        
        char c = *tmp++;
        if(c == 0)
          break;
        
        char s[2];
        s[0] = c;
        s[1] = 0;
        hostname += s;
        
        currOffset++;
        
        if(currOffset == secSize)
        {
          hostname += ".";
          secSize = *tmp++;
        }
      }
        
      // null byte
      hostLen++;
      req->entry->hostname = hostname;
      
      uintptr_t structStart = buffLoc + sizeof(DnsHeader) + hostLen + sizeof(QuestionSecNameSuffix) + 1;
      uintptr_t ansStart = structStart;
      
      req->entry->ip = 0;
      req->entry->numIps = 0;
      
      for(size_t answer = 0; answer < ansCount; answer++)
      {
        DnsAnswer* ans = reinterpret_cast<DnsAnswer*>(ansStart);
        if(BIG_TO_HOST16(ans->name) & 0x2)
        {
          // stores an offset to the name for this entry
          NOTICE("Is an offset - " << (BIG_TO_HOST16(ans->name) - 2) << " bytes");
        }
        else
        {
          // apparently this is actually meant to be a proper name component? weird....
        }

        switch(BIG_TO_HOST16(ans->type))
        {
          /** A Record */
          case 0x0001:
            {
              if(BIG_TO_HOST16(ans->length) == 4)
              {
                IpAddress* newList = new IpAddress[req->entry->numIps + 1];
                if(req->entry->ip)
                {
                  size_t i;
                  for(i = 0; i < req->entry->numIps; i++)
                    newList[i] = req->entry->ip[i];
                  delete [] req->entry->ip;
                }
                req->entry->ip = newList;

                uint32_t newIp = *reinterpret_cast<uint32_t*>(ansStart + sizeof(DnsAnswer));
                req->entry->ip[req->entry->numIps].setIp(newIp);
                req->entry->numIps++;
              }
            }
            break;
          
          /** CNAME */
          case 0x0005:
            {
              WARNING("TODO: DNS CNAME records --> Hostent aliases");
            }
            break;
        }
        
        ansStart += sizeof(DnsAnswer) + BIG_TO_HOST16(ans->length);
      }
      
      
      m_DnsCache.pushBack(req->entry);
      req->success = true;
      req->waitSem.release();

      NOTICE("Handled a response");
    }
  }
}

IpAddress* Dns::hostToIp(String hostname, size_t& nIps, Network* pCard)
{
  // check the DNS cache
  for(List<DnsEntry*>::Iterator it = m_DnsCache.begin(); it!= m_DnsCache.end(); it++)
  {
    if(!strcmp(static_cast<const char*>((*it)->hostname), static_cast<const char*>(hostname)))
    {
      NOTICE("Obtaining from cache");
      nIps = (*it)->numIps;
      return (*it)->ip;
    }
  }
  
  // grab the DNS server to use
  StationInfo info = pCard->getStationInfo();
  if(info.dnsServer.getIp() == 0)
  {
    nIps = 0;
    return 0;
  }
  
  // setup for our request
  Endpoint* e = m_Endpoint; //UdpManager::instance().getEndpoint(info.dnsServer, 0, 53);
  
  uint8_t* buff = new uint8_t[512];
  uintptr_t buffLoc = reinterpret_cast<uintptr_t>(buff);
  memset(buff, 0, 512);
  
  // setup the DNS message header
  DnsHeader* head = reinterpret_cast<DnsHeader*>(buffLoc);
  head->id = m_NextId++;
  head->opAndParam = HOST_TO_BIG16(DNS_RECUSRION);
  head->qCount = HOST_TO_BIG16(1);
  
  // build the modified hostname
  size_t len = hostname.length() + 1;
  char* host = new char[len];
  memset(host, 0, len);
  
  size_t top = hostname.length();
  size_t prevSize = 0;
  for(ssize_t i = top - 1; i >= 0; i--)
  {
    if(hostname[i] == '.')
    {
      host[i+1] = prevSize;
      prevSize = 0;
    }
    else
    {
      host[i+1] = hostname[i];
      prevSize++;
    }
  }
  host[0] = prevSize;
  
  memcpy(reinterpret_cast<char*>(buffLoc + sizeof(DnsHeader)), host, len);
  
  delete [] host;
  
  // requesting the host address
  QuestionSecNameSuffix* q = reinterpret_cast<QuestionSecNameSuffix*>(buffLoc + sizeof(DnsHeader) + len + 1);
  q->type = HOST_TO_BIG16(1);
  q->cls = HOST_TO_BIG16(1);

  Endpoint::RemoteEndpoint remoteHost;
  remoteHost.remotePort = 53;
  remoteHost.ip = info.dnsServer;
  
  // shove all this into a DnsRequest ready for replies
  DnsRequest* req = new DnsRequest;
  DnsEntry* ent = new DnsEntry;
  req->id = head->id;
  req->entry = ent;
  
  m_DnsRequests.pushBack(req);
  
  e->send(sizeof(DnsHeader) + sizeof(QuestionSecNameSuffix) + len + 1, buffLoc, remoteHost, false, pCard);
  
  req->waitSem.acquire();
  
  if(!req->success)
  {
    delete ent;
    delete req;
  }
  else
  {
    delete req;
    
    nIps = ent->numIps;
    return ent->ip;
  }
  
  return 0;
}
