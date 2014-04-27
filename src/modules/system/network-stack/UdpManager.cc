/*
 * 
 * Copyright (c) 2008-2014, Pedigree Developers
 *
 * Please see the CONTRIB file in the root of the source tree for a full
 * list of contributors.
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
#include "UdpManager.h"
#include <Log.h>
#include <syscallError.h>
#include <processor/Processor.h>

UdpManager UdpManager::manager;

void UdpEndpoint::setLocalPort(uint16_t port)
{
    if(!port)
    {
        port = UdpManager::instance().allocatePort();
        if(!port)
            return;
    }

    UdpManager::instance().bindEndpoint(this, port);

    Endpoint::setLocalPort(port);
}

int UdpEndpoint::send(size_t nBytes, uintptr_t buffer, RemoteEndpoint remoteHost, bool broadcast, Network *pCard)
{
  bool success = Udp::instance().send(remoteHost.ip, getLocalPort(), remoteHost.remotePort, nBytes, buffer, broadcast, pCard);
  return success ? nBytes : -1;
};

int UdpEndpoint::recv(uintptr_t buffer, size_t maxSize, bool bBlock, RemoteEndpoint* remoteHost, int nTimeout)
{
  // Check for data to be ready. dataReady (acquire) takes one from the Semaphore,
  // so using dataReady *again* to see if data is available is *wrong* unless
  // the queue is empty.
  bool bDataReady = false;
  while(1)
  {
      if(dataReady(bBlock, nTimeout))
      {
          if(m_DataQueueSize.acquire())
          {
              bDataReady = true;
              break;
          }
      }
      else if(!bBlock)
          break;
  }

  if(bDataReady)
  {
    DataBlock* ptr = m_DataQueue.popFront();
    if(ptr->magic != 0xdeadbeef)
    {
      // Bang! We've been overrun, impossible to recover.
      ERROR("UDP packet information is corrupted - magic is not 0xdeadbeef!");
      ERROR("The packet cannot be forwarded to the application *or* freed.");
      delete ptr;
      return 0;
    }

    size_t nBytes = maxSize;
    bool allRead = false;

    // Only read in this block
    if(nBytes >= ptr->size)
    {
      nBytes = ptr->size;
      allRead = true;
    }

    // And only past the offset
    nBytes -= ptr->offset;

    memcpy(reinterpret_cast<void*>(buffer), reinterpret_cast<void*>(ptr->ptr + ptr->offset), nBytes);

    *remoteHost = ptr->remoteHost;

    // If the entire block has not been read, repush it with the new offset
    if(!allRead)
    {
      ptr->offset = nBytes;
      m_DataQueue.pushFront(ptr);
      return nBytes;
    }
    // Otherwise we're done - free the block and return
    else
    {
      delete reinterpret_cast<uint8_t*>(ptr->ptr);
      delete ptr;
      return nBytes;
    }
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

size_t UdpEndpoint::depositPayload(size_t nBytes, uintptr_t payload, RemoteEndpoint remoteHost)
{
  /// \note Perhaps nBytes should also have an upper limit check?
  if(!nBytes || !payload)
    return 0;

  // Do not accept a payload if our receiver is shut down
  if(!m_bCanRecv)
    return 0;

  // Otherwise, grab the data block and add it to the queue
  uint8_t* data = new uint8_t[nBytes];
  memcpy(data, reinterpret_cast<void*>(payload), nBytes);

  DataBlock* newBlock = new DataBlock;
  newBlock->ptr = reinterpret_cast<uintptr_t>(data);
  newBlock->offset = 0;
  newBlock->size = nBytes;
  newBlock->remoteHost = remoteHost;

  m_DataQueue.pushBack(newBlock);
  m_DataQueueSize.release();

  // Data has arrived!
  for(List<Socket*>::Iterator it = m_Sockets.begin(); it != m_Sockets.end(); ++it)
  {
    (*it)->endpointStateChanged();
  }

  // Data added now
  return nBytes;
}

bool UdpEndpoint::dataReady(bool block, uint32_t tmout)
{
    // Fast check for data in queue.
    bool bResult = m_DataQueueSize.tryAcquire();
    if(!bResult)
    {
        // No data, no blocking - no go!
        if(!block)
            return false;

        // Block (until a timeout, if one is present)
        if(tmout)
            bResult = m_DataQueueSize.acquire(1, tmout);
        else
            bResult = m_DataQueueSize.acquire();
    }

    if(bResult)
        m_DataQueueSize.release(); // Undo the acquire we just did.
    return bResult;
}

void UdpManager::receive(IpAddress from, IpAddress to, uint16_t sourcePort, uint16_t destPort, uintptr_t payload, size_t payloadSize, Network* pCard)
{
  if(!pCard)
    return;

  // is there an endpoint for this port?
  ConnectionlessEndpoint* e;
  {
    LockGuard<Mutex> guard(m_UdpMutex);
    e = static_cast<ConnectionlessEndpoint *>(m_Endpoints.lookup(destPort));
  }

  StationInfo cardInfo = pCard->getStationInfo();
  if(e != 0)
  {
    /** Should we pass on the packet? **/
    bool passOn = false;

    // Broadcast, and accepting broadcast?
    if(to.getIp() == 0xffffffff)
      passOn = e->acceptAnyAddress();

    // Not to us, but accepting any address?
    else if(to.getIp() != cardInfo.ipv4.getIp())
      passOn = e->acceptAnyAddress();

    // To us!
    else
      passOn = true;

    if(!passOn)
      return;

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
  LockGuard<Mutex> guard(m_UdpMutex);
  if(e)
  {
    m_Endpoints.remove(e->getLocalPort());
    delete e;
  }
}

Endpoint* UdpManager::getEndpoint(IpAddress remoteHost, uint16_t localPort, uint16_t remotePort)
{
  // Try to find a unique port.
  if(localPort == 0)
  {
    localPort = allocatePort();
    if(localPort == 0)
        return 0;
  }

  {
      LockGuard<Mutex> guard(m_UdpMutex);

      // Is there an endpoint for this port?
      /// \note If there is already an Endpoint, we do *not* reallocate it.
      /// \todo Can UDP endpoints be shared? Is it a good idea?
      Endpoint* e;
      if((e = m_Endpoints.lookup(localPort)) == 0)
      {
          e = new UdpEndpoint(remoteHost, localPort, remotePort);
          e->setManager(this);

          m_Endpoints.insert(localPort, e);
      }

      return e;
  }
}

uint16_t UdpManager::allocatePort()
{
    LockGuard<Mutex> guard(m_UdpMutex);

    size_t bit = m_PortsAvailable.getFirstClear();
    if(bit > 0xFFFF)
    {
        WARNING("No UDP ports available.");
        return 0;
    }
    m_PortsAvailable.set(bit);

    return static_cast<uint16_t>(bit & 0xFFFF);
}

void UdpManager::bindEndpoint(Endpoint *p, size_t localPort)
{
    LockGuard<Mutex> guard(m_UdpMutex);

    // Already allocated?
    Endpoint *e = m_Endpoints.lookup(localPort);
    if(e)
    {
        NOTICE("bindEndpoint called with already-used local port");
        return;
    }

    // Insert.
    p->setManager(this);
    m_Endpoints.insert(localPort, p);
}

