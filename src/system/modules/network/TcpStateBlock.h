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
#ifndef TCPSTATEBLOCK_H
#define TCPSTATEBLOCK_H

#include <processor/types.h>
#include <process/Semaphore.h>
#include <machine/Network.h>

#include "NetworkStack.h"
#include "Endpoint.h"
#include "TcpMisc.h"

// TCP is based on connections, so we need to keep track of them
// before we even think about depositing into Endpoints. These state blocks
// keep track of important information relating to the connection state.
class StateBlock : public TimerHandler
{
  public:
    StateBlock() :
      currentState(Tcp::CLOSED), localPort(0), remoteHost(),
      iss(0), snd_nxt(0), snd_una(0), snd_wnd(0), snd_up(0), snd_wl1(0), snd_wl2(0),
      rcv_nxt(0), rcv_wnd(0), rcv_up(0), irs(0),
      seg_seq(0), seg_ack(0), seg_len(0), seg_wnd(0), seg_up(0), seg_prc(0),
      fin_ack(false), fin_seq(0),
      numEndpointPackets(0), /// \todo Remove, obsolete
      waitState(0), pCard(0), endpoint(0), connId(0),
      retransmitQueue(), nRemovedFromRetransmit(0),
      waitingForTimeout(false), didTimeout(false), timeoutWait(0), useWaitSem(true),
      m_Nanoseconds(0), m_Seconds(0), m_Timeout(30)
    {
      Timer* t = Machine::instance().getTimer();
      if(t)
        t->registerHandler(this);
    };
    ~StateBlock()
    {
      Timer* t = Machine::instance().getTimer();
      if(t)
        t->unregisterHandler(this);
    };
    
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
    
    // retransmission queue
    TcpBuffer retransmitQueue;
    
    // number of bytes removed from the retransmit queue
    size_t nRemovedFromRetransmit;
    
    // timer for all retransmissions (and state changes such as TIME_WAIT)
    virtual void timer(uint64_t delta, InterruptState& state)
    {
      if(!waitingForTimeout)
        return;
      
      if(UNLIKELY(m_Seconds < m_Timeout))
      {
        m_Nanoseconds += delta;
        if(UNLIKELY(m_Nanoseconds >= 1000000000ULL))
        {
          ++m_Seconds;
          m_Nanoseconds -= 1000000000ULL;
        }
        
        if(UNLIKELY(m_Seconds >= m_Timeout))
        {
          // timeout is hit!
          waitingForTimeout = false;
          didTimeout = true;
          if(useWaitSem)
            timeoutWait.release();
          
          // check to see if there's data on the retransmission queue to send
          if(retransmitQueue.getSize())
          {
            // still more data unacked
            NOTICE("Remote TCP did not ack all the data!");
            
            //TcpManager::instance().send(connId, retransmitQueue.getBuffer(), true, retransmitQueue.getSize(), false);
            
            // reset the timeout
            //resetTimer();
          }
          else if(currentState == Tcp::TIME_WAIT)
          {
            // timer has fired, we need to close the connection
            NOTICE("TIME_WAIT timeout complete");
            currentState = Tcp::CLOSED;
            
            /// \todo How to remove this connection and free the object, when this function is called from
            ///       its instance?
          }
        }
      }
    }
    
    // resets the timer (to restart a timeout)
    void resetTimer(uint32_t timeout = 30)
    {
      m_Seconds = m_Nanoseconds = 0;
      m_Timeout = timeout;
      didTimeout = false;
    }
    
    // are we waiting on a timeout?
    bool waitingForTimeout;
    
    // did the action time out or not?
    /// \note This ensures that, if we end up releasing the timeout wait semaphore
    ///       via a non-timeout source (such as a data ack) we know where the release
    ///       actually came from.
    bool didTimeout;
    
    // timeout wait semaphore (in case)
    Semaphore timeoutWait;
    bool useWaitSem;
  
  private:
    
    // number of nanoseconds & seconds for the timer
    uint64_t m_Nanoseconds;
    uint64_t m_Seconds;
    uint32_t m_Timeout;
  
    StateBlock(const StateBlock& s) :
      currentState(Tcp::CLOSED), localPort(0), remoteHost(),
      iss(0), snd_nxt(0), snd_una(0), snd_wnd(0), snd_up(0), snd_wl1(0), snd_wl2(0),
      rcv_nxt(0), rcv_wnd(0), rcv_up(0), irs(0),
      seg_seq(0), seg_ack(0), seg_len(0), seg_wnd(0), seg_up(0), seg_prc(0),
      fin_ack(false), fin_seq(0),
      numEndpointPackets(0), /// \todo Remove, obsolete
      waitState(0), pCard(0), endpoint(0), connId(0),
      retransmitQueue(), nRemovedFromRetransmit(0),
      waitingForTimeout(false), didTimeout(false), timeoutWait(0), useWaitSem(true),
      m_Nanoseconds(0), m_Seconds(0), m_Timeout(30)
    {
      // same as TcpEndpoint - the copy constructor should not be called
      ERROR("Tcp: StateBlock copy constructor called");
    }
    StateBlock& operator = (const StateBlock& s)
    {
      // this isn't actually correct EITHER
      ERROR("Tcp: StateBlock copy constructor has been called.");
      return *this;
    }
};

#endif
