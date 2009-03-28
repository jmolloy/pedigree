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
#include <Log.h>

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

int RawEndpoint::send(size_t nBytes, uintptr_t buffer, RemoteEndpoint remoteHost, bool broadcast, Network* pCard)
{
  Ethernet::instance().receive(nBytes, buffer, pCard, 0);
  return true;
};

int RawEndpoint::recv(uintptr_t buffer, size_t maxSize, RemoteEndpoint* remoteHost)
{
  if(m_DataQueue.count())
  {
    DataBlock* ptr = m_DataQueue.popFront();
    
    // only read in this packet
    size_t nBytes = maxSize;
    if(nBytes >= ptr->size)
      nBytes = ptr->size;
    
    memcpy(reinterpret_cast<void*>(buffer), reinterpret_cast<void*>(ptr->ptr), nBytes);
  
    delete reinterpret_cast<uint8_t*>(ptr->ptr);
    delete ptr;
    return nBytes;
  }
  return 0;
};

void RawEndpoint::depositPacket(size_t nBytes, uintptr_t payload)
{
  /// \note Perhaps nBytes should also have an upper limit check?
  if(!nBytes || !payload)
    return;
  uint8_t* data = new uint8_t[nBytes];
  memcpy(data, reinterpret_cast<void*>(payload), nBytes);
  
  DataBlock* newBlock = new DataBlock;
  newBlock->ptr = reinterpret_cast<uintptr_t>(data);
  newBlock->size = nBytes;
  
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

void RawManager::receive(uintptr_t payload, size_t payloadSize, Network* pCard)
{
  // iterate through each endpoint, add this packet
  for(List<Endpoint*>::Iterator it = m_Endpoints.begin(); it != m_Endpoints.end(); it++)
  {
    RawEndpoint* e = reinterpret_cast<RawEndpoint*>((*it));
    e->depositPacket(payloadSize, payload);
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

Endpoint* RawManager::getEndpoint()
{
  Endpoint* ret = new RawEndpoint();
  m_Endpoints.pushBack(ret);
  return ret;
}
