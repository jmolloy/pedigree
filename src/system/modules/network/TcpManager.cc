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

#include "TcpManager.h"
#include <Log.h>

TcpManager TcpManager::manager;

size_t TcpManager::Listen(Endpoint* e, uint16_t port, Network* pCard)
{
  if(!e || !pCard || !port)
    return 0;
  
  StateBlockHandle* handle = new StateBlockHandle;
  if(!handle)
    return 0;
  handle->localPort = port;
  handle->remotePort = 0;
  handle->remoteHost.ip.setIp(static_cast<uint32_t>(0));
  handle->listen = true;
  StateBlock* stateBlock;
  if((stateBlock = m_StateBlocks.lookup(*handle)) != 0)
  {
    delete handle;
    return 0;
  }
  
  // build a state block for it
  size_t connId = getConnId();
  
  stateBlock = new StateBlock;
  if(!stateBlock)
    return 0;
  
  stateBlock->localPort = port;
  stateBlock->remoteHost = handle->remoteHost;
  
  stateBlock->connId = connId;
  
  stateBlock->currentState = Tcp::LISTEN;
  
  stateBlock->pCard = pCard;
  
  stateBlock->endpoint = static_cast<TcpEndpoint*>(e);
  
  stateBlock->numEndpointPackets = 0;
  
  m_ListeningStateBlocks.insert(*handle, stateBlock);
  m_CurrentConnections.insert(connId, handle);
  
  return connId;
}

size_t TcpManager::Connect(Endpoint::RemoteEndpoint remoteHost, uint16_t localPort, TcpEndpoint* endpoint, Network* pCard)
{
  if(!endpoint || !pCard)
    return 0;
  
  StateBlockHandle* handle = new StateBlockHandle;
  if(!handle)
    return 0;
  
  handle->localPort = localPort;
  handle->remotePort = remoteHost.remotePort;
  handle->remoteHost = remoteHost;
  handle->listen = false;
  StateBlock* stateBlock;
  if((stateBlock = m_StateBlocks.lookup(*handle)) != 0)
  {
    delete handle;
    return 0;
  }
  
  // build a state block for it
  size_t connId = getConnId();
  
  stateBlock = new StateBlock;
  if(!stateBlock)
  {
    delete handle;
    return 0;
  }
  
  stateBlock->localPort = localPort;
  stateBlock->remoteHost = remoteHost;
  
  stateBlock->connId = connId;
  
  stateBlock->iss = getNextSequenceNumber();
  stateBlock->snd_nxt = stateBlock->iss + 1;
  stateBlock->snd_una = stateBlock->iss;
  stateBlock->snd_wnd = 16384;
  stateBlock->snd_up = 0;
  stateBlock->snd_wl1 = stateBlock->snd_wl2 = 0;
  
  stateBlock->currentState = Tcp::SYN_SENT;
  
  stateBlock->pCard = pCard;
  
  stateBlock->endpoint = endpoint;
  
  stateBlock->numEndpointPackets = 0;
  
  m_StateBlocks.insert(*handle, stateBlock);
  m_CurrentConnections.insert(connId, handle);
  
  Tcp::send(stateBlock->remoteHost.ip, stateBlock->localPort, stateBlock->remoteHost.remotePort, stateBlock->iss, 0, Tcp::SYN, stateBlock->snd_wnd, 0, 0, pCard);
  
  bool timedOut = false;
  Timer* t = Machine::instance().getTimer();
  NetworkBlockTimeout* timeout = new NetworkBlockTimeout;
  if(timeout)
  {
    timeout->setSemaphore(&(stateBlock->waitState));
    timeout->setTimedOut(&timedOut);
    if(t)
      t->registerHandler(timeout);
    stateBlock->waitState.acquire();
    if(t)
      t->unregisterHandler(timeout);
    delete timeout;
  }
  
  if((stateBlock->currentState != Tcp::ESTABLISHED) || timedOut)
    return 0; /// \todo Keep track of an error number somewhere in StateBlock
  else
    return connId;
}

void TcpManager::Disconnect(size_t connectionId)
{
  StateBlockHandle* handle;
  if((handle = m_CurrentConnections.lookup(connectionId)) == 0)
    return;
  
  StateBlock* stateBlock;
  if((stateBlock = m_StateBlocks.lookup(*handle)) == 0)
    return;
  
  IpAddress dest;
  dest = stateBlock->remoteHost.ip;
  
  // this is our FIN
  stateBlock->fin_seq = stateBlock->snd_nxt++;
  
  // no FIN received yet
  if(stateBlock->currentState == Tcp::ESTABLISHED)
  {
    stateBlock->currentState = Tcp::FIN_WAIT_1;
    Tcp::send(dest, stateBlock->localPort, stateBlock->remoteHost.remotePort, stateBlock->fin_seq, stateBlock->rcv_nxt, Tcp::FIN | Tcp::ACK, stateBlock->snd_wnd, 0, 0, stateBlock->pCard);
  }
  // received a FIN already
  else if(stateBlock->currentState == Tcp::CLOSE_WAIT)
  {
    stateBlock->currentState = Tcp::LAST_ACK;
    Tcp::send(dest, stateBlock->localPort, stateBlock->remoteHost.remotePort, stateBlock->fin_seq, stateBlock->rcv_nxt, Tcp::FIN | Tcp::ACK, stateBlock->snd_wnd, 0, 0, stateBlock->pCard);
  }
  // LISTEN socket closing
  else if(stateBlock->currentState == Tcp::LISTEN)
  {
    NOTICE("Disconnect called on a LISTEN socket\n");
    stateBlock->currentState = Tcp::CLOSED;
    removeConn(stateBlock->connId);
  }
  else
  {
    NOTICE("Connection Id " << Dec << connectionId << Hex << " is trying to close but not valid state!");
  }
}

void TcpManager::send(size_t connId, uintptr_t payload, bool push, size_t nBytes, bool addToRetransmitQueue)
{
  if(!payload || !nBytes)
    return;
  
  StateBlockHandle* handle;
  if((handle = m_CurrentConnections.lookup(connId)) == 0)
    return;
  
  StateBlock* stateBlock;
  if((stateBlock = m_StateBlocks.lookup(*handle)) == 0)
    return;
  
  // first things first, we need to only send 1024 bytes at a time (just to be weird) max
  size_t offset;
  for(offset = 0; offset < nBytes; offset += 1024)
  {
    size_t segmentSize = 0;
    if((offset + 1024) >= nBytes)
      segmentSize = nBytes - offset;
    
    stateBlock->seg_seq = stateBlock->snd_nxt;
    stateBlock->snd_nxt += segmentSize;
    
    IpAddress dest;
    dest = stateBlock->remoteHost.ip;
    
    Tcp::send(dest, stateBlock->localPort, stateBlock->remoteHost.remotePort, stateBlock->seg_seq, stateBlock->rcv_nxt, Tcp::ACK | (push ? Tcp::PSH : 0), stateBlock->snd_wnd, segmentSize, payload + offset, stateBlock->pCard);
  }
  
  // throw this block onto the retransmission queue (if we should, because this send might be sending unack'd data that's in the retransmit queue)
  if(addToRetransmitQueue)
  {
    NOTICE("Adding to the retransmit queue...");
    //stateBlock->retransmitQueue.append(payload, nBytes);
    //stateBlock->resetTimer();
    //stateBlock->waitingForTimeout = true;
  }
}

void TcpManager::removeConn(size_t connId)
{
  return;
  NOTICE("Tcp: Removing connection " << connId);
  StateBlockHandle* handle;
  if((handle = m_CurrentConnections.lookup(connId)) == 0)
    return;
  
  StateBlock* stateBlock;
  if(handle->listen)
    stateBlock = m_ListeningStateBlocks.lookup(*handle);
  else
    stateBlock = m_StateBlocks.lookup(*handle);
  if(stateBlock == 0)
    return;
  
  // only remove closed connections!
  NOTICE("Current state of connection " << connId << " is " << Tcp::stateString(stateBlock->currentState) << "...");
  if(stateBlock->currentState != Tcp::CLOSED)
  {
    return;
  }
    
  // remove from the lists
  NOTICE("Handle: " << Dec << handle->localPort << ", " << handle->remotePort << ", " << Hex << handle->remoteHost.ip.getIp() << ".");
  if(handle->listen)
    m_ListeningStateBlocks.remove(*handle);
  else
    m_StateBlocks.remove(*handle);
  m_CurrentConnections.remove(connId);
  
  NOTICE("Win");
  
  // destroy the state block (and its internals)
  stateBlock->waitState.release();
  delete stateBlock;
  
  NOTICE("Returning now");
  
  // stateBlock->endpoint is what applications are using right now, so
  // we can't really delete it yet. They will do that with returnEndpoint().
}

void TcpManager::returnEndpoint(Endpoint* e)
{
  if(e)
  {
    // remove from the endpoint list
    //m_Endpoints.remove(e->getConnId());
    
    // if we can (state == CLOSED) remove the connection itself
    // if the state is TIME_WAIT this will be done by the TIME_WAIT timeout
    removeConn(e->getConnId());
    
    // clean up the memory
    delete e;
  }
}

Endpoint* TcpManager::getEndpoint(uint16_t localPort, Network* pCard)
{
  Endpoint* e;
  
  // this will fail for servers! we need a unique identifier for all TcpEndpoints - shouldn't be
  // overly difficult to implement (but to lookup... that might be hard?)
  //if((e = m_Endpoints.lookup(localPort)) == 0)
  //{
    if(localPort == 0)
      localPort = allocatePort();
    
    TcpEndpoint* tmp = new TcpEndpoint(localPort, 0);
    if(!tmp)
      return 0;
    
    tmp->setCard(pCard);
    
    e = static_cast<Endpoint*>(tmp);
    //m_Endpoints.insert(localPort, e);
  //}
  return e;
}
