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
#ifndef MACHINE_ENDPOINT_H
#define MACHINE_ENDPOINT_H

#include <utilities/String.h>
#include <utilities/Vector.h>
#include <processor/types.h>
#include <machine/Network.h>

/**
 * The Pedigree network stack - Protocol Endpoint
 */
class Endpoint
{
  public:
  
    /** Constructors and destructors */
    Endpoint() :
      m_LocalPort(0), m_RemotePort(0), m_RemoteIp()
    {};
    Endpoint(uint16_t local, uint16_t remote) :
      m_LocalPort(local), m_RemotePort(remote), m_RemoteIp()
    {};
    Endpoint(IpAddress remoteIp, uint16_t local = 0, uint16_t remote = 0) :
      m_LocalPort(local), m_RemotePort(remote), m_RemoteIp(remoteIp)
    {};
    virtual ~Endpoint() {};
    
    /** Access to internal information */
    uint16_t getLocalPort()
    {
      return m_LocalPort;
    }
    uint16_t getRemotePort()
    {
      return m_RemotePort;
    }
    IpAddress getRemoteIp()
    {
      return m_RemoteIp;
    }
    
    void setLocalPort(uint16_t port)
    {
      m_LocalPort = port;
    }
    void setRemotePort(uint16_t port)
    {
      m_RemotePort = port;
    }
    void setRemoteIp(IpAddress remote)
    {
      m_RemoteIp = remote;
    }
    
    /** Special address type, like stationInfo but with port info too */
    struct RemoteEndpoint
    {
      RemoteEndpoint() :
        ip(), remotePort(0)
      {};
      
      IpAddress ip; // either IPv4 or IPv6
      uint16_t remotePort;
    };

    /** What state is the endpoint in? Only really relevant for connection-based sockets I guess */
    virtual int state()
    {
      return -1;
    }
    
    /** Is data ready to recv yet? */
    virtual bool dataReady(bool block = false, uint32_t timeout = 30)
    {
      return false;
    };
    
    /** <Protocol>Manager functionality */
    virtual void depositPayload(size_t nBytes, uintptr_t payload, RemoteEndpoint remoteHost)
    {
    }
    
    /** Connectionless endpoints */
    virtual int send(size_t nBytes, uintptr_t buffer, RemoteEndpoint remoteHost, bool broadcast, Network* pCard)
    {
      return -1;
    };
    virtual int recv(uintptr_t buffer, size_t maxSize, RemoteEndpoint* remoteHost)
    {
      return -1;
    };
    virtual inline bool acceptAnyAddress() { return false; };
    virtual inline void acceptAnyAddress(bool accept) {};
    
    /** Connection-based endpoints */
    virtual bool connect(Endpoint::RemoteEndpoint remoteHost, bool bBlock = true)
    {
      return false;
    };
    virtual void close()
    {
    };
    
    virtual void listen()
    {
    };
    virtual Endpoint* accept()
    {
      return 0;
    };
    
    virtual int send(size_t nBytes, uintptr_t buffer)
    {
      return -1;
    };
    virtual int recv(uintptr_t buffer, size_t maxSize, bool block, bool bPeek)
    {
      return -1;
    };
    
    virtual inline uint32_t getConnId()
    {
      return 0;
    }
    
    virtual void setRemoteHost(RemoteEndpoint host)
    {
    };
    
    virtual void setCard(Network* pCard)
    {
    };
  
  private:
  
    /** Our local port (sourcePort in the UDP header) */
    uint16_t m_LocalPort;
    
    /** Our destination port */
    uint16_t m_RemotePort;
    
    /** Remote IP */
    IpAddress m_RemoteIp;
};

#endif
