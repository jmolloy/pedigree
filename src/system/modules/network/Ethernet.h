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
#ifndef MACHINE_ETHERNET_H
#define MACHINE_ETHERNET_H

#include <utilities/String.h>
#include <utilities/Vector.h>
#include <processor/types.h>
#include <machine/Network.h>
#include "NetworkStack.h"

#define ETH_ARP   0x0806
#define ETH_RARP  0x8035
#define ETH_IP    0x0800

/**
 * The Pedigree network stack - Ethernet layer
 */
class Ethernet
{
public:
  Ethernet();
  virtual ~Ethernet();
  
  /** For access to the stack without declaring an instance of it */
  static Ethernet& instance()
  {
    return ethernetInstance;
  }
  
  /** Packet arrival callback */
  void receive(size_t nBytes, uintptr_t packet, Network* pCard, uint32_t offset);
  
  /** Sends an ethernet packet */
  static void send(size_t nBytes, uintptr_t packet, Network* pCard, MacAddress dest, uint16_t type);

private:

  static Ethernet ethernetInstance;

  struct ethernetHeader
  {
    uint8_t   destMac[6];
    uint8_t   sourceMac[6];
    uint16_t  type;
  } __attribute__ ((packed));

};

#endif
