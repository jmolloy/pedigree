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

#include "NetManager.h"
#include "RawManager.h"
#include "NetManager.h"
#include <Log.h>
#include <syscallError.h>

#include "Ipv4.h"

#include "RoutingTable.h"

#include "Filter.h"

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

int RawEndpoint::send(size_t nBytes, uintptr_t buffer, Endpoint::RemoteEndpoint remoteHost, bool broadcast, Network *pCard)
{
  // Grab the NIC to send on.
  if(!pCard)
  {
    IpAddress tmp = remoteHost.ip;
    pCard = RoutingTable::instance().DetermineRoute(&tmp);
    if(!pCard)
    {
      WARNING("RAW: Couldn't find a route for destination '" << remoteHost.ip.toString() << "'.");
      return false;
    }
  }

  bool success = false;
  switch(m_Type)
  {
    case RAW_ICMP:
      success = Ipv4::instance().send(remoteHost.ip, pCard->getStationInfo().ipv4, IP_ICMP, nBytes, buffer);
      if(success)
        return static_cast<int>(nBytes);
      break;
    case RAW_UDP:
      success = Ipv4::instance().send(remoteHost.ip, pCard->getStationInfo().ipv4, IP_UDP, nBytes, buffer);
      if(success)
        return static_cast<int>(nBytes);
      break;
    case RAW_TCP:
      success = Ipv4::instance().send(remoteHost.ip, pCard->getStationInfo().ipv4, IP_TCP, nBytes, buffer);
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

int RawEndpoint::recv(uintptr_t buffer, size_t maxSize, bool bBlock, Endpoint::RemoteEndpoint* remoteHost, int nTimeout)
{
  // Check for data to be ready. dataReady (acquire) takes one from the Semaphore,
  // so using dataReady *again* to see if data is available is *wrong* unless
  // the queue is empty.
  bool bDataReady = true;
  if(m_DataQueue.count())
    bDataReady = true;
  else if(bBlock)
  {
    if(!dataReady(true, nTimeout))
      bDataReady = false;
  }
  else
    bDataReady = false;

  if(bDataReady)
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

  if(!bBlock)
  {
    // EAGAIN, or EWOULDBLOCK
    SYSCALL_ERROR(NoMoreProcesses);
    return -1;
  }
  else
    return 0;
};

size_t RawEndpoint::depositPacket(size_t nBytes, uintptr_t payload, Endpoint::RemoteEndpoint* remoteHost)
{
  /// \note Perhaps nBytes should also have an upper limit check?
  if(!nBytes || !payload)
    return 0;
  uint8_t* data = new uint8_t[nBytes];
  memcpy(data, reinterpret_cast<void*>(payload), nBytes);

  DataBlock* newBlock = new DataBlock;
  newBlock->ptr = reinterpret_cast<uintptr_t>(data);
  newBlock->size = nBytes;
  if(remoteHost)
    newBlock->remoteHost = *remoteHost;

  m_DataQueue.pushBack(newBlock);
  m_DataQueueSize.release();

  // Data has arrived!
  for(List<Socket*>::Iterator it = m_Sockets.begin(); it != m_Sockets.end(); ++it)
    (*it)->endpointStateChanged();
  return nBytes;
}

bool RawEndpoint::dataReady(bool block, uint32_t tmout)
{
    // Attempt to avoid setting up the timeout if possible
    if(m_DataQueueSize.tryAcquire())
        return true;
    else if(!block)
        return false;

    // Otherwise jump straight into blocking
    if(block)
    {
        if(tmout)
            m_DataQueueSize.acquire(1, tmout);
        else
            m_DataQueueSize.acquire();

        if(Processor::information().getCurrentThread()->wasInterrupted())
            return false;
    }
    return true;
}

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

    if(e->getRawType() == endType)
    {
        NOTICE("Depositing payload");
      e->depositPacket(payloadSize, payload, remoteHost);
    }
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
  {
    ret->setManager(this);
    m_Endpoints.pushBack(ret);
  }
  return ret;
}
