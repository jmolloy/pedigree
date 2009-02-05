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
#ifndef MACHINE_TCPMANAGER_H
#define MACHINE_TCPMANAGER_H

#include <utilities/String.h>
#include <utilities/Tree.h>
#include <utilities/List.h>
#include <processor/types.h>
#include <process/Semaphore.h>
#include <machine/Network.h>

#include <Log.h>

#include "NetworkStack.h"
#include "Endpoint.h"
#include "Tcp.h"
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
      nBytesRemoved(0), m_ShadowDataStream(), m_Listening(false), m_IncomingConnectionCount(0)
    {};
    TcpEndpoint(uint16_t local, uint16_t remote) :
      Endpoint(local, remote), m_Card(0), m_ConnId(0), m_RemoteHost(), m_DataStream(),
      nBytesRemoved(0), m_ShadowDataStream(), m_Listening(false), m_IncomingConnectionCount(0)
    {
      TcpEndpoint();
      Endpoint(local, remote);
    };
    TcpEndpoint(stationInfo remoteInfo, uint16_t local = 0, uint16_t remote = 0) :
      Endpoint(remoteInfo, local, remote), m_Card(0), m_ConnId(0), m_RemoteHost(), m_DataStream(),
      nBytesRemoved(0), m_ShadowDataStream(), m_Listening(false), m_IncomingConnectionCount(0)
    {
      TcpEndpoint();
      Endpoint(remoteInfo, local, remote);
    };
    TcpEndpoint(size_t connId, stationInfo remoteInfo, uint16_t local = 0, uint16_t remote = 0) :
      Endpoint(remoteInfo, local, remote), m_Card(0), m_ConnId(connId), m_RemoteHost(), m_DataStream(),
      nBytesRemoved(0), m_ShadowDataStream(), m_Listening(false), m_IncomingConnectionCount(0)
    {};
    virtual ~TcpEndpoint() {};
    
    /** Application interface */
    virtual bool send(size_t nBytes, uintptr_t buffer);
    virtual size_t recv(uintptr_t buffer, size_t maxSize, bool bBlock = false);
    virtual bool dataReady(bool block = false);
    
    virtual bool connect(Endpoint::RemoteEndpoint remoteHost);
    virtual void close();
    
    virtual Endpoint* accept();
    virtual void listen();
    
    virtual void setRemoteHost(Endpoint::RemoteEndpoint host)
    {
      m_RemoteHost = host;
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
  
    /** Listen endpoint? */
    bool m_Listening;
  
    /** The incoming data stream */
    TcpBuffer m_DataStream;
    
    /** Shadow incoming data stream - actually receives the bytes from the stack until PUSH flag is set */
    TcpBuffer m_ShadowDataStream;
    
    /** Number of bytes we've removed off the front of the (shadow) data stream */
    size_t nBytesRemoved;
    
    /** Incoming connection queue (to be handled by accept) */
    List<Endpoint*> m_IncomingConnections;
    Semaphore m_IncomingConnectionCount;
    
    /** TcpManager connection ID */
    size_t m_ConnId;
    
    /** The network device to use */
    Network* m_Card;
    
    /** The host we're connected to at the moment */
    RemoteEndpoint m_RemoteHost;
};

/**
 * The Pedigree network stack - TCP Protocol Manager
 */
class TcpManager
{
public:
  TcpManager() :
    m_NextTcpSequence(0), m_NextConnId(1)
  {};
  virtual ~TcpManager()
  {};
  
  /** For access to the manager without declaring an instance of it */
  static TcpManager& instance()
  {
    return manager;
  }
  
  /** Connects to a remote host (blocks until connected) */
  size_t Connect(Endpoint::RemoteEndpoint remoteHost, uint16_t localPort, TcpEndpoint* endpoint, Network* pCard = NetworkStack::instance().getDevice(0)); /// \todo Actually, local ports should be allocated
  
  /** Starts listening for connections */
  size_t Listen(Endpoint* e, uint16_t port, Network* pCard = NetworkStack::instance().getDevice(0));
  
  /** Disconnects from a remote host (blocks until disconnected) */
  void Disconnect(size_t connectionId);
  
  /** Gets a new Endpoint for a connection */
  //Endpoint* getEndpoint(Endpoint::RemoteEndpoint remoteHost, uint16_t localPort = 0, Network* pCard = NetworkStack::instance().getDevice(0));
  Endpoint* getEndpoint(uint16_t localPort = 0, Network* pCard = NetworkStack::instance().getDevice(0));
  
  /** Returns an Endpoint */
  void returnEndpoint(Endpoint* e);
  
  /** A new packet has arrived! */
  void receive(stationInfo from, uint16_t sourcePort, uint16_t destPort, Tcp::tcpHeader* header, uintptr_t payload, size_t payloadSize, Network* pCard);
  
  /** Sends a TCP packet over the given connection ID */
  void send(size_t connId, uintptr_t payload, bool push, size_t nBytes);
  
  /** Grabs the current state of a given connection */
  Tcp::TcpState getState(size_t connId)
  {
    StateBlockHandle* handle;
    if((handle = m_CurrentConnections.lookup(connId)) == 0)
      return Tcp::UNKNOWN;
    
    StateBlock* stateBlock;
    if((stateBlock = m_StateBlocks.lookup(*handle)) == 0)
      return Tcp::UNKNOWN;
    
    return stateBlock->currentState;
  }
  
  /** Gets the next sequence number to use */
  uint32_t getNextSequenceNumber()
  {
    /// \todo These need to be randomised to avoid sequence attacks
    m_NextTcpSequence += 0xffff;
    return m_NextTcpSequence;
  }
  
  /** Gets a unique connection ID */
  size_t getConnId()
  {
    size_t ret = m_NextConnId;
    while(m_CurrentConnections.lookup(ret) != 0) // ensure it's unique
      ret++;
    m_NextConnId = ret + 1;
    return ret;
  }
  
  /** Grabs the number of packets that have been queued for a given connection */
  uint32_t getNumQueuedPackets(size_t connId)
  {
    StateBlockHandle* handle;
    if((handle = m_CurrentConnections.lookup(connId)) == 0)
      return 0;
    
    StateBlock* stateBlock;
    if((stateBlock = m_StateBlocks.lookup(*handle)) == 0)
      return 0;
    
    return stateBlock->numEndpointPackets;
  }
  
  /** Reduces the number of queued packets by the specified amount */
  void removeQueuedPackets(size_t connId, uint32_t n = 1)
  {
    StateBlockHandle* handle;
    if((handle = m_CurrentConnections.lookup(connId)) == 0)
      return;
    
    StateBlock* stateBlock;
    if((stateBlock = m_StateBlocks.lookup(*handle)) == 0)
      return;
    
    stateBlock->numEndpointPackets -= n;
  }
  
  /** Allocates a unique local port for a connection with a server */
  uint16_t allocatePort()
  {
    // default behaviour: start at 32768
    /// \todo Meant to be randomised, and this isn't ideal
    size_t i;
    for(i = 32768; i <= 0xFFFF; i++)
    {
      bool* used;
      if(m_PortsAvailable.lookup(i) == 0)
      {
        used = new bool;
        *used = true;
        m_PortsAvailable.insert(i, used);
        return static_cast<uint16_t>(i);
      }
      used = m_PortsAvailable.lookup(i);
      if(!*used)
      {
        *used = true;
        return static_cast<uint16_t>(i);
      }
    }
    return 0; // no ports :(
  }

private:

  static TcpManager manager;
  
  // this keeps track of the next valid connection ID
  size_t m_NextConnId;
  
  // TCP is based on connections, so we need to keep track of them
  // before we even think about depositing into Endpoints. These state blocks
  // keep track of important information relating to the connection state.
  class StateBlock
  {
    public:
      StateBlock() :
        waitState(0), pCard(0)
      {};
      ~StateBlock()
      {};
      
      Tcp::TcpState currentState;
      
      uint16_t localPort;
      
      Endpoint::RemoteEndpoint remoteHost;
      
      // send sequence variables
      uint32_t iss; // initial sender sequence number (CLIENT)
      uint32_t snd_nxt; // next send sequence number
      uint32_t snd_una; // send unack
      uint32_t snd_wnd; // send window ----> How much they can receive max
      uint32_t snd_up; // urgent pointer?
      uint32_t snd_wl1; // segment sequence number for last WND update
      uint32_t snd_wl2; // segment ack number for last WND update

      // receive sequence variables
      uint32_t rcv_nxt; // receive next - what we're expecting perhaps?
      uint32_t rcv_wnd; // receive window ----> How much we want to receive methinks...
      uint32_t rcv_up; // receive urgent pointer
      uint32_t irs; // initial receiver sequence number (SERVER)

      // segment variables
      uint32_t seg_seq; // segment sequence number
      uint32_t seg_ack; // ack number
      uint32_t seg_len; // segment length
      uint32_t seg_wnd; // segment window
      uint32_t seg_up; // urgent pointer
      uint32_t seg_prc; // precedence
      
      // FIN information
      bool     fin_ack; // is ACK already set (for use with FIN bit checks)
      uint32_t fin_seq; // last FIN we sent had this sequence number
      
      // number of packets we've deposited into our Endpoint
      // (decremented when a packet is picked up by the receiver)
      uint32_t numEndpointPackets;
      
      // waiting for something?
      Semaphore waitState;
      
      // the card which we use for all sending
      Network* pCard;
      
      // the endpoint applications use for this TCP connection
      TcpEndpoint* endpoint;
      
      // the id of this specific connection
      size_t connId;
  };
  
  // next TCP sequence number to allocate
  uint32_t m_NextTcpSequence;
  
/*  static int CompareHandles(void* p1, void* p2)
  {
    StateBlockHandle* one = reinterpret_cast<StateBlockHandle*>(p1);
    StateBlockHandle* two = reinterpret_cast<StateBlockHandle*>(p2);
    if(*one == *two)
      return 0;
    else if(*one > *two)
      return 1;
    else
      return -1;
  }*/
  
  /** This is a lot of fun :D */
  Tree<StateBlockHandle, StateBlock*> m_StateBlocks;
  
  /** Current connections - basically a map between connection IDs and handles */
  Tree<size_t, StateBlockHandle*> m_CurrentConnections;

  /** Currently known endpoints (all actually TcpEndpoints). */
  Tree<size_t, Endpoint*> m_Endpoints;
  
  /** Port availability */
  Tree<size_t, bool*> m_PortsAvailable;
};

#endif
