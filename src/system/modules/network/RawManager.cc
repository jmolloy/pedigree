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

#include "RawManager.h"
#include "NetManager.h"
#include <Log.h>

#include "Ip.h"

RawManager RawManager::manager;

RawEndpoint::~RawEndpoint()
{
  while(m_DataQueue.count())
  {
    DataBlock* ptr = m_DataQueue.popFront();
    delete [] reinterpret_cast<uint8_t*>(ptr->ptr);
    delete ptr;
  }
}

int RawEndpoint::send(size_t nBytes, uintptr_t buffer, Endpoint::RemoteEndpoint remoteHost, bool broadcast, Network* pCard)
{
  bool success = false;
  switch(m_Type)
  {
    case RAW_ICMP:
      success = Ip::send(remoteHost.ip, pCard->getStationInfo().ipv4, IP_ICMP, nBytes, buffer, pCard);
      if(success)
        return static_cast<int>(nBytes);
      break;
    case RAW_UDP:
      success = Ip::send(remoteHost.ip, pCard->getStationInfo().ipv4, IP_UDP, nBytes, buffer, pCard);
      if(success)
        return static_cast<int>(nBytes);
      break;
    case RAW_TCP:
      success = Ip::send(remoteHost.ip, pCard->getStationInfo().ipv4, IP_TCP, nBytes, buffer, pCard);
      if(success)
        return static_cast<int>(nBytes);
      break;

    // RAW_WIRE is for them PF_SOCKET things... it'll allow you direct access to the very bottom level of the
    // implementation.
    case RAW_WIRE:
    default:
      pCard->send(nBytes, buffer);
      return static_cast<int>(nBytes);
      break;
  };
  return -1;
};

int RawEndpoint::recv(uintptr_t buffer, size_t maxSize, Endpoint::RemoteEndpoint* remoteHost)
{
  if(m_DataQueue.count())
  {
    DataBlock* ptr = m_DataQueue.popFront();
    
    // only read in this packet
    size_t nBytes = maxSize;
    if(nBytes >= ptr->size)
      nBytes = ptr->size;
    
    memcpy(reinterpret_cast<void*>(buffer), reinterpret_cast<void*>(ptr->ptr), nBytes);

    *remoteHost = ptr->remoteHost;
  
    delete reinterpret_cast<uint8_t*>(ptr->ptr);
    delete ptr;
    return nBytes;
  }
  return -1; // no data
};

void RawEndpoint::depositPacket(size_t nBytes, uintptr_t payload, Endpoint::RemoteEndpoint* remoteHost)
{
  /// \note Perhaps nBytes should also have an upper limit check?
  if(!nBytes || !payload)
    return;
  uint8_t* data = new uint8_t[nBytes];
  memcpy(data, reinterpret_cast<void*>(payload), nBytes);
  
  DataBlock* newBlock = new DataBlock;
  newBlock->ptr = reinterpret_cast<uintptr_t>(data);
  newBlock->size = nBytes;
  if(remoteHost)
    newBlock->remoteHost = *remoteHost;
  
  m_DataQueue.pushBack(newBlock);
  m_DataQueueSize.release();
}

bool RawEndpoint::dataReady(bool block, uint32_t tmout)
{
  bool timedOut = false;
  if(block)
  {
    Timer* t = Machine::instance().getTimer();
    NetworkBlockTimeout* timeout = new NetworkBlockTimeout;
    timeout->setTimeout(tmout);
    timeout->setSemaphore(&m_DataQueueSize);
    timeout->setTimedOut(&timedOut);
    if(t)
      t->registerHandler(timeout);
    m_DataQueueSize.acquire();
    if(t)
      t->unregisterHandler(timeout);
    delete timeout;
  }
  else
    return m_DataQueueSize.tryAcquire();
  return !timedOut;
};

void RawManager::receive(uintptr_t payload, size_t payloadSize, Endpoint::RemoteEndpoint* remoteHost, int type, Network* pCard)
{
  RawEndpoint::Type endType;
  if(type == IPPROTO_UDP)
    endType = RawEndpoint::RAW_UDP;
  else if(type == IPPROTO_TCP)
    endType = RawEndpoint::RAW_TCP;
  else if(type == IPPROTO_ICMP)
    endType = RawEndpoint::RAW_ICMP;
  else
    endType = RawEndpoint::RAW_WIRE;

  // iterate through each endpoint, add this packet
  for(List<Endpoint*>::Iterator it = m_Endpoints.begin(); it != m_Endpoints.end(); it++)
  {
    RawEndpoint* e = reinterpret_cast<RawEndpoint*>((*it));

    if(e->getType() == endType)
      e->depositPacket(payloadSize, payload, remoteHost);
  }
}

void RawManager::returnEndpoint(Endpoint* e)
{
  if(e)
  {
    for(List<Endpoint*>::Iterator it = m_Endpoints.begin(); it != m_Endpoints.end(); it++)
    {
      if((*it) == e)
      {
        m_Endpoints.erase(it);
        delete e;
        break;
      }
    }
  }
}

Endpoint* RawManager::getEndpoint(int proto)
{
  Endpoint* ret;
  switch(proto)
  { 
    // icmp
    case IPPROTO_ICMP:
      ret = new RawEndpoint(RawEndpoint::RAW_ICMP);
      break;
    
    // udp
    case IPPROTO_UDP:
      ret = new RawEndpoint(RawEndpoint::RAW_UDP);
      break;
    
    // tcp
    case IPPROTO_TCP:
      ret = new RawEndpoint(RawEndpoint::RAW_TCP);
      break;

    // wire
    default:
      ret = new RawEndpoint(RawEndpoint::RAW_WIRE);
      break;
  }
  if(ret)
    m_Endpoints.pushBack(ret);
  return ret;
}
