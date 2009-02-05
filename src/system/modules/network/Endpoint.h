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
      m_LocalPort(0), m_RemotePort(0), m_RemoteInfo()
    {};
    Endpoint(uint16_t local, uint16_t remote) :
      m_LocalPort(local), m_RemotePort(remote), m_RemoteInfo()
    {};
    Endpoint(stationInfo remoteInfo, uint16_t local = 0, uint16_t remote = 0) :
      m_LocalPort(local), m_RemotePort(remote), m_RemoteInfo(remoteInfo)
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
    stationInfo getRemoteInfo()
    {
      return m_RemoteInfo;
    }
    
    void setLocalPort(uint16_t port)
    {
      m_LocalPort = port;
    }
    void setRemotePort(uint16_t port)
    {
      m_RemotePort = port;
    }
    void setRemoteInfo(stationInfo remote)
    {
      m_RemoteInfo = remote;
    }
    
    /** Special address type, like stationInfo but with port info too */
    struct RemoteEndpoint
    {
      uint32_t ipv4;
      uint8_t ipv6[16]; /// \todo IPv4 - AGAIN -_-
      
      uint16_t remotePort;
    };
    
    /** Is data ready to recv yet? */
    virtual bool dataReady(bool block = false)
    {
      return false;
    };
    
    /** <Protocol>Manager functionality */
    virtual void depositPayload(size_t nBytes, uintptr_t payload, RemoteEndpoint remoteHost)
    {
    }
    
    /** Connectionless endpoints */
    virtual bool send(size_t nBytes, uintptr_t buffer, RemoteEndpoint remoteHost, Network* pCard)
    {
      return false;
    };
    virtual size_t recv(uintptr_t buffer, size_t maxSize, RemoteEndpoint* remoteHost)
    {
      return 0;
    };
    
    /** Connection-based endpoints */
    virtual bool connect(Endpoint::RemoteEndpoint remoteHost)
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
    
    virtual bool send(size_t nBytes, uintptr_t buffer)
    {
      return false;
    };
    virtual size_t recv(uintptr_t buffer, size_t maxSize, bool block)
    {
      return 0;
    };
    
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
    
    /** Remote station information */
    stationInfo m_RemoteInfo;
};

#endif
