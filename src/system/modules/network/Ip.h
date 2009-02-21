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
#ifndef MACHINE_IP_H
#define MACHINE_IP_H

#include <utilities/String.h>
#include <utilities/Vector.h>
#include <processor/types.h>
#include <process/Semaphore.h>
#include <machine/Network.h>

#include "NetworkStack.h"
#include "Ethernet.h"

#define IP_ICMP  0x01 
#define IP_UDP   0x11
#define IP_TCP   0x06

/**
 * The Pedigree network stack - IP layer
 * \todo IPv6
 */
class Ip
{
public:
  Ip();
  virtual ~Ip();
  
  /** For access to the stack without declaring an instance of it */
  static Ip& instance()
  {
    return ipInstance;
  }
  
  /** Packet arrival callback */
  void receive(size_t nBytes, uintptr_t packet, Network* pCard, uint32_t offset);
  
  /** Sends an IP packet */
  static bool send(IpAddress dest, IpAddress from, uint8_t type, size_t nBytes, uintptr_t packet, Network* pCard = 0);

  /// \todo Needs to support IPv6 as well - protocolSize here is assumed to be 4
  struct ipHeader
  {
    uint8_t   verlen;
    uint8_t   tos;
    uint16_t  len;
    uint16_t  id;
    uint16_t  frag;
    uint8_t   ttl;
    uint8_t   type;
    uint16_t  checksum;
    uint32_t  ipSrc;
    uint32_t  ipDest;
  } __attribute__ ((packed));

  /** Gets the next IP Packet ID */
  uint16_t getNextId()
  {
    return m_IpId++;
  }
  
private:

  static Ip ipInstance;
  
  uint16_t m_IpId;

};

#endif
