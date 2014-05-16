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

#ifndef MACHINE_TCPMANAGER_H
#define MACHINE_TCPMANAGER_H

#include <utilities/String.h>
#include <utilities/Tree.h>
#include <utilities/List.h>
#include <processor/types.h>
#include <process/Semaphore.h>
#include <machine/Network.h>
#include <process/Mutex.h>
#include <machine/TimerHandler.h>
#include <LockGuard.h>

#include <Log.h>

#include "Manager.h"

#include "NetworkStack.h"
#include "Tcp.h"
#include "TcpMisc.h"
#include "Endpoint.h"
#include "TcpEndpoint.h"
#include "TcpStateBlock.h"

/**
 * The Pedigree network stack - TCP Protocol Manager
 */
class TcpManager : public ProtocolManager, public TimerHandler
{
public:
  TcpManager() :
    m_NextTcpSequence(1), m_NextConnId(1), m_StateBlocks(), m_ListeningStateBlocks(),
    m_CurrentConnections(), m_Endpoints(), m_PortsAvailable(), m_TcpMutex(false),
    m_SequenceMutex(false), m_Nanoseconds(0)
  {
    // First 1024 ports are not usable for client -> server connections.
    for(size_t n = 0; n < 1024; ++n)
    {
      m_PortsAvailable.set(n);
    }

    Timer *t = Machine::instance().getTimer();
    if(t)
    {
      t->registerHandler(this);
    }
  };
  virtual ~TcpManager()
  {};

  /** For access to the manager without declaring an instance of it */
  static TcpManager& instance()
  {
    return manager;
  }

  /** Every half a second, increments sequence number by 64,000. */
  virtual void timer(uint64_t delta, InterruptState &state)
  {
    m_Nanoseconds += delta;
    if(UNLIKELY(m_Nanoseconds > 200000000ULL))
    {
      // 200 ms tick - check for segments that we need to ack.
    }

    if(UNLIKELY(m_Nanoseconds > 500000000ULL))
    {
      // 500 ms tick - increment sequence number and reset tick counter.
      m_Nanoseconds = 0;

      LockGuard<Mutex> guard(m_SequenceMutex);
      m_NextTcpSequence += 64000;
    }
  }

  /** Connects to a remote host (blocks until connected) */
  size_t Connect(Endpoint::RemoteEndpoint remoteHost, uint16_t localPort, TcpEndpoint* endpoint, bool bBlock = true);

  /** Starts listening for connections */
  size_t Listen(Endpoint* e, uint16_t port, Network* pCard = 0);

  /** In TCP terms - sends FIN. */
  void Shutdown(size_t connectionId, bool bOnlyStopReceive = false);

  /** Disconnects from a remote host (blocks until disconnected). Totally tears down the connection, don't
   *  call unless you absolutely must! Use Shutdown to begin a standard disconnect without blocking.
   */
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
    LockGuard<Mutex> guard(m_TcpMutex);

    StateBlockHandle* handle;
    if((handle = m_CurrentConnections.lookup(connId)) == 0)
    {
      WARNING("getState couldn't find a connection for ID " << connId);
      return Tcp::UNKNOWN;
    }

    StateBlock* stateBlock;
    if((stateBlock = m_ListeningStateBlocks.lookup(*handle)) == 0)
    {
      if((stateBlock = m_StateBlocks.lookup(*handle)) == 0)
      {
        WARNING("getState couldn't find a state block for ID " << connId);
        return Tcp::UNKNOWN;
      }
    }

    return stateBlock->currentState;
  }

  /** Gets the next sequence number to use */
  uint32_t getNextSequenceNumber()
  {
    LockGuard<Mutex> guard(m_SequenceMutex);

    /// \todo This needs to be randomised to avoid sequence attacks
    size_t retSeq = m_NextTcpSequence;
    m_NextTcpSequence += 64000;
    return retSeq;
  }

  /** Gets a unique connection ID */
  size_t getConnId()
  {
    /// \todo Need recursive mutexes!

    size_t ret = m_NextConnId;
    while(m_CurrentConnections.lookup(ret) != 0) // ensure it's unique
      ret++;
    m_NextConnId = ret + 1;
    return ret;
  }

  /** Grabs the number of packets that have been queued for a given connection */
  uint32_t getNumQueuedPackets(size_t connId)
  {
    LockGuard<Mutex> guard(m_TcpMutex);

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
    LockGuard<Mutex> guard(m_TcpMutex);

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
    LockGuard<Mutex> guard(m_TcpMutex);

    size_t bit = m_PortsAvailable.getFirstClear();
    if(bit > 0xFFFF)
    {
      WARNING("No ports available!");
      return 0;
    }
    m_PortsAvailable.set(bit);

    return bit;
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
  ExtensibleBitmap m_PortsAvailable;

  /** Lock to control access to state blocks. */
  Mutex m_TcpMutex;

  /**
   * Lock to control access to the sequence number, as it increments every
   * half a second.
   */
  Mutex m_SequenceMutex;

  /** Count of milliseconds, used for timer handler. */
  uint64_t m_Nanoseconds;
};

#endif
