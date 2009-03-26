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
#ifndef TCPENDPOINT_H
#define TCPENDPOINT_H

#include <processor/types.h>
#include <utilities/List.h>
#include <process/Semaphore.h>
#include <machine/Network.h>

#include <Log.h>

#include "Endpoint.h"
#include "TcpMisc.h"

/**
 * The Pedigree network stack - TCP Endpoint
 * \todo This needs to keep track of a LOCAL IP as well
 *        so we can actually bind properly if there's multiple
 *        cards with different configurations.
 */
class TcpEndpoint : public Endpoint
{
  public:
  
    /** Constructors and destructors */
    TcpEndpoint() :
      Endpoint(), m_Card(0), m_ConnId(0), m_RemoteHost(), m_DataStream(),
      nBytesRemoved(0), m_ShadowDataStream(), m_Listening(false), m_IncomingConnections(), m_IncomingConnectionCount(0)
    {};
    TcpEndpoint(uint16_t local, uint16_t remote) :
      Endpoint(local, remote), m_Card(0), m_ConnId(0), m_RemoteHost(), m_DataStream(),
      nBytesRemoved(0), m_ShadowDataStream(), m_Listening(false), m_IncomingConnections(), m_IncomingConnectionCount(0)
    {};
    TcpEndpoint(IpAddress remoteIp, uint16_t local = 0, uint16_t remote = 0) :
      Endpoint(remoteIp, local, remote), m_Card(0), m_ConnId(0), m_RemoteHost(), m_DataStream(),
      nBytesRemoved(0), m_ShadowDataStream(), m_Listening(false), m_IncomingConnections(), m_IncomingConnectionCount(0)
    {};
    TcpEndpoint(size_t connId, IpAddress remoteIp, uint16_t local = 0, uint16_t remote = 0) :
      Endpoint(remoteIp, local, remote), m_Card(0), m_ConnId(connId), m_RemoteHost(), m_DataStream(),
      nBytesRemoved(0), m_ShadowDataStream(), m_Listening(false), m_IncomingConnections(), m_IncomingConnectionCount(0)
    {};
    virtual ~TcpEndpoint() {};
    
    /** Application interface */
    virtual int send(size_t nBytes, uintptr_t buffer);
    virtual int recv(uintptr_t buffer, size_t maxSize, bool bBlock = false, bool bPeek = false);
    virtual bool dataReady(bool block = false, uint32_t tmout = 30);
    
    virtual bool connect(Endpoint::RemoteEndpoint remoteHost);
    virtual void close();
    
    virtual Endpoint* accept();
    virtual void listen();
    
    virtual void setRemoteHost(Endpoint::RemoteEndpoint host)
    {
      m_RemoteHost = host;
    }
    
    virtual inline uint32_t getConnId()
    {
      return m_ConnId;
    }
    
    /** TcpManager functionality - called to deposit data into our local buffer */
    virtual void depositPayload(size_t nBytes, uintptr_t payload, uint32_t sequenceNumber, bool push);
    
    /** Setters */
    void setCard(Network* pCard)
    {
      m_Card = pCard;
    }
    
    void addIncomingConnection(TcpEndpoint* conn)
    {
      if(conn)
      {
        m_IncomingConnections.pushBack(static_cast<Endpoint*>(conn));
        m_IncomingConnectionCount.release();
      }
    }
  
  private:
  
    /** Copy constructors */
    TcpEndpoint(const TcpEndpoint& s) :
      Endpoint(), m_Card(0), m_ConnId(0), m_RemoteHost(), m_DataStream(),
      nBytesRemoved(0), m_ShadowDataStream(), m_Listening(false), m_IncomingConnections(), m_IncomingConnectionCount(0)
    {
      // shouldn't be called
      ERROR("Tcp: TcpEndpoint copy constructor has been called.");
    }
    TcpEndpoint& operator = (const TcpEndpoint& s)
    {
      // shouldn't be called
      ERROR("Tcp: TcpEndpoint copy constructor has been called.");
      return *this;
    }
    
    /** The network device to use */
    Network* m_Card;
    
    /** TcpManager connection ID */
    size_t m_ConnId;
    
    /** The host we're connected to at the moment */
    RemoteEndpoint m_RemoteHost;
  
    /** The incoming data stream */
    TcpBuffer m_DataStream;
    
    /** Number of bytes we've removed off the front of the (shadow) data stream */
    size_t nBytesRemoved;
    
    /** Shadow incoming data stream - actually receives the bytes from the stack until PUSH flag is set
      * or the buffer fills up, or the connection starts closing.
      */
    TcpBuffer m_ShadowDataStream;
  
    /** Listen endpoint? */
    bool m_Listening;
    
    /** Incoming connection queue (to be handled by accept) */
    List<Endpoint*> m_IncomingConnections;
    Semaphore m_IncomingConnectionCount;
};

#endif
