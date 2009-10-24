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

String qnameLabelHelper(char *buff, size_t &len, uintptr_t buffLoc)
{
    String ret;
    size_t retLen = 0;
    char *locbuff = buff;

    // Current offset within this section
    size_t currOffset = 0;

    // Current size of this section
    size_t sectionSize = *locbuff++;

    // Keep reading until we hit a zero-sized section
    while(true)
    {
        retLen++;

        // Read in the character at this specific position
        char c = *locbuff++;

        // If it's null, then this is the beginning of a zero-sized section, so
        // break from the loop: we're done.
        if(!c)
            break;
        // If the top two bits are set, then it's an inline pointer (FUN)
        else if((c & 0xC0) == 0xC0)
        {
            // The rest of this read comes from *that* location!
            char offset = *locbuff++;
            locbuff = reinterpret_cast<char*>(buffLoc + offset);
            ret += ".";

            // Prepare to read the next part of the hostname
            currOffset = 0;
            sectionSize = *locbuff++;
            continue;
        }

        // Otherwise, check to see if we're at the end of the section
        if(currOffset++ == sectionSize)
        {
            ret += ".";
            sectionSize = c;
            currOffset = 0;
        }
        else
        {
            char s[2] = {c, 0};
            ret += s;
        }
      }

      // Count the last section size byte
      retLen++;

      // All done!
      len = retLen;
      return ret;
}

Dns::Dns() :
  m_DnsCache(), m_DnsRequests(), m_Endpoint(0)
{
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

void Dns::initialise()
{
    Endpoint *p = UdpManager::instance().getEndpoint(IpAddress(), 0, 53);
    m_Endpoint = static_cast<ConnectionlessEndpoint *>(p);
    m_Endpoint->acceptAnyAddress(true);
    new Thread(Processor::information().getCurrentThread()->getParent(),
               reinterpret_cast<Thread::ThreadStartFunc>(&trampoline),
               reinterpret_cast<void*>(this));
}

int Dns::trampoline(void* p)
{
  Dns *pDns = reinterpret_cast<Dns*>(p);
  pDns->mainThread();
  return 0;
}

void Dns::mainThread()
{
  uint8_t* buff = new uint8_t[1024];
  uintptr_t buffLoc = reinterpret_cast<uintptr_t>(buff);
  memset(buff, 0, 1024);

  IpAddress addr(Network::convertToIpv4(0, 0, 0, 0));
  ConnectionlessEndpoint* e = m_Endpoint;

  Endpoint::RemoteEndpoint remoteHost;

  while(true)
  {
    if(e->dataReady(true))
    {
      // Read the packet (Safe to block because we've already run dataReady)
      int n = e->recv(buffLoc, 1024, true, &remoteHost);
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

      uint16_t qCount = BIG_TO_HOST16(head->qCount);
      uint16_t ansCount = BIG_TO_HOST16(head->aCount);
      uint16_t nameCount = BIG_TO_HOST16(head->nCount);
      uint16_t addCount = BIG_TO_HOST16(head->dCount);

      // Read in each question section
      size_t ansOffset = sizeof(DnsHeader);
      for(uint16_t question = 0; question < qCount; question++)
      {
          // Read in the string (have to do this to get the size... bah). Note
          // that the NAME field for a QUESTION section will never be a pointer,
          // unlike the ANSWER section below.
          size_t nameLength = 0;
          String tmp = qnameLabelHelper(reinterpret_cast<char*>(buffLoc + ansOffset), nameLength, buffLoc);

          // Add to the offset. We don't really care about the suffix at the
          // moment as we're mainly doing this to get to the answer section.
          ansOffset += nameLength + sizeof(QuestionSecNameSuffix);
      }

      // read the hostname to find the start of the question and answer structures
      /*size_t hostLen = 0;
      String hostname = qnameLabelHelper(reinterpret_cast<char*>(buffLoc + sizeof(DnsHeader)), hostLen);
      req->entry->hostname = hostname;

      NOTICE("Obtained hostname " << hostname << " len " << hostLen << ".");*/

      uintptr_t structStart = buffLoc + ansOffset; // + hostLen + sizeof(QuestionSecNameSuffix);
      uintptr_t ansStart = structStart;

      req->entry->ip = 0;
      req->entry->numIps = 0;

      // http://www.zytrax.com/books/dns/ch15/ for future reference
      for(uint16_t answer = 0; answer < (ansCount + nameCount + addCount); answer++)
      {
        DnsAnswer* ans = reinterpret_cast<DnsAnswer*>(ansStart);
        if(BIG_TO_HOST16(ans->name) & 0xC000)
        {
          // This is an offset within the DNS packet to the string
          size_t len = 0;
          size_t offset = BIG_TO_HOST16(ans->name) - 0xC000;
          String name = qnameLabelHelper(reinterpret_cast<char*>(buffLoc + offset), len, buffLoc);
          NOTICE("Got: " << name << ", len " << len << ".");
        }
        else
        {
          // This is a name component within the answer section
          size_t len = 0;
          String name = qnameLabelHelper(reinterpret_cast<char*>(buffLoc + ansStart), len, buffLoc);
          NOTICE("Got: " << name << ", len " << len << ".");

          // Update the answer pointer so the rest of the structure comes across properly
          ans = reinterpret_cast<DnsAnswer*>((ansStart + len) - sizeof(ans->name));
        }

        switch(BIG_TO_HOST16(ans->type))
        {
          /** A Record */
          case 0x0001:
            {
              /// \todo A record -> h_aliases in hostent
              /// \todo Each IP address we pick up from an A record should go
              ///       into the h_addr_list in hostent
              NOTICE("A Record");
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
            /** NS */
            case 0x0002:
            {
                WARNING("TODO: NS Records");
            }
            break;
            /** CNAME */
            case 0x0005:
            {
                /// \todo CNAME -> h_name in hostent
                WARNING("TODO: DNS CNAME records");
            }
            break;
            /** SOA */
            case 0x0006:
            {
                WARNING("TODO: SOA Records");
            }
            break;
            /* WKS */
            case 0x000B:
            {
                WARNING("TODO: WKS Records");
            }
            break;
            /* PTR */
            case 0x000C:
            {
                WARNING("TODO: PTR Records");
            }
            break;
            /* MX */
            case 0x000F:
            {
                WARNING("TODO: MX Records");
            }
            break;
            /* SRV */
            case 0x0021:
            {
                WARNING("TODO: SRV Records");
            }
            break;
            /* A6 */
            case 0x0026:
            {
                WARNING("TODO: A6 Records");
            }
            break;
            default:
            {
                WARNING("Unknown record type returned from DNS server");
            }
            break;
        }

        ansStart += sizeof(DnsAnswer) + BIG_TO_HOST16(ans->length);
      }

      m_DnsCache.pushBack(req->entry);
      req->success = true;
      req->waitSem.release();
    }
  }
}

IpAddress* Dns::hostToIp(String hostname, size_t& nIps, Network* pCard)
{
  if(!pCard || !pCard->isConnected())
  {
    nIps = 0;
    return 0;
  }

  // check the DNS cache
  for(List<DnsEntry*>::Iterator it = m_DnsCache.begin(); it!= m_DnsCache.end(); it++)
  {
    if(!strcmp(static_cast<const char*>((*it)->hostname), static_cast<const char*>(hostname)))
    {
      nIps = (*it)->numIps;
      return (*it)->ip;
    }
  }

  // grab the DNS server to use
  StationInfo info = pCard->getStationInfo();
  if(info.nDnsServers == 0)
  {
    nIps = 0;
    return 0;
  }

  // setup for our request
  ConnectionlessEndpoint* e = m_Endpoint;

  uint8_t* buff = new uint8_t[1024];
  uintptr_t buffLoc = reinterpret_cast<uintptr_t>(buff);
  memset(buff, 0, 1024);

  // setup the DNS message header
  DnsHeader* head = reinterpret_cast<DnsHeader*>(buffLoc);
  head->id = m_NextId++;
  head->opAndParam = HOST_TO_BIG16(DNS_RECURSION);
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
  remoteHost.ip = info.dnsServers[0];

  // shove all this into a DnsRequest ready for replies
  DnsRequest* req = new DnsRequest;
  DnsEntry* ent = new DnsEntry;
  req->id = head->id;
  req->entry = ent;

  m_DnsRequests.pushBack(req);

  req->success = false;
  e->send(sizeof(DnsHeader) + sizeof(QuestionSecNameSuffix) + len + 1, buffLoc, remoteHost, false);

  req->waitSem.acquire(1, 15);

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
