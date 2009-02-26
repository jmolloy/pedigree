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
#ifndef MACHINE_UDPMANAGER_H
#define MACHINE_UDPMANAGER_H

#include <utilities/String.h>
#include <utilities/Tree.h>
#include <utilities/List.h>
#include <process/Semaphore.h>
#include <machine/Network.h>

#include "Endpoint.h"
#include "Udp.h"

/**
 * The Pedigree network stack - UDP Endpoint
 * \todo This needs to keep track of a LOCAL IP as well
 *        so we can actually bind properly if there's multiple
 *        cards with different configurations.
 */
class UdpEndpoint : public Endpoint
{
  public:
  
    /** Constructors and destructors */
    UdpEndpoint() :
      Endpoint(), m_DataQueue(), m_DataQueueSize(0), m_bAcceptAll(false)
    {};
    UdpEndpoint(uint16_t local, uint16_t remote) :
      Endpoint(local, remote), m_DataQueue(), m_DataQueueSize(0), m_bAcceptAll(false)
    {};
    UdpEndpoint(IpAddress remoteIp, uint16_t local = 0, uint16_t remote = 0) :
      Endpoint(remoteIp, local, remote), m_DataQueue(), m_DataQueueSize(0), m_bAcceptAll(false)
    {};
    virtual ~UdpEndpoint() {};
    
    /** Application interface */
    virtual bool send(size_t nBytes, uintptr_t buffer, RemoteEndpoint remoteHost, bool broadcast, Network* pCard);
    virtual size_t recv(uintptr_t buffer, size_t maxSize, RemoteEndpoint* remoteHost);
    virtual bool dataReady(bool block = false, uint32_t tmout = 30);
    virtual inline bool acceptAnyAddress() { return m_bAcceptAll; };
    virtual inline void acceptAnyAddress(bool accept) { m_bAcceptAll = accept; };
    
    /** UdpManager functionality - called to deposit data into our local buffer */
    virtual void depositPayload(size_t nBytes, uintptr_t payload, RemoteEndpoint remoteHost);
  
  private:
  
    struct DataBlock
    {
      DataBlock() :
        size(0), offset(0), ptr(0), remoteHost()
      {};
      
      size_t size;
      size_t offset; // if we only do a partial read, this is filled
      uintptr_t ptr;
      
      RemoteEndpoint remoteHost; // who sent it to us - needed for port info!
    };
  
    /** Incoming data queue */
    List<DataBlock*> m_DataQueue;
    
    /** Data queue size */
    Semaphore m_DataQueueSize;
    
    /** Accept any address? */
    bool m_bAcceptAll;
};

/**
 * The Pedigree network stack - UDP Protocol Manager
 */
class UdpManager
{
public:
  UdpManager() :
    m_Endpoints()
  {};
  virtual ~UdpManager()
  {};
  
  /** For access to the manager without declaring an instance of it */
  static UdpManager& instance()
  {
    return manager;
  }
  
  /** Gets a new Endpoint */
  Endpoint* getEndpoint(IpAddress remoteHost, uint16_t localPort, uint16_t remotePort);
  
  /** Returns an Endpoint */
  void returnEndpoint(Endpoint* e);
  
  /** A new packet has arrived! */
  void receive(IpAddress from, IpAddress to, uint16_t sourcePort, uint16_t destPort, uintptr_t payload, size_t payloadSize, Network* pCard);

private:

  static UdpManager manager;

  /** Currently known endpoints (all actually UdpEndpoints). */
  Tree<size_t, Endpoint*> m_Endpoints;
};

#endif
