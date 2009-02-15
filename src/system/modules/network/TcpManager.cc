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

Endpoint* TcpEndpoint::accept()
{
  NOTICE("a");
  m_IncomingConnectionCount.acquire();
  NOTICE("b");
  Endpoint* e = m_IncomingConnections.popFront();
  NOTICE("c");
  return e;
}

void TcpEndpoint::listen()
{
  m_ConnId = TcpManager::instance().Listen(this, getLocalPort());
}

bool TcpEndpoint::connect(Endpoint::RemoteEndpoint remoteHost)
{
  setRemoteHost(remoteHost);
  setRemotePort(remoteHost.remotePort);
  m_ConnId = TcpManager::instance().Connect(m_RemoteHost, getLocalPort(), this, m_Card);
  if(m_ConnId == 0)
    NOTICE("TcpEndpoint::connect: got 0 for the connection id");
  return (m_ConnId != 0); /// \todo Error codes
}

void TcpEndpoint::close()
{
  TcpManager::instance().Disconnect(m_ConnId);
}

bool TcpEndpoint::send(size_t nBytes, uintptr_t buffer)
{
  /// \todo Send needs to return an error code or something, and PUSH needs to be an option
  TcpManager::instance().send(m_ConnId, buffer, true, nBytes);
  return true;
};

size_t TcpEndpoint::recv(uintptr_t buffer, size_t maxSize, bool bBlock)
{
  bool queueReady = false;
  if(bBlock)
    queueReady = dataReady(true);
  else
    queueReady = m_DataStream.getSize() != 0; // m_DataQueue.count() != 0;
  
  if(queueReady)
  {    
    TcpManager::instance().removeQueuedPackets(m_ConnId);
    
    // read off the front
    uintptr_t front = m_DataStream.getBuffer();
    
    // how many bytes to read
    size_t nBytes = maxSize;
    if(nBytes > m_DataStream.getSize())
      nBytes = m_DataStream.getSize();
    
    // copy
    memcpy(reinterpret_cast<void*>(buffer), reinterpret_cast<void*>(front), nBytes);
    
    // remove from the buffer, we've read
    m_DataStream.remove(0, nBytes);
    // nBytesRemoved += nBytes;
    // above no longer updated here, it's needed for the shadow stream
    
    // we've read in this block
    return nBytes;
  }
  return 0;
};

void TcpEndpoint::depositPayload(size_t nBytes, uintptr_t payload, uint32_t sequenceNumber, bool push)
{
  /// \note Perhaps nBytes should also have an upper limit check?
  if(!nBytes || !payload)
  {
    NOTICE("Dud arguments to depositPayload!");
    return;
  }
  
  // here goes
  m_ShadowDataStream.insert(payload, nBytes, sequenceNumber - nBytesRemoved, false);
  if(push)
  {
    // take all the data OUT of the shadow stream, shove it into the user stream
    size_t shadowSize = m_ShadowDataStream.getSize();
    m_DataStream.append(m_ShadowDataStream.getBuffer(), shadowSize);
    m_ShadowDataStream.remove(0, shadowSize);
    nBytesRemoved += shadowSize;
  }
}

bool TcpEndpoint::dataReady(bool block)
{
  if(block)
  {
    while(true)
    {      
      //if(m_DataQueueSize.tryAcquire())
      //  return true;
      if(m_DataStream.getSize() != 0)
        return true;
      
      // if there's no more data in the stream, and we need to close, do it
      // you'd think the above would handle this, but timing is an awful thing to assume
      // much testing has led to the addition of the stream size check
      if(TcpManager::instance().getState(m_ConnId) > Tcp::FIN_WAIT_2 && (m_DataStream.getSize() == 0))
      {
        NOTICE("CLOSING with " << m_DataStream.getSize() << " bytes in the stream");
        return false;
      }
      
      // definitely data around?
      //NOTICE("State is " << Tcp::stateString(TcpManager::instance().getState(m_ConnId)) << " and number on queue is " << TcpManager::instance().getNumQueuedPackets(m_ConnId) << ".");
      //if(/*TcpManager::instance().getNumQueuedPackets(m_ConnId) != 0 &&*/ m_DataStream.getSize() != 0) // m_DataQueueSize.tryAcquire())
      //  return true;
      
        
      // if there's no data on the queue, and we're not in ESTABLISHED, we've probably either
      // sent a FIN, or received one.
      // however, segment processing still occurs in FIN_WAIT states, so only check if we're beyond
      // that
      // close wait also means that the remote TCP has requested a close, but we need to actually send
      // our own close
      /*bool yetAgain = m_DataQueueSize.tryAcquire();
      if(TcpManager::instance().getState(m_ConnId) > Tcp::FIN_WAIT_2 && !yetAgain) // && TcpManager::instance().getState(m_ConnId) != Tcp::CLOSE_WAIT)
        return false;
      else if(yetAgain)
        return true;*/
    }
  }
  else
  {
    return (m_DataStream.getSize() != 0); //m_DataQueueSize.tryAcquire();
  }
};

size_t TcpManager::Listen(Endpoint* e, uint16_t port, Network* pCard)
{
  // build a state block for it
  size_t connId = getConnId();
  
  StateBlockHandle* handle = new StateBlockHandle;
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
  
  stateBlock = new StateBlock;
  stateBlock->localPort = port;
  stateBlock->remoteHost = handle->remoteHost;
  
  stateBlock->connId = connId;
  
  /* redundant thanks to constructors
  stateBlock->iss = 0;
  stateBlock->snd_nxt = 0;
  stateBlock->snd_una = 0;
  stateBlock->snd_wnd = 0;
  stateBlock->snd_up = 0;
  stateBlock->snd_wl1 = stateBlock->snd_wl2 = 0;
  
  stateBlock->irs = 0;
  stateBlock->rcv_nxt = 0;
  stateBlock->rcv_wnd = 0;
  stateBlock->rcv_up = 0;
  
  // no segments yet
  stateBlock->seg_seq = 0;
  stateBlock->seg_ack = 0;
  stateBlock->seg_len = 0;
  stateBlock->seg_wnd = 0;
  stateBlock->seg_up = 0;
  stateBlock->seg_prc = 0;
  
  stateBlock->fin_ack = false;
  stateBlock->fin_seq = 0;
  */
  
  stateBlock->currentState = Tcp::LISTEN;
  
  stateBlock->pCard = pCard;
  
  stateBlock->endpoint = static_cast<TcpEndpoint*>(e);
  
  stateBlock->numEndpointPackets = 0;
  
  m_StateBlocks.insert(*handle, stateBlock);
  m_CurrentConnections.insert(connId, handle);
  
  return connId;
}

size_t TcpManager::Connect(Endpoint::RemoteEndpoint remoteHost, uint16_t localPort, TcpEndpoint* endpoint, Network* pCard)
{
  // build a state block for it
  size_t connId = getConnId(); //localPort;
  
  StateBlockHandle* handle = new StateBlockHandle;
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
  
  stateBlock = new StateBlock;
  stateBlock->localPort = localPort;
  stateBlock->remoteHost = remoteHost;
  
  stateBlock->connId = connId;
  
  stateBlock->iss = getNextSequenceNumber();
  stateBlock->snd_nxt = stateBlock->iss + 1;
  stateBlock->snd_una = stateBlock->iss;
  stateBlock->snd_wnd = 16384;
  stateBlock->snd_up = 0;
  stateBlock->snd_wl1 = stateBlock->snd_wl2 = 0;
  
  /* redundant thanks to constructors
  stateBlock->irs = 0;
  stateBlock->rcv_nxt = 0;
  stateBlock->rcv_wnd = 0;
  stateBlock->rcv_up = 0;
  
  // no segments yet
  stateBlock->seg_seq = 0;
  stateBlock->seg_ack = 0;
  stateBlock->seg_len = 0;
  stateBlock->seg_wnd = 0;
  stateBlock->seg_up = 0;
  stateBlock->seg_prc = 0;
  
  stateBlock->fin_ack = false;
  stateBlock->fin_seq = 0;
  */
  
  stateBlock->currentState = Tcp::SYN_SENT;
  
  stateBlock->pCard = pCard;
  
  stateBlock->endpoint = endpoint;
  
  stateBlock->numEndpointPackets = 0;
  
  m_StateBlocks.insert(*handle, stateBlock);
  m_CurrentConnections.insert(connId, handle);
  
  Tcp::send(stateBlock->remoteHost.ip, stateBlock->localPort, stateBlock->remoteHost.remotePort, stateBlock->iss, 0, Tcp::SYN, stateBlock->snd_wnd, 0, 0, pCard);
  
  /// \todo Keep an error state in the stateBlock and break free of this loop
  ///       if an error occurs (such as connection refused)
  //while(stateBlock->currentState != Tcp::ESTABLISHED)
    stateBlock->waitState.acquire();
  
  NOTICE("Connect wait returns, state is " << Tcp::stateString(stateBlock->currentState) << ".");
  if(stateBlock->currentState != Tcp::ESTABLISHED)
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
  stateBlock->fin_seq = stateBlock->snd_nxt;
  
  NOTICE("TcpManager::Disconnect, we're working with " << Dec << stateBlock->localPort << Hex << " as the local port");
  
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
  else
  {
    NOTICE("Connection Id " << Dec << connectionId << Hex << " is trying to close but not valid state!");
  }
  
  /// \todo Keep an error state in the stateBlock and break free of this loop
  ///       if an error occurs (such as connection refused)
  //while(stateBlock->currentState != Tcp::ESTABLISHED)
  //  stateBlock->waitState.acquire();
  
  //if(stateBlock->currentState != Tcp::ESTABLISHED)
  //  return 0; /// \todo Keep track of an error number somewhere in StateBlock
  //else
  //  return connId;
}

void TcpManager::send(size_t connId, uintptr_t payload, bool push, size_t nBytes)
{
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
    
    /// \todo Add to the retransmit queue, so we can... retransmit :/
  }
}

void TcpManager::receive(IpAddress from, uint16_t sourcePort, uint16_t destPort, Tcp::tcpHeader* header, uintptr_t payload, size_t payloadSize, Network* pCard)
{  
  // find the state block if possible, if none exists create one
  StateBlockHandle handle;
  handle.localPort = destPort;
  handle.remotePort = sourcePort;
  handle.remoteHost.ip = from;
  handle.listen = false; // DON'T look for listen sockets yet
  StateBlock* stateBlock;
  if((stateBlock = m_StateBlocks.lookup(handle)) == 0)
  {
    // check for a listen socket
    handle.listen = true;
    if((stateBlock = m_StateBlocks.lookup(handle)) == 0)
    {
      // port doesn't exist, so temporary stateBlock required for proper RST handle
      stateBlock = new StateBlock;
      //m_StateBlocks.insert(handle, stateBlock);
      
      NOTICE("TCP Packet arriving on port " << Dec << handle.localPort << Hex << " has no destination.");
      
      stateBlock->currentState = Tcp::CLOSED;
    }
  }
  
  // fill current segment information
  stateBlock->seg_seq = BIG_TO_HOST32(header->seqnum);
  stateBlock->seg_ack = BIG_TO_HOST32(header->acknum);
  stateBlock->seg_len = payloadSize;
  stateBlock->seg_wnd = BIG_TO_HOST16(header->winsize);
  stateBlock->seg_up = BIG_TO_HOST16(header->urgptr);
  stateBlock->seg_prc = 0; // where the heck does this come from?
  stateBlock->rcv_wnd = stateBlock->seg_wnd;
  
  stateBlock->fin_ack = false;
  
  // has an Ack already been sent in this segment?
  bool alreadyAck = false;
  
  // what state are we in?
  // RFC793, page 65 onwards
  Tcp::TcpState oldState = stateBlock->currentState;
  NOTICE("TCP packet arrived while stateBlock in " << Tcp::stateString(stateBlock->currentState) << " [remote port = " << stateBlock->remoteHost.remotePort << ".");
  switch(stateBlock->currentState)
  {
    /* Incoming segment while the state is CLOSED */
    case Tcp::CLOSED:
    
      // if no RST, we need to send one
      if(!(header->flags & Tcp::RST))
      {
        uint32_t seq = 0;
        uint32_t ack = 0;
        uint16_t window = 0;
        
        uint8_t flags = 0;
        
        if(header->flags & Tcp::ACK)
        {
          seq = BIG_TO_HOST32(header->acknum);
          flags = Tcp::RST;
        }
        else
        {
          ack = BIG_TO_HOST32(header->seqnum) + 1;
          flags = Tcp::RST | Tcp::ACK;
        }
        Tcp::send(from, handle.localPort, handle.remotePort, seq, ack, flags, window, 0, 0, pCard);
      }
      
      //m_StateBlocks.remove(handle);
      delete stateBlock;
      
      return;
      
      break;
    
    /* Incoming segment while the state is LISTEN */
    case Tcp::LISTEN:
    
      if(header->flags & Tcp::RST)
      {
        // RST on a listen state is invalid
      }
      else if(header->flags & Tcp::ACK)
      {
        // an ACK on a listen state is invalid
        Tcp::send(from, handle.localPort, handle.remotePort, stateBlock->seg_ack, 0, Tcp::RST, 0, 0, 0, pCard);
      }
      else if(header->flags & Tcp::SYN)
      {
        /// \todo Server-side stuff
        
        // create a new server state block for this incoming connection, and register the new client
        // connection ID with the listening endpoint
        
        NOTICE("SYN on a LISTEN socket");
        
        size_t connId = getConnId();
  
        StateBlock* newStateBlock = new StateBlock;
        
        // memset(newStateBlock, 0, sizeof(StateBlock));
        
        newStateBlock->connId = connId;
        
        newStateBlock->localPort = destPort;
        newStateBlock->remoteHost.remotePort = sourcePort;
        newStateBlock->remoteHost.ip = from;
        
        newStateBlock->iss = getNextSequenceNumber();
        newStateBlock->snd_nxt = newStateBlock->iss + 1;
        newStateBlock->snd_una = newStateBlock->iss;
        newStateBlock->snd_wnd = 16384;
        newStateBlock->snd_up = 0;
        newStateBlock->snd_wl1 = newStateBlock->snd_wl2 = 0;
        
        newStateBlock->irs = stateBlock->seg_seq;
        newStateBlock->rcv_nxt = stateBlock->seg_seq + 1;
        newStateBlock->rcv_wnd = stateBlock->seg_wnd;
        newStateBlock->rcv_up = 0;
        
        // no segments yet
        newStateBlock->seg_seq = 0;
        newStateBlock->seg_ack = 0;
        newStateBlock->seg_len = 0;
        newStateBlock->seg_wnd = 0;
        newStateBlock->seg_up = 0;
        newStateBlock->seg_prc = 0;
        
        newStateBlock->fin_ack = false;
        newStateBlock->fin_seq = 0;
        
        newStateBlock->currentState = Tcp::SYN_RECEIVED;
        
        newStateBlock->pCard = pCard;
        
        newStateBlock->endpoint = stateBlock->endpoint;
        
        newStateBlock->numEndpointPackets = 0;
        
        handle.listen = false;
        m_StateBlocks.insert(handle, newStateBlock);
        
        StateBlockHandle* tmp = new StateBlockHandle;
        memcpy(tmp, &handle, sizeof(StateBlockHandle));
        m_CurrentConnections.insert(connId, tmp);
        
        // ACK the SYN
        IpAddress dest;
        dest = newStateBlock->remoteHost.ip;
        Tcp::send(dest, newStateBlock->localPort, newStateBlock->remoteHost.remotePort, newStateBlock->iss, newStateBlock->rcv_nxt, Tcp::SYN | Tcp::ACK, newStateBlock->snd_wnd, 0, 0, pCard);
      }
      else
      {
        WARNING("TCP Packet incoming on port " << Dec << handle.localPort << Hex << " during LISTEN without RST, ACK, or SYN set.");
      }
      
      break;
    
    /* Incoming segment while the state is SYN_SENT */
    case Tcp::SYN_SENT:
        
      // ACK verification
      if(header->flags & Tcp::ACK)
      {
        if((stateBlock->seg_ack <= stateBlock->iss) || (stateBlock->seg_ack > stateBlock->snd_nxt))
        {
          NOTICE("TCP Packet arriving on port " << Dec << handle.localPort << Hex << " during SYN-SENT has unacceptable ACK 1.");
          
          // RST required
          if(!(header->flags & Tcp::RST))
          {
            Tcp::send(from, handle.localPort, handle.remotePort, stateBlock->seg_ack, 0, Tcp::RST, 0, 0, 0, pCard);
            break;
          }
        }
        
        if(!((stateBlock->snd_una <= stateBlock->seg_ack) && (stateBlock->seg_ack <= stateBlock->snd_nxt)))
        {
          // ACK unacceptable
          NOTICE("TCP Packet arriving on port " << Dec << handle.localPort << Hex << " during SYN-SENT has unacceptable ACK 2.");
          break;
        }
      }
      
      if(header->flags & Tcp::RST)
      {
        if(header->flags & Tcp::ACK)
        {
          // drop segment, we're closing NOW!
          stateBlock->currentState = Tcp::CLOSED;
          break;
        }
      }
      
      if(header->flags & Tcp::SYN)
      {
        if(header->flags & Tcp::ACK)
        {
          stateBlock->rcv_nxt = stateBlock->seg_seq + 1;
          stateBlock->irs = stateBlock->seg_seq;
          stateBlock->snd_una = stateBlock->seg_ack;
          
          if(stateBlock->snd_una > stateBlock->iss)
          {
            stateBlock->currentState = Tcp::ESTABLISHED;
            
            Tcp::send(from, handle.localPort, handle.remotePort, stateBlock->snd_nxt, stateBlock->rcv_nxt, Tcp::ACK, stateBlock->snd_wnd, 0, 0, pCard);
            
            break;
          }
          else
          {
            stateBlock->currentState = Tcp::SYN_RECEIVED;
            
            Tcp::send(from, handle.localPort, handle.remotePort, stateBlock->iss, stateBlock->rcv_nxt, Tcp::SYN | Tcp::ACK, stateBlock->snd_wnd, 0, 0, pCard);
            
            break;
          }
        }
      }
      
      break;
      
    /* These all have the same style of handling (with special handling as needed) */
    case Tcp::SYN_RECEIVED:
    case Tcp::ESTABLISHED:
    case Tcp::FIN_WAIT_1:
    case Tcp::FIN_WAIT_2:
    case Tcp::CLOSE_WAIT:
    case Tcp::CLOSING:
    case Tcp::LAST_ACK:
    case Tcp::TIME_WAIT:
    
      if(stateBlock->seg_len == 0 && stateBlock->rcv_wnd == 0)
      {
        // unacceptable
        if(!(stateBlock->seg_seq == stateBlock->rcv_nxt))
        {
          NOTICE("TCP Packet arriving on port " << Dec << handle.localPort << Hex << " during " << Tcp::stateString(stateBlock->currentState) << " is unacceptable 1.");
          Tcp::send(from, handle.localPort, handle.remotePort, stateBlock->snd_nxt, stateBlock->rcv_nxt, Tcp::ACK, stateBlock->snd_wnd, 0, 0, pCard);
          break;
        }
      }
      
      if(stateBlock->seg_len == 0 && stateBlock->rcv_wnd > 0)
      {
        // unacceptable
        if(!(stateBlock->rcv_nxt <= stateBlock->seg_seq && stateBlock->seg_seq < (stateBlock->rcv_nxt + stateBlock->rcv_wnd)))
        {
          NOTICE("TCP Packet arriving on port " << Dec << handle.localPort << Hex << " during " << Tcp::stateString(stateBlock->currentState) << " is unacceptable 2.");
          Tcp::send(from, handle.localPort, handle.remotePort, stateBlock->snd_nxt, stateBlock->rcv_nxt, Tcp::ACK, stateBlock->snd_wnd, 0, 0, pCard);
          break;
        }
      }
      
      // unacceptable
      if(stateBlock->seg_len > 0 && stateBlock->rcv_wnd == 0)
      {
        NOTICE("TCP Packet arriving on port " << Dec << handle.localPort << Hex << " during " << Tcp::stateString(stateBlock->currentState) << " is unacceptable 3.");
        Tcp::send(from, handle.localPort, handle.remotePort, stateBlock->snd_nxt, stateBlock->rcv_nxt, Tcp::ACK, stateBlock->snd_wnd, 0, 0, pCard);
        break;
      }
      
      if(stateBlock->seg_len > 0 && stateBlock->rcv_wnd > 0)
      {
        if(!(
          (stateBlock->rcv_nxt <= stateBlock->seg_seq && stateBlock->seg_seq < (stateBlock->rcv_nxt + stateBlock->rcv_wnd))
          ||
          (stateBlock->rcv_nxt <= (stateBlock->seg_seq + stateBlock->seg_len - 1) && (stateBlock->seg_seq + stateBlock->seg_len - 1) < (stateBlock->rcv_nxt + stateBlock->rcv_wnd))))
        {
          NOTICE("TCP Packet arriving on port " << Dec << handle.localPort << Hex << " during " << Tcp::stateString(stateBlock->currentState) << " is unacceptable 4.");
          Tcp::send(from, handle.localPort, handle.remotePort, stateBlock->snd_nxt, stateBlock->rcv_nxt, Tcp::ACK, stateBlock->snd_wnd, 0, 0, pCard);
          break;
        }
      }
      
      if(header->flags & Tcp::RST)
      {
        switch(stateBlock->currentState)
        {
          case Tcp::SYN_RECEIVED:
            // passive open - keep track of what we were in FIRST
            // we'll need to return to listen if so
            
            // active open, we're closed and outta here
            break;
          
          case Tcp::ESTABLISHED:
          case Tcp::FIN_WAIT_1:
          case Tcp::FIN_WAIT_2:
          case Tcp::CLOSE_WAIT:
          
            // blow away segment queues, receive/send exit with error
            break;
            
          case Tcp::CLOSING:
          case Tcp::LAST_ACK:
          case Tcp::TIME_WAIT:
          
            // close the connection
            break;
          
          default:
            break;
        }
        
        stateBlock->currentState = Tcp::CLOSED;
        break;
      }
      
      // check security and precedence...
      
      if(header->flags & Tcp::SYN)
      {
        // reset..
        NOTICE("TCP: unexpected SYN!");
        break;
      }
      
      if(header->flags & Tcp::ACK)
      {
        switch(stateBlock->currentState)
        {
          case Tcp::SYN_RECEIVED:
          {
            if(!(stateBlock->snd_una <= stateBlock->seg_ack && stateBlock->seg_ack <= stateBlock->snd_nxt))
            {
              NOTICE("TCP Packet arriving on port " << Dec << handle.localPort << Hex << " during " << Tcp::stateString(stateBlock->currentState) << " is an unacceptable segment ACK.");
              Tcp::send(from, handle.localPort, handle.remotePort, stateBlock->seg_ack, 0, Tcp::RST, 0, 0, 0, pCard);
              break;
            }
            
            size_t connId = stateBlock->connId;
            TcpEndpoint* parent = stateBlock->endpoint;
            stateBlock->endpoint = new TcpEndpoint(connId, from, stateBlock->localPort, stateBlock->remoteHost.remotePort);
            //TcpEndpoint(size_t connId, stationInfo remoteInfo, uint16_t local = 0, uint16_t remote = 0)
            
            //new TcpEndpoint(connId); //endpoint;
            
            // ensure that the parent endpoint handle this properly
            parent->addIncomingConnection(stateBlock->endpoint);
            
            // fall through otherwise
            stateBlock->currentState = Tcp::ESTABLISHED;
          }
          
          case Tcp::ESTABLISHED:
          case Tcp::FIN_WAIT_1:
          case Tcp::FIN_WAIT_2:
          case Tcp::CLOSE_WAIT:
          case Tcp::CLOSING:
          
            if(stateBlock->snd_una <= stateBlock->seg_ack && stateBlock->seg_ack <= stateBlock->snd_nxt)
              stateBlock->snd_una = stateBlock->seg_ack;
            
            if(stateBlock->seg_ack < stateBlock->snd_una)
              break; // dupe ack
            
            // remove from retransmission queue any acknowledged packets...
            
            if(stateBlock->seg_ack > stateBlock->snd_nxt)
            {
              // send an ack?
            }
            
            if(stateBlock->snd_una < stateBlock->seg_ack && stateBlock->seg_ack <= stateBlock->snd_nxt)
            {
              if((stateBlock->snd_wl1 < stateBlock->seg_seq) || (stateBlock->snd_wl1 == stateBlock->seg_seq && stateBlock->snd_wl2 <= stateBlock->seg_ack))
              {
                stateBlock->snd_wnd = stateBlock->seg_wnd;
                stateBlock->snd_wl1 = stateBlock->seg_seq;
                stateBlock->snd_wl2 = stateBlock->seg_ack;
              }
            }
            
            if(stateBlock->currentState == Tcp::FIN_WAIT_1)
            {
              // if this is ack'ing our FIN - how would we know?
              // probably snd_nxt?
              
              if(stateBlock->fin_seq <= stateBlock->seg_ack)
              {
                stateBlock->currentState = Tcp::FIN_WAIT_2;
                stateBlock->fin_ack = true; // FIN has been acked
              }
            }
            else if(stateBlock->currentState == Tcp::FIN_WAIT_2)
            {
              // user's close can return now, but no deletion of the state block yet
              
              if(stateBlock->fin_seq <= stateBlock->seg_ack)
              {
                stateBlock->currentState = Tcp::FIN_WAIT_2;
                stateBlock->fin_ack = true; // FIN has been acked
              }
            }
            else if(stateBlock->currentState == Tcp::CLOSING)
            {
              if(stateBlock->fin_seq <= stateBlock->seg_ack)
              {
                stateBlock->currentState = Tcp::TIME_WAIT;
                stateBlock->fin_ack = true; // FIN has been acked
              }
            }
            
            break;
            
          case Tcp::LAST_ACK:
        
            // only our FIN ack can come now, so close
            if(stateBlock->fin_seq == stateBlock->seg_seq)
            {
              stateBlock->currentState = Tcp::CLOSED;
              stateBlock->fin_ack = true; // FIN has been acked
            }
            break;
            
          case Tcp::TIME_WAIT:
          
            // only a FIN can come in during this state, ACK it and start timeout...
          
            break;
          
          default:
            break;
        }
        
        if(stateBlock->currentState == Tcp::CLOSED)
          break;
      }
      else
        NOTICE("TCP Packet arriving on port " << Dec << handle.localPort << Hex << " during " << Tcp::stateString(stateBlock->currentState) << " has no ACK.");
      
      if(header->flags & Tcp::URG)
      {
        // handle urgent notification to the application
      }
      
      /* Finally, process the actual segment payload */
      if(stateBlock->currentState == Tcp::ESTABLISHED || stateBlock->currentState == Tcp::FIN_WAIT_1 || stateBlock->currentState == Tcp::FIN_WAIT_2)
      {
        // TODO: handle incoming data... Page 74 of RFC 793          
        if(stateBlock->seg_len)
        {
          NOTICE("TCP Packet arriving on port " << Dec << handle.localPort << Hex << " during " << Tcp::stateString(stateBlock->currentState) << " is ready for payload reading.");
          
          stateBlock->rcv_nxt += stateBlock->seg_len;
          stateBlock->rcv_wnd -= stateBlock->seg_len;
           
          stateBlock->endpoint->depositPayload(stateBlock->seg_len, payload, stateBlock->seg_seq - stateBlock->irs - 1, header->flags & Tcp::PSH);
          
          Tcp::send(from, handle.localPort, handle.remotePort, stateBlock->snd_nxt, stateBlock->rcv_nxt, Tcp::ACK, stateBlock->snd_wnd, 0, 0, pCard);
          alreadyAck = true;
          
          stateBlock->numEndpointPackets++;
        }
      }
      
      if(header->flags & Tcp::FIN)
      {
        if(stateBlock->currentState == Tcp::CLOSED || stateBlock->currentState == Tcp::LISTEN || stateBlock->currentState == Tcp::SYN_SENT)
          break;
        
        stateBlock->rcv_nxt = stateBlock->seg_seq + 1;
        
        if(!alreadyAck)
        {
          Tcp::send(from, handle.localPort, handle.remotePort, stateBlock->snd_nxt, stateBlock->rcv_nxt, Tcp::ACK, stateBlock->snd_wnd, 0, 0, pCard);
          alreadyAck = true;
        }
        
        switch(stateBlock->currentState)
        {
          case Tcp::SYN_RECEIVED:
          case Tcp::ESTABLISHED:
          
            stateBlock->currentState = Tcp::CLOSE_WAIT;
            
            break;
          
          case Tcp::FIN_WAIT_1:
          
            // already been acked previously? if so this needs to go to TIME_WAIT
            // if NOT, closing
            if(stateBlock->fin_ack)
              stateBlock->currentState = Tcp::TIME_WAIT;
            else
              stateBlock->currentState = Tcp::CLOSING;
            
            break;
          
          case Tcp::FIN_WAIT_2:
          
            stateBlock->currentState = Tcp::TIME_WAIT;
            
            break;
          
          case Tcp::CLOSE_WAIT:
          case Tcp::CLOSING:
          case Tcp::LAST_ACK:
          
            // remain this state
            
            break;
          
          default:
            break;
        }
      }
    
      break;
    
    default:
      break;
  }
  
  if(oldState != stateBlock->currentState)
  {
    NOTICE("TCP Packet arriving on port " << Dec << handle.localPort << Hex << " caused state change from " << Tcp::stateString(oldState) << " to " << Tcp::stateString(stateBlock->currentState) << ".");
    stateBlock->waitState.release();
  }
  
  if(stateBlock->currentState == Tcp::CLOSED)
  {
    NOTICE("TCP Packet arriving on port " << Dec << handle.localPort << Hex << " caused connection to close.");
    m_StateBlocks.remove(handle);
    delete stateBlock;
  }
}

void TcpManager::returnEndpoint(Endpoint* e)
{
  if(e)
  {
    m_Endpoints.remove(e->getLocalPort());
    delete e;
  }
}

/*
Endpoint* TcpManager::getEndpoint(Endpoint::RemoteEndpoint remoteHost, uint16_t localPort, Network* pCard)
{
  // is there an endpoint for this port?
  Endpoint* e;
  if((e = m_Endpoints.lookup(localPort)) == 0)
  {
    if(localPort == 0)
      localPort = allocatePort();
  
    stationInfo remote;
    remote.ipv4 = remoteHost.ipv4;
    uint16_t remotePort = remoteHost.remotePort;
    TcpEndpoint* tmp = new TcpEndpoint(remote, localPort, remotePort);
    tmp->setCard(pCard);
    tmp->setRemoteHost(remoteHost);
    e = static_cast<Endpoint*>(tmp);
    m_Endpoints.insert(localPort, e);
  }
  return e;
}
*/

Endpoint* TcpManager::getEndpoint(uint16_t localPort, Network* pCard)
{
  Endpoint* e;
  if((e = m_Endpoints.lookup(localPort)) == 0)
  {
    if(localPort == 0)
      localPort = allocatePort();
    
    TcpEndpoint* tmp = new TcpEndpoint(localPort, 0);
    
    tmp->setCard(pCard);
    
    e = static_cast<Endpoint*>(tmp);
    m_Endpoints.insert(localPort, e);
  }
  return e;
}
