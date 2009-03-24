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

void TcpManager::receive(IpAddress from, uint16_t sourcePort, uint16_t destPort, Tcp::tcpHeader* header, uintptr_t payload, size_t payloadSize, Network* pCard)
{
  // sanity checks
  if(!header)
    return;
  
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
    if((stateBlock = m_ListeningStateBlocks.lookup(handle)) == 0)
    {
      // port doesn't exist, so temporary stateBlock required for proper RST handle
      stateBlock = new StateBlock;
      if(stateBlock == 0)
        return;
      
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
  stateBlock->seg_prc = 0; // IP header contains precedence information
  stateBlock->rcv_wnd = stateBlock->seg_wnd;
  
  stateBlock->fin_ack = false;
  
  // has an Ack already been sent in this segment?
  bool alreadyAck = false;
  
  // what state are we in?
  // RFC793, page 65 onwards
  Tcp::TcpState oldState = stateBlock->currentState;
  NOTICE("TCP Packet arrived while stateBlock in " << Tcp::stateString(stateBlock->currentState) << " [remote port = " << Dec << stateBlock->remoteHost.remotePort << Hex << "] [connId = " << stateBlock->connId << "].");
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
        // create a new server state block for this incoming connection, and register the new client
        // connection ID with the listening endpoint
        
        size_t connId = getConnId();
  
        StateBlock* newStateBlock = new StateBlock;
        if(!newStateBlock)
        {
          // if we don't get this new block, pretend we're notlistening
          Tcp::send(from, handle.localPort, handle.remotePort, 0, stateBlock->seg_ack, Tcp::RST | Tcp::ACK, 0, 0, 0, pCard);
          return;
        }
        
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
        
        newStateBlock->currentState = Tcp::SYN_RECEIVED;
        
        newStateBlock->pCard = pCard;
        
        newStateBlock->endpoint = stateBlock->endpoint;
        
        newStateBlock->numEndpointPackets = 0;
        
        handle.listen = false;
        handle.localPort = destPort;
        handle.remotePort = sourcePort;
        handle.remoteHost.ip = from;
        
        StateBlockHandle* tmp = new StateBlockHandle;
        if(!tmp)
        {
          delete newStateBlock;
          Tcp::send(from, handle.localPort, handle.remotePort, 0, stateBlock->seg_ack, Tcp::RST | Tcp::ACK, 0, 0, 0, pCard);
          return;
        }
        
        *tmp = handle;
        
        m_StateBlocks.insert(handle, newStateBlock);
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
            /// \note LISTEN sockets never go into SYN_RECEIVED, so
            ///       we don't handle a passive open case here
            break;
          
          case Tcp::ESTABLISHED:
          case Tcp::FIN_WAIT_1:
          case Tcp::FIN_WAIT_2:
          case Tcp::CLOSE_WAIT:
          
            /// \todo recv/send need to handle the connection being reset
            
            stateBlock->currentState = Tcp::CLOSED;
            
            break;
            
          case Tcp::CLOSING:
          case Tcp::LAST_ACK:
          case Tcp::TIME_WAIT:
          
            // merely close (state change below)
            break;
          
          default:
            break;
        }
        
        stateBlock->currentState = Tcp::CLOSED;
        break;
      }
      
      /// \todo Check security and precedence (IP header)...
      
      if(header->flags & Tcp::SYN)
      {
        /// \todo RST needs to be sent
        NOTICE("TCP: unexpected SYN!");
        break;
      }
      
      if(header->flags & Tcp::ACK)
      {
        // remove all acked segments from the transmit queue
        stateBlock->ackSegment();
        
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
            if(!parent)
            {
              NOTICE("TCP State Block is in SYN_RECEIVED but has no parent endpoint!");
              return;
            }
            
            stateBlock->endpoint = new TcpEndpoint(connId, from, stateBlock->localPort, stateBlock->remoteHost.remotePort);
            if(!stateBlock->endpoint)
            {
              removeConn(connId);
              Tcp::send(from, handle.localPort, handle.remotePort, stateBlock->seg_ack, 0, Tcp::RST, 0, 0, 0, pCard);
              return;
            }
            
            // ensure that the parent endpoint handles this properly
            NOTICE("stateBlock->endpoint is " << reinterpret_cast<uintptr_t>(stateBlock->endpoint) << " and parent is " << reinterpret_cast<uintptr_t>(parent) << "...");
            parent->addIncomingConnection(stateBlock->endpoint);
            NOTICE("added incoming connection!");
            
            // fall through otherwise
            stateBlock->currentState = Tcp::ESTABLISHED;
          }
          
          case Tcp::ESTABLISHED:
          case Tcp::FIN_WAIT_1:
          case Tcp::FIN_WAIT_2:
          case Tcp::CLOSE_WAIT:
          case Tcp::CLOSING:
            
            if(stateBlock->seg_ack < stateBlock->snd_una)
              break; // dupe ack, just skip it and continue
            
            // remove from retransmission queue any acknowledged packets...
            //stateBlock->retransmitQueue.remove(stateBlock->snd_una - stateBlock->nRemovedFromRetransmit, stateBlock->seg_len);
            //stateBlock->nRemovedFromRetransmit += (stateBlock->seg_ack - stateBlock->snd_una);
          
            // update the unack'd data information
            if(stateBlock->snd_una < stateBlock->seg_ack && stateBlock->seg_ack <= stateBlock->snd_nxt)
              stateBlock->snd_una = stateBlock->seg_ack;
            
            // reset the retransmit timer
            stateBlock->resetTimer();
            
            if(stateBlock->snd_una >= stateBlock->snd_nxt)
              stateBlock->waitingForTimeout = false;
            
            if(stateBlock->seg_ack > stateBlock->snd_nxt)
            {
              // ack the ack with the proper sequence number, because the remote TCP has ack'd data that hasn't been sent
              Tcp::send(from, handle.localPort, handle.remotePort, stateBlock->snd_nxt, stateBlock->seg_seq, Tcp::ACK, stateBlock->snd_wnd, 0, 0, pCard);
              alreadyAck = true;
              break;
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
                //stateBlock->currentState = Tcp::TIME_WAIT;
                stateBlock->currentState = Tcp::CLOSED;
                stateBlock->fin_ack = true; // FIN has been acked
              }
            }
            
            break;
            
          case Tcp::LAST_ACK:
        
            // only our FIN ack can come now, so close
            if((stateBlock->fin_seq + 1) == stateBlock->seg_ack)
            {
              stateBlock->currentState = Tcp::CLOSED;
              stateBlock->fin_ack = true; // FIN has been acked
            }
            break;
            
          case Tcp::TIME_WAIT:
          
            // only a FIN can come in during this state, ACK will be performed later on
            stateBlock->resetTimer(120); // 2 minute timeout for TIME_WAIT
            stateBlock->waitingForTimeout = true;
          
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
        if(stateBlock->seg_len)
        {          
          stateBlock->rcv_nxt += stateBlock->seg_len;
          stateBlock->rcv_wnd -= stateBlock->seg_len;
          
          if(stateBlock->endpoint)
            stateBlock->endpoint->depositPayload(stateBlock->seg_len, payload, stateBlock->seg_seq - stateBlock->irs - 1, (header->flags & Tcp::PSH) == Tcp::PSH);
          
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
            {
              //stateBlock->currentState = Tcp::TIME_WAIT;
              stateBlock->currentState = Tcp::CLOSED;
              
              stateBlock->resetTimer(120); // 2 minute timeout for TIME_WAIT
              stateBlock->waitingForTimeout = true;
            }
            else
              stateBlock->currentState = Tcp::CLOSING;
            
            break;
          
          case Tcp::FIN_WAIT_2:
          
            NOTICE("closing! winwinwin!");
          
            //stateBlock->currentState = Tcp::TIME_WAIT;
            stateBlock->currentState = Tcp::CLOSED;
            
            stateBlock->resetTimer(120); // 2 minute timeout for TIME_WAIT
            stateBlock->waitingForTimeout = true;
            
            break;
          
          case Tcp::CLOSE_WAIT:
          case Tcp::CLOSING:
          case Tcp::LAST_ACK:
          
            // remain this state
            
            break;
          
          case Tcp::TIME_WAIT:
            
            // reset the timer
            stateBlock->resetTimer(120); // 2 minute timeout for TIME_WAIT
            stateBlock->waitingForTimeout = true;
          
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
    TcpManager::instance().returnEndpoint(stateBlock->endpoint);
    removeConn(stateBlock->connId);
  }
}
