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
#ifndef MACHINE_DNS_H
#define MACHINE_DNS_H

#include <utilities/String.h>
#include <utilities/Vector.h>
#include <utilities/Tree.h>
#include <processor/state.h>
#include <processor/types.h>
#include <process/Semaphore.h>
#include <machine/Network.h>
#include <machine/Machine.h>

#include "NetworkStack.h"
#include "Ethernet.h"
#include "UdpManager.h"

/** These are bit masks for the opAndParam field of the header */
#define DNS_QUESREQ    0x8000 // query = 0, request = 1
#define DNS_OPCODE     0x7800 // 0 = standard, 1 = inverse
#define DNS_AUTHANS    0x400 // 1 if authoritative answer
#define DNS_TRUNC      0x200 // 1 if the message is truncated
#define DNS_RECUSRION  0x100 // 1 if recursion is wanted
#define DNS_RECURAVAIL 0x80 // 1 if recursion is available
#define DNS_RSVD       0x70
#define DNS_RESPONSE   0xF // response code - 0 means no errors

/** Query types */
#define DNSQUERY_HOSTADDR     1

/**
 * The Pedigree network stack - DNS implementation
 */
class Dns
{
public:
  Dns();
  Dns(const Dns& ent);
  virtual ~Dns();
  
  /** For access to the stack without declaring an instance of it */
  static Dns& instance()
  {
    return dnsInstance;
  }
  
  /** Thread trampoline */
  static int trampoline(void* p);
  
  /** Main daemon thread */
  void mainThread();
  
  /** Requests a lookup for a hostname */
  IpAddress* hostToIp(String hostname, size_t& nIps, Network* pCard = 0);
  
  /** Operator = is invalid */
  Dns& operator = (const Dns& ent)
  {
    NOTICE("Dns::operator = called!");
    return *this;
  }

private:

  static Dns dnsInstance;
  static uint16_t m_NextId;

  struct DnsHeader
  {
    uint16_t  id;
    uint16_t  opAndParam;
    uint16_t  qCount; // question entry count
    uint16_t  aCount; // answer entry count
    uint16_t  nCount; // nameserver count
    uint16_t  dCount; // additional data count
  } __attribute__ ((packed));
  
  struct QuestionSecNameSuffix
  {
    uint16_t  type;
    uint16_t  cls;
  } __attribute__ ((packed));
  
  struct DnsAnswer
  {
    uint16_t  name;
    uint16_t  type;
    uint16_t  cls;
    uint32_t  ttl;
    uint16_t  length;
  } __attribute__ ((packed));
  
  /** An entry in the DNS cache */
  class DnsEntry
  {
    public:
      DnsEntry() : ip(0), numIps(0), hostname()
      {};
      DnsEntry(const DnsEntry& ent) : ip(ent.ip), numIps(ent.numIps), hostname(ent.hostname)
      {};
      
      /// Multiple IP addresses are possible
      IpAddress* ip;
      size_t numIps;
      
      /// Hostname
      String hostname;
      
      DnsEntry& operator = (const DnsEntry& ent)
      {
        ip = ent.ip;
        numIps = ent.numIps;
        hostname = ent.hostname;
        return *this;
      }
  };
  
  /// a DNS request we've sent
  class DnsRequest : public TimerHandler
  {
    public:
      DnsRequest() :
        entry(0), id(0), waitSem(0), m_Timeout(30), success(false), m_Nanoseconds(0), m_Seconds(0)
      {};
      DnsRequest(const DnsRequest& ent) :
        entry(ent.entry), id(ent.id), waitSem(ent.waitSem), m_Timeout(ent.m_Timeout),
        success(ent.success), m_Nanoseconds(ent.m_Nanoseconds), m_Seconds(ent.m_Seconds)
      {};
      
      DnsEntry* entry;
      uint16_t id;
      
      Semaphore waitSem;
      
      uint32_t m_Timeout; // defaults to 30 seconds
      
      bool success;
      
      void timer(uint64_t delta, InterruptState &state)
      {
        if(UNLIKELY(m_Seconds < m_Timeout))
        {
          m_Nanoseconds += delta;
          if(UNLIKELY(m_Nanoseconds >= 1000000000ULL))
          {
            ++m_Seconds;
            m_Nanoseconds -= 1000000000ULL;
          }
          
          if(UNLIKELY(m_Seconds >= m_Timeout))
          {
            success = false;
            waitSem.release();
          }
        }
      }
      
      DnsRequest& operator = (const DnsRequest& ent)
      {
        entry = ent.entry;
        id = ent.id;
        waitSem = ent.waitSem;
        m_Timeout = ent.m_Timeout;
        success = ent.success;
        m_Nanoseconds = ent.m_Nanoseconds;
        m_Seconds = ent.m_Seconds;
        return *this;
      }
      
    private:
    
      uint64_t m_Nanoseconds;
      uint64_t m_Seconds;
  };
  
  /// DNS cache (not a Tree because we can't look up an IpAddress object)
  List<DnsEntry*> m_DnsCache;
  
  /// DNS request list
  Vector<DnsRequest*> m_DnsRequests;
  
  /// DNS communication endpoint
  Endpoint* m_Endpoint;
};

#endif
