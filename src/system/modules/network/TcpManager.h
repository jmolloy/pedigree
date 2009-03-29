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
#include "Tcp.h"
#include "TcpMisc.h"
#include "Endpoint.h"
#include "TcpEndpoint.h"
#include "TcpStateBlock.h"

/**
 * The Pedigree network stack - TCP Protocol Manager
 */
class TcpManager
{
public:
  TcpManager() :
    m_NextTcpSequence(0), m_NextConnId(1), m_StateBlocks(), m_ListeningStateBlocks(), m_CurrentConnections(), m_Endpoints(), m_PortsAvailable()
  {};
  virtual ~TcpManager()
  {};
  
  /** For access to the manager without declaring an instance of it */
  static TcpManager& instance()
  {
    return manager;
  }
  
  /** Connects to a remote host (blocks until connected) */
  size_t Connect(Endpoint::RemoteEndpoint remoteHost, uint16_t localPort, TcpEndpoint* endpoint, bool bBlock = true, Network* pCard = 0);
  
  /** Starts listening for connections */
  size_t Listen(Endpoint* e, uint16_t port, Network* pCard = 0);
  
  /** Disconnects from a remote host (blocks until disconnected) */
  void Disconnect(size_t connectionId);
  
  /** Gets a new Endpoint for a connection */
  Endpoint* getEndpoint(uint16_t localPort = 0, Network* pCard = 0);
  
  /** Returns an Endpoint */
  void returnEndpoint(Endpoint* e);
  
  /** A new packet has arrived! */
  void receive(IpAddress from, uint16_t sourcePort, uint16_t destPort, Tcp::tcpHeader* header, uintptr_t payload, size_t payloadSize, Network* pCard);
  
  /** Sends a TCP packet over the given connection ID */
  int send(size_t connId, uintptr_t payload, bool push, size_t nBytes, bool addToRetransmitQueue = true);
  
  /** Removes a given (closed) connection from the system */
  void removeConn(size_t connId);
  
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
    static uint16_t lastPort = 32768;
    return lastPort++;
    
    // default behaviour: start at 32768
    /// \todo Meant to be randomised, and this isn't ideal
    size_t i;
    for(i = 32768; i <= 0xFFFF; i++)
    {
      bool* used;
      if(m_PortsAvailable.lookup(i) == 0)
      {
        used = new bool;
        if(used)
        {
          *used = true;
          m_PortsAvailable.insert(i, used);
          return static_cast<uint16_t>(i);
        }
      }
      used = m_PortsAvailable.lookup(i);
      if(used)
      {
        if(!*used)
        {
          *used = true;
          return static_cast<uint16_t>(i);
        }
      }
    }
    NOTICE("No ports - i = " << Dec << i << Hex << "!");
    return 0; // no ports :(
  }

private:

  static TcpManager manager;
  
  // next TCP sequence number to allocate
  uint32_t m_NextTcpSequence;
  
  // this keeps track of the next valid connection ID
  size_t m_NextConnId;
  
  // standard state blocks
  Tree<StateBlockHandle, StateBlock*> m_StateBlocks;
  
  // server state blocks (separated from standard blocks in the list)
  Tree<StateBlockHandle, StateBlock*> m_ListeningStateBlocks;
  
  /** Current connections - basically a map between connection IDs and handles */
  Tree<size_t, StateBlockHandle*> m_CurrentConnections;

  /** Currently known endpoints (all actually TcpEndpoints). */
  Tree<size_t, Endpoint*> m_Endpoints;
  
  /** Port availability */
  Tree<size_t, bool*> m_PortsAvailable;
};

#endif
