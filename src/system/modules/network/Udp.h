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
#ifndef MACHINE_UDP_H
#define MACHINE_UDP_H

#include <utilities/String.h>
#include <utilities/Vector.h>
#include <processor/types.h>
#include <process/Semaphore.h>
#include <machine/Network.h>

/**
 * The Pedigree network stack - UDP layer
 */
class Udp
{
private:

  static Udp udpInstance;

  struct udpHeader
  {
    uint16_t  src_port;
    uint16_t  dest_port;
    uint16_t  len;
    uint16_t  checksum;
  } __attribute__ ((packed));
  
  // psuedo-header that's added to the packet during checksum
  struct udpPsuedoHeaderIpv4
  {
    uint32_t  src_addr;
    uint32_t  dest_addr;
    uint8_t   zero;
    uint8_t   proto;
    uint16_t  udplen;
  } __attribute__ ((packed));
  
public:
  Udp();
  virtual ~Udp();
  
  /** For access to the stack without declaring an instance of it */
  static Udp& instance()
  {
    return udpInstance;
  }
  
  /** Packet arrival callback */
  void receive(stationInfo from, size_t nBytes, uintptr_t packet, Network* pCard, uint32_t offset);
  
  /** Sends a UDP packet */
  static void send(stationInfo dest, uint16_t srcPort, uint16_t destPort, size_t nBytes, uintptr_t payload, Network* pCard = 0);
  
  /** Calculates a UDP checksum */
  uint16_t udpChecksum(uint32_t srcip, uint32_t destip, udpHeader* data);
};

#endif
