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

#include <process/Mutex.h>
#include <LockGuard.h>

#define TCP_DEBUG 1

void TcpManager::receive(IpAddress from, uint16_t sourcePort, uint16_t destPort, Tcp::tcpHeader* header, uintptr_t payload, size_t payloadSize, Network* pCard)
{
  // sanity checks
  if(!header)
    return;

  LockGuard<Mutex> guard(m_TcpMutex);

  // Find the state block if possible, if none exists create one
  bool bDidAllocateStateBlock = false;
  StateBlockHandle handle;
  handle.localPort = destPort;
  handle.remotePort = sourcePort;
  handle.remoteHost.ip = from;
  handle.listen = false; // DON'T look for listen sockets yet
  StateBlock* stateBlock;
  if((stateBlock = m_StateBlocks.lookup(handle)) == 0)
  {
    // Check for a listen socket
    handle.listen = true;
    if((stateBlock = m_ListeningStateBlocks.lookup(handle)) == 0)
    {
      // Port doesn't exist, so temporary stateBlock required for proper RST handle
      stateBlock = new StateBlock;
      if(stateBlock == 0)
        return;

      WARNING("TCP Packet arriving on port " << Dec << handle.localPort << Hex << " has no destination.");

      bDidAllocateStateBlock = true;

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
#if TCP_DEBUG
  NOTICE("TCP Packet arrived while stateBlock in " << Tcp::stateString(stateBlock->currentState) << " [remote port = " << Dec << stateBlock->remoteHost.remotePort << Hex << "] [connId = " << stateBlock->connId << "].");

  // dump some pretty information
  NOTICE(" + SEQ=" << Dec << stateBlock->seg_seq << " ACK=" << stateBlock->seg_ack << " LEN=" << stateBlock->seg_len << " WND=" << stateBlock->seg_wnd << Hex);
  NOTICE(" + FLAGS: " <<
                (header->flags & Tcp::FIN ? "FIN " : "") <<
                (header->flags & Tcp::SYN ? "SYN " : "") <<
                (header->flags & Tcp::RST ? "RST " : "") <<
                (header->flags & Tcp::PSH ? "PSH " : "") <<
                (header->flags & Tcp::ACK ? "ACK " : "") <<
                (header->flags & Tcp::URG ? "URG " : "") <<
                (header->flags & Tcp::ECE ? "ECE " : "") <<
                (header->flags & Tcp::CWR ? "CWR " : "")
  );
#endif

  switch(stateBlock->currentState)
  {
    /* Incoming segment while the state is CLOSED */
    case Tcp::CLOSED:

      // If no RST, we need to send one
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
        if(!Tcp::send(from, handle.localPort, handle.remotePort, seq, ack, flags, window, 0, 0))
          WARNING("TCP: Sending RST due to incoming segment while in CLOSED state failed.");
      }

      if(bDidAllocateStateBlock)
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
        // An ACK on a listen state is invalid
        if(!Tcp::send(from, handle.localPort, handle.remotePort, stateBlock->seg_ack, 0, Tcp::RST, 0, 0, 0))
          WARNING("TCP: Sending RST due to ACK while in LISTEN state failed.");
      }
      else if(header->flags & Tcp::SYN)
      {
        // Create a new server state block for this incoming connection, and register the new client
        // connection ID with the listening endpoint

        size_t connId = getConnId();

        StateBlock* newStateBlock = new StateBlock;
        if(!newStateBlock)
        {
          // If we don't get this new block, pretend we're not listening
          if(!Tcp::send(from, handle.localPort, handle.remotePort, 0, stateBlock->seg_ack, Tcp::RST | Tcp::ACK, 0, 0, 0))
            WARNING("TCP: Couldn't ACK a SYN because no memory was available for a state block.");
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

        newStateBlock->seg_seq = newStateBlock->rcv_nxt;

        newStateBlock->currentState = Tcp::SYN_RECEIVED;

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
          if(!Tcp::send(from, handle.localPort, handle.remotePort, 0, stateBlock->seg_ack, Tcp::RST | Tcp::ACK, 0, 0, 0))
            WARNING("TCP: Couldn't ACK a SYN because no memory was available for a state block handle.");
          return;
        }

        *tmp = handle;

        m_StateBlocks.insert(handle, newStateBlock);
        m_CurrentConnections.insert(connId, tmp);

        // ACK the SYN
        IpAddress dest;
        dest = newStateBlock->remoteHost.ip;
        if(!Tcp::send(dest, newStateBlock->localPort, newStateBlock->remoteHost.remotePort, newStateBlock->iss, newStateBlock->rcv_nxt, Tcp::SYN | Tcp::ACK, newStateBlock->snd_wnd, 0, 0))
          WARNING("TCP: Sending SYN/ACK failed");
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
            if(!Tcp::send(from, handle.localPort, handle.remotePort, stateBlock->seg_ack, 0, Tcp::RST, 0, 0, 0))
              WARNING("TCP: Sending RST due to invalid ACK while in SYN_SENT state failed.");
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
          // Drop segment, we're closing NOW!
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

            if(!Tcp::send(from, handle.localPort, handle.remotePort, stateBlock->snd_nxt, stateBlock->rcv_nxt, Tcp::ACK, stateBlock->snd_wnd, 0, 0))
              WARNING("TCP: Sending ACK due to SYN/ACK while in SYN_SENT state failed.");

            break;
          }
          else
          {
            stateBlock->currentState = Tcp::SYN_RECEIVED;

            if(!Tcp::send(from, handle.localPort, handle.remotePort, stateBlock->iss, stateBlock->rcv_nxt, Tcp::SYN | Tcp::ACK, stateBlock->snd_wnd, 0, 0))
              WARNING("TCP: Sending SYN/ACK due to incorrect SYN/ACK while in SYN_SENT state failed.");

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

      // Is SYN set on the incoming packet?
      if(header->flags & Tcp::SYN)
      {
        NOTICE("TCP: unexpected SYN!");
        if(!Tcp::send(from, handle.localPort, handle.remotePort, stateBlock->snd_nxt, stateBlock->rcv_nxt, Tcp::ACK | Tcp::RST, stateBlock->snd_wnd, 0, 0))
          WARNING("TCP: Sending RST due to SYN during non-SYN phase failed.");
        break;
      }

      if((stateBlock->seg_len == 0) && (stateBlock->rcv_wnd == 0))
      {
        // Unacceptable
        if(!(stateBlock->seg_seq == stateBlock->rcv_nxt))
        {
          NOTICE("TCP Packet arriving on port " << Dec << handle.localPort << Hex << " during " << Tcp::stateString(stateBlock->currentState) << " is unacceptable 1.");
          if(!Tcp::send(from, handle.localPort, handle.remotePort, stateBlock->snd_nxt, stateBlock->rcv_nxt, Tcp::ACK, stateBlock->snd_wnd, 0, 0))
            WARNING("TCP: Sending ACK due to unacceptable ACK (1) while in post-SYN_SENT state failed.");
          break;
        }
      }

      /*
        (NN) TCP Packet arriving on port 1234 during SYN-RECEIVED is unacceptable 2.
        (NN)     >> RCV_NXT = 0xd3c9b468
        (NN)     >> SEG_SEQ = 0xd3c9b467
        (NN)     >> RCV_NXT + RCV_WND = 0xd3c9cae8
        */

      if((stateBlock->seg_len == 0) && (stateBlock->rcv_wnd > 0))
      {
        // Unacceptable
        if(!((stateBlock->rcv_nxt <= stateBlock->seg_seq) && (stateBlock->seg_seq < (stateBlock->rcv_nxt + stateBlock->rcv_wnd))))
        {
          NOTICE("TCP Packet arriving on port " << Dec << handle.localPort << Hex << " during " << Tcp::stateString(stateBlock->currentState) << " is unacceptable 2.");
          NOTICE("    >> RCV_NXT = " << stateBlock->rcv_nxt);
          NOTICE("    >> SEG_SEQ = " << stateBlock->seg_seq);
          NOTICE("    >> RCV_NXT + RCV_WND = " << (stateBlock->rcv_nxt + stateBlock->rcv_wnd));
          if(!Tcp::send(from, handle.localPort, handle.remotePort, stateBlock->snd_nxt, stateBlock->rcv_nxt, Tcp::ACK, stateBlock->snd_wnd, 0, 0))
            WARNING("TCP: Sending ACK due to unacceptable ACK (2) while in post-SYN_SENT state failed.");
          break;
        }
      }

      // Unacceptable
      if((stateBlock->seg_len > 0) && (stateBlock->rcv_wnd == 0))
      {
        NOTICE("TCP Packet arriving on port " << Dec << handle.localPort << Hex << " during " << Tcp::stateString(stateBlock->currentState) << " is unacceptable 3.");
        if(!Tcp::send(from, handle.localPort, handle.remotePort, stateBlock->snd_nxt, stateBlock->rcv_nxt, Tcp::ACK, stateBlock->snd_wnd, 0, 0))
          WARNING("TCP: Sending ACK due to unacceptable ACK (3) while in post-SYN_SENT state failed.");
        break;
      }

      if((stateBlock->seg_len > 0) && (stateBlock->rcv_wnd > 0))
      {
        if(!(
          ((stateBlock->rcv_nxt <= stateBlock->seg_seq) && (stateBlock->seg_seq < (stateBlock->rcv_nxt + stateBlock->rcv_wnd)))
          ||
          ((stateBlock->rcv_nxt <= (stateBlock->seg_seq + stateBlock->seg_len - 1)) && ((stateBlock->seg_seq + stateBlock->seg_len - 1) < (stateBlock->rcv_nxt + stateBlock->rcv_wnd)))))
        {
          NOTICE("TCP Packet arriving on port " << Dec << handle.localPort << Hex << " during " << Tcp::stateString(stateBlock->currentState) << " is unacceptable 4.");
          if(!Tcp::send(from, handle.localPort, handle.remotePort, stateBlock->snd_nxt, stateBlock->rcv_nxt, Tcp::ACK, stateBlock->snd_wnd, 0, 0))
            WARNING("TCP: Sending ACK due to unacceptable ACK (4) while in post-SYN_SENT state failed.");
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

            // Merely close (state change below)
            break;

          default:
            break;
        }

        stateBlock->currentState = Tcp::CLOSED;
        break;
      }

      /// \todo Check security and precedence (IP header)...

      if(header->flags & Tcp::ACK)
      {
        // Remove all acked segments from the transmit queue
        stateBlock->ackSegment();

        switch(stateBlock->currentState)
        {
          case Tcp::SYN_RECEIVED:
          {
            if(!(stateBlock->snd_una <= stateBlock->seg_ack && stateBlock->seg_ack <= stateBlock->snd_nxt))
            {
              NOTICE("TCP Packet arriving on port " << Dec << handle.localPort << Hex << " during " << Tcp::stateString(stateBlock->currentState) << " is an unacceptable segment ACK.");
              if(!Tcp::send(from, handle.localPort, handle.remotePort, stateBlock->seg_ack, 0, Tcp::RST, 0, 0, 0))
                WARNING("TCP: Sending ACK due to unacceptable segment ACK while in post-SYN_SENT state failed.");
              break;
            }

            size_t connId = stateBlock->connId;
            TcpEndpoint* parent = stateBlock->endpoint;
            if(!parent)
            {
              WARNING("TCP State Block is in SYN_RECEIVED but has no parent endpoint!");
              return;
            }

            stateBlock->endpoint = new TcpEndpoint(connId, from, stateBlock->localPort, stateBlock->remoteHost.remotePort);
            if(!stateBlock->endpoint)
            {
              removeConn(connId);
              if(!Tcp::send(from, handle.localPort, handle.remotePort, stateBlock->seg_ack, 0, Tcp::RST, 0, 0, 0))
                WARNING("TCP: Sending RST due to no memory for incoming connection's endpoint");
              return;
            }

            // Fall through otherwise
            stateBlock->currentState = Tcp::ESTABLISHED;

            // Ensure that the parent endpoint handles this properly
            parent->addIncomingConnection(stateBlock->endpoint);
          }

          case Tcp::ESTABLISHED:
          case Tcp::FIN_WAIT_1:
          case Tcp::FIN_WAIT_2:
          case Tcp::CLOSE_WAIT:
          case Tcp::CLOSING:

            if(stateBlock->seg_ack < stateBlock->snd_una)
              break; // Dupe ack, just skip it and continue

            // Remove from retransmission queue any acknowledged packets...
            //stateBlock->retransmitQueue.remove(stateBlock->snd_una - stateBlock->nRemovedFromRetransmit, stateBlock->seg_len);
            //stateBlock->nRemovedFromRetransmit += (stateBlock->seg_ack - stateBlock->snd_una);

            // update the unack'd data information
            if(stateBlock->snd_una < stateBlock->seg_ack && stateBlock->seg_ack <= stateBlock->snd_nxt)
              stateBlock->snd_una = stateBlock->seg_ack;

            // Reset the retransmit timer
            stateBlock->resetTimer();

            if(stateBlock->snd_una >= stateBlock->snd_nxt)
              stateBlock->waitingForTimeout = false;

            if(stateBlock->seg_ack > stateBlock->snd_nxt)
            {
              // Ack the ack with the proper sequence number, because the remote TCP has ack'd data that hasn't been sent
              if(!Tcp::send(from, handle.localPort, handle.remotePort, stateBlock->snd_nxt, stateBlock->seg_seq, Tcp::ACK, stateBlock->snd_wnd, 0, 0))
                WARNING("TCP: Sending ACK with proper sequence number (remote TCP ack'd data that we didn't send) failed.");
              else
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
        // Is this a valid data segment?
        if(stateBlock->seg_seq < stateBlock->rcv_nxt)
        {
          // Transmission of already-acked data. Resend an ACK.
          WARNING(" + (sequence is already partially acked)");
          if(!Tcp::send(from, handle.localPort, handle.remotePort, stateBlock->snd_nxt, stateBlock->rcv_nxt, Tcp::ACK, stateBlock->snd_wnd, 0, 0))
            WARNING("TCP: Sending ACK for incoming data failed!");
          else
            alreadyAck = true;
        }
        else if(stateBlock->seg_seq > stateBlock->rcv_nxt)
        {
          // Packet has come in out-of-order - drop on the floor.
          WARNING(" + (sequence out of order)");
          alreadyAck = true;
        }
        else if(stateBlock->seg_len)
        {
          NOTICE(" + Payload: " << String(reinterpret_cast<const char*>(payload)));
          if(stateBlock->endpoint)
          {
            size_t winChange = stateBlock->endpoint->depositPayload(stateBlock->seg_len, payload, stateBlock->seg_seq - stateBlock->irs - 1, (header->flags & Tcp::PSH) == Tcp::PSH);
            stateBlock->rcv_nxt += winChange;
            if(winChange > stateBlock->rcv_wnd)
            {
                WARNING("TCP: incoming data was larger than rcv_wind");
                winChange = stateBlock->rcv_wnd;
            }
            stateBlock->snd_wnd -= winChange;

            if(!Tcp::send(from, handle.localPort, handle.remotePort, stateBlock->snd_nxt, stateBlock->rcv_nxt, Tcp::ACK, stateBlock->snd_wnd, 0, 0))
              WARNING("TCP: Sending ACK for incoming data failed!");
            else
              alreadyAck = true;
          }

          stateBlock->numEndpointPackets++;
        }
      }

      if(header->flags & Tcp::FIN)
      {
        if(stateBlock->currentState == Tcp::CLOSED || stateBlock->currentState == Tcp::LISTEN || stateBlock->currentState == Tcp::SYN_SENT)
          break;

        // FIN means the remote host has nothing more to send, so push any remaining data to the application
        if(stateBlock->endpoint)
          stateBlock->endpoint->depositPayload(0, 0, 0, true);

        stateBlock->rcv_nxt = stateBlock->seg_seq + 1;

        if(!alreadyAck)
        {
          if(!Tcp::send(from, handle.localPort, handle.remotePort, stateBlock->snd_nxt, stateBlock->rcv_nxt, Tcp::ACK, stateBlock->snd_wnd, 0, 0))
            WARNING("TCP: Sending ACK to FIN failed.");
          else
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
              stateBlock->currentState = Tcp::TIME_WAIT;
              //stateBlock->currentState = Tcp::CLOSED;

              stateBlock->resetTimer(120); // 2 minute timeout for TIME_WAIT
              stateBlock->waitingForTimeout = true;
            }
            else
              stateBlock->currentState = Tcp::CLOSING;

            break;

          case Tcp::FIN_WAIT_2:

            stateBlock->currentState = Tcp::TIME_WAIT;
            //stateBlock->currentState = Tcp::CLOSED;

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
#if TCP_DEBUG
    NOTICE("TCP Packet arriving on port " << Dec << handle.localPort << Hex << " caused state change from " << Tcp::stateString(oldState) << " to " << Tcp::stateString(stateBlock->currentState) << ".");
#endif
    stateBlock->endpoint->stateChanged(stateBlock->currentState);
    stateBlock->waitState.release();
  }

  if(stateBlock->currentState == Tcp::CLOSED)
  {
#if TCP_DEBUG
    NOTICE("TCP Packet arriving on port " << Dec << handle.localPort << Hex << " caused connection to close.");
#endif

    // If we are in a state that's not created by user intervention, we can safely remove and close the connection
    // both of these require the user to go through a close() operation, which inherently calls disconnect.
    /// \note If you're working in kernel space, and you close a connection, *DO NOT* return the endpoint
    if(oldState == Tcp::LAST_ACK || oldState == Tcp::CLOSING)
    {
      TcpManager::instance().returnEndpoint(stateBlock->endpoint);
      removeConn(stateBlock->connId);
    }
  }
}
