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
#ifndef MACHINE_RAWMANAGER_H
#define MACHINE_RAWMANAGER_H

#include <utilities/String.h>
#include <utilities/Tree.h>
#include <utilities/List.h>
#include <process/Semaphore.h>
#include <machine/Network.h>

#include "Endpoint.h"
#include "Ethernet.h"

/**
 * The Pedigree network stack - RAW Endpoint
 * \todo This is really messy - needs a rewrite at some point!
 */
class RawEndpoint : public Endpoint
{
  public:

    /** What type is this Raw Endpoint? */
    enum Type
    {
      RAW_WIRE = 0, // wire-level
      RAW_ICMP, // IP levels
      RAW_UDP,
      RAW_TCP
    };
  
    /** Constructors and destructors */
    RawEndpoint() :
      Endpoint(), m_DataQueue(), m_DataQueueSize(0), m_bAcceptAll(false), m_Type(RAW_WIRE)
    {};

    /** These shouldn't be used - totally pointless */
    RawEndpoint(uint16_t local, uint16_t remote) :
      Endpoint(local, remote), m_DataQueue(), m_DataQueueSize(0), m_bAcceptAll(false), m_Type(RAW_WIRE)
    {};
    RawEndpoint(IpAddress remoteIp, uint16_t local = 0, uint16_t remote = 0) :
      Endpoint(remoteIp, local, remote), m_DataQueue(), m_DataQueueSize(0), m_bAcceptAll(false), m_Type(RAW_WIRE)
    {};
    RawEndpoint(Type type) :
      Endpoint(0, 0), m_DataQueue(), m_DataQueueSize(0), m_bAcceptAll(false), m_Type(type)
    {};
    virtual ~RawEndpoint();
    
    /** Injects the given buffer into the network stack */
    virtual int send(size_t nBytes, uintptr_t buffer, Endpoint::RemoteEndpoint remoteHost, bool broadcast, Network* pCard);

    /** Reads from the front of the packet queue. Will return truncated packets if maxSize < packet size. */
    virtual int recv(uintptr_t buffer, size_t maxSize, Endpoint::RemoteEndpoint* remoteHost);

    /** Are there packets to read? */
    virtual bool dataReady(bool block = false, uint32_t tmout = 30);

    /** Not relevant in this context
    virtual inline bool acceptAnyAddress() { return m_bAcceptAll; };
    virtual inline void acceptAnyAddress(bool accept) { m_bAcceptAll = accept; }; */
    
    /** Deposits a packet into this endpoint */
    virtual void depositPacket(size_t nBytes, uintptr_t payload, Endpoint::RemoteEndpoint* remoteHost);

    /** What type is this endpoint? */
    inline Type getType() {return m_Type;};
  
  private:
  
    struct DataBlock
    {
      DataBlock() :
        size(0), ptr(0), remoteHost()
      {};
      
      size_t size;
      uintptr_t ptr;
      Endpoint::RemoteEndpoint remoteHost;
    };
  
    /** Incoming data queue */
    List<DataBlock*> m_DataQueue;
    
    /** Data queue size */
    Semaphore m_DataQueueSize;
    
    /** Accept any address? */
    bool m_bAcceptAll;

    /** Our type */
    Type m_Type;
};

/**
 * The Pedigree network stack - RAW Protocol Manager
 */
class RawManager
{
public:
  RawManager() :
    m_Endpoints()
  {};
  virtual ~RawManager()
  {};
  
  /** For access to the manager without declaring an instance of it */
  static RawManager& instance()
  {
    return manager;
  }
  
  /** Gets a new Endpoint */
  Endpoint* getEndpoint(int proto); //IpAddress remoteHost, uint16_t localPort, uint16_t remotePort);
  
  /** Returns an Endpoint */
  void returnEndpoint(Endpoint* e);
  
  /** A new packet has arrived! */
  void receive(uintptr_t payload, size_t payloadSize, Endpoint::RemoteEndpoint* remoteHost, int proto, Network* pCard);

private:

  static RawManager manager;

  /** Currently known endpoints (all actually RawEndpoints - each one is passed incoming packets). */
  List<Endpoint*> m_Endpoints;
};

#endif
