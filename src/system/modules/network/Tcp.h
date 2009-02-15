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
#ifndef MACHINE_TCP_H
#define MACHINE_TCP_H

#include <utilities/String.h>
#include <utilities/Vector.h>
#include <processor/types.h>
#include <process/Semaphore.h>
#include <machine/Network.h>

/**
 * The Pedigree network stack - TCP layer
 */
class Tcp
{
private:

  static Tcp tcpInstance;
  
	struct tcpOption
	{
		uint8_t optkind;
		uint8_t optlen;
	} __attribute__((packed));
  
  // psuedo-header that's added to the packet during checksum
  struct tcpPsuedoHeaderIpv4
  {
    uint32_t  src_addr;
    uint32_t  dest_addr;
    uint8_t   zero;
    uint8_t   proto;
    uint16_t  tcplen;
  } __attribute__ ((packed));
  
public:
  Tcp();
  virtual ~Tcp();
  
  /** For access to the stack without declaring an instance of it */
  static Tcp& instance()
  {
    return tcpInstance;
  }

  struct tcpHeader
  {
    uint16_t  src_port;
    uint16_t  dest_port;
    uint32_t  seqnum;
    uint32_t  acknum;
    uint32_t  rsvd : 4;
    uint32_t  offset : 4;
    uint8_t   flags;
    uint16_t  winsize;
    uint16_t  checksum;
    uint16_t  urgptr;
  } __attribute__ ((packed));
  
  /** Packet arrival callback */
  void receive(IpAddress from, size_t nBytes, uintptr_t packet, Network* pCard, uint32_t offset);
  
  /** Sends a TCP packet */
  static void send(IpAddress dest, uint16_t srcPort, uint16_t destPort, uint32_t seqNumber, uint32_t ackNumber, uint8_t flags, uint16_t window, size_t nBytes, uintptr_t payload, Network* pCard = 0);
  
  /** Calculates a TCP checksum */
  uint16_t tcpChecksum(uint32_t srcip, uint32_t destip, tcpHeader* data, uint16_t len);
  
  /// \todo Work on my hex to be able to do this in my head rather than use decimal numbers
  enum TcpFlag
  {
    FIN = 1,
    SYN = 2,
    RST = 4,
    PSH = 8,
    ACK = 16,
    URG = 32,
    ECE = 64,
    CWR = 128
  };
  
  enum TcpOption
  {
    OPT_END = 0,
    OPT_NOP,
    OPT_MSS,
    OPT_WSS,
    OPT_SACK,
    OPT_TMSTAMP
  };

  enum TcpState
  {
    LISTEN = 0,
    SYN_SENT,
    SYN_RECEIVED,
    ESTABLISHED,
    FIN_WAIT_1,
    FIN_WAIT_2,
    CLOSE_WAIT,
    CLOSING,
    LAST_ACK,
    TIME_WAIT,
    CLOSED,
    UNKNOWN
  };
  
  // provides a string representation of a given state
  static const char* stateString(TcpState state)
  {
    switch(state)
    {
      case LISTEN:
        return "LISTEN";
      case SYN_SENT:
        return "SYN-SENT";
      case SYN_RECEIVED:
        return "SYN-RECEIVED";
      case ESTABLISHED:
        return "ESTABLISHED";
      case FIN_WAIT_1:
        return "FIN-WAIT-1";
      case FIN_WAIT_2:
        return "FIN-WAIT-2";
      case CLOSE_WAIT:
        return "CLOSE-WAIT";
      case CLOSING:
        return "CLOSING";
      case LAST_ACK:
        return "LAST-ACK";
      case TIME_WAIT:
        return "TIME-WAIT";
      case CLOSED:
        return "CLOSED";
      default:
        return "UNKNOWN";
    }
  }
};

#endif
