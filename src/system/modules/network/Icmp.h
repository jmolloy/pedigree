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
#ifndef MACHINE_ICMP_H
#define MACHINE_ICMP_H

#include <utilities/String.h>
#include <utilities/Vector.h>
#include <processor/types.h>
#include <process/Semaphore.h>
#include <machine/Network.h>

/// \todo Implement more!
#define ICMP_ECHO_REPLY   0x00
#define ICMP_ECHO_REQUEST 0x08

/**
 * The Pedigree network stack - ICMP layer
 */
class Icmp
{
public:
  Icmp();
  virtual ~Icmp();
  
  /** For access to the stack without declaring an instance of it */
  static Icmp& instance()
  {
    return icmpInstance;
  }
  
  /** Packet arrival callback */
  void receive(stationInfo from, size_t nBytes, uintptr_t packet, Network* pCard, uint32_t offset);
  
  /** Sends an ICMP packet */
  static void send(stationInfo dest, uint8_t type, uint8_t code, uint16_t id, uint16_t seq, size_t nBytes, uintptr_t payload, Network* pCard = 0);
  
private:

  static Icmp icmpInstance;

  struct icmpHeader
  {
    uint8_t   type;
    uint8_t   code;
    uint16_t  checksum;
    uint16_t  id;
    uint16_t  seq;
  } __attribute__ ((packed));

};

#endif
