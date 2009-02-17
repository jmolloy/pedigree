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

#include "UdpManager.h"
#include <Log.h>

UdpManager UdpManager::manager;

bool UdpEndpoint::send(size_t nBytes, uintptr_t buffer, RemoteEndpoint remoteHost, bool broadcast, Network* pCard)
{
  Udp::instance().send(remoteHost.ip, getLocalPort(), remoteHost.remotePort, nBytes, buffer, broadcast, pCard);
  return true;
};

size_t UdpEndpoint::recv(uintptr_t buffer, size_t maxSize, RemoteEndpoint* remoteHost)
{
  if(m_DataQueue.count())
  {
    DataBlock* ptr = m_DataQueue.popFront();
    
    size_t nBytes = maxSize;
    bool allRead = false;
    
    // only read in this block
    if(nBytes >= ptr->size)
    {
      nBytes = ptr->size;
      allRead = true;
    }
    
    // and only past the offset
    nBytes -= ptr->offset;
    
    memcpy(reinterpret_cast<void*>(buffer), reinterpret_cast<void*>(ptr->ptr + ptr->offset), nBytes);
    
    *remoteHost = ptr->remoteHost;
    
    if(!allRead)
    {
      ptr->offset = nBytes;
      m_DataQueue.pushFront(ptr);
      return nBytes;
    }
    else
    {
      delete reinterpret_cast<uint8_t*>(ptr->ptr);
      delete ptr;
      return nBytes;
    }
  }
  return 0;
};

void UdpEndpoint::depositPayload(size_t nBytes, uintptr_t payload, RemoteEndpoint remoteHost)
{
  /// \note Perhaps nBytes should also have an upper limit check?
  if(!nBytes || !payload)
    return;
  uint8_t* data = new uint8_t[nBytes];
  memcpy(data, reinterpret_cast<void*>(payload), nBytes);
  
  DataBlock* newBlock = new DataBlock;
  newBlock->ptr = reinterpret_cast<uintptr_t>(data);
  newBlock->offset = 0;
  newBlock->size = nBytes;
  newBlock->remoteHost = remoteHost;
  
  m_DataQueue.pushBack(newBlock);
  m_DataQueueSize.release();
}

bool UdpEndpoint::dataReady(bool block)
{
  if(block)
    m_DataQueueSize.acquire();
  else
    return m_DataQueueSize.tryAcquire();
  return true;
};

void UdpManager::receive(IpAddress from, uint16_t sourcePort, uint16_t destPort, uintptr_t payload, size_t payloadSize)
{  
  // is there an endpoint for this port?
  Endpoint* e;
  NOTICE("Incoming UDP packet on port " << Dec << destPort << Hex << "...");
  if((e = m_Endpoints.lookup(destPort)) != 0)
  {
    NOTICE("Passing it through");
    // e->setRemotePort(sourcePort);
    Endpoint::RemoteEndpoint host;
    host.ip = from;
    if(sourcePort)
      host.remotePort = sourcePort;
    else
      host.remotePort = destPort;
    e->depositPayload(payloadSize, payload, host);
  }
}

void UdpManager::returnEndpoint(Endpoint* e)
{
  if(e)
  {
    m_Endpoints.remove(e->getLocalPort());
    delete e;
  }
}

Endpoint* UdpManager::getEndpoint(IpAddress remoteHost, uint16_t localPort, uint16_t remotePort)
{
  // is there an endpoint for this port?
  Endpoint* e;
  if((e = m_Endpoints.lookup(localPort)) == 0)
  {
    e = new UdpEndpoint(remoteHost, localPort, remotePort);
    m_Endpoints.insert(localPort, e);
  }
  return e;
}
