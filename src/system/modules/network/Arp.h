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
#ifndef MACHINE_ARP_H
#define MACHINE_ARP_H

#include <utilities/String.h>
#include <utilities/Vector.h>
#include <processor/types.h>
#include <process/Semaphore.h>
#include <machine/Network.h>

#include "NetworkStack.h"
#include "Ethernet.h"

#define ARP_OP_REQUEST  0x0001
#define ARP_OP_REPLY    0x0002

/**
 * The Pedigree network stack - ARP layer
 */
class Arp
{
public:
  Arp();
  virtual ~Arp();
  
  /** For access to the stack without declaring an instance of it */
  static Arp& instance()
  {
    return arpInstance;
  }
  
  /** Packet arrival callback */
  void receive(size_t nBytes, uintptr_t packet, Network* pCard, uint32_t offset);
  
  /** Sends an ARP request */
  void send(stationInfo req, Network* pCard = 0);
  
  /** Gets an entry from the ARP cache, and optionally resolves it if needed. */
  bool getFromCache(uint32_t ipv4, bool resolve, MacAddress* ent, Network* pCard);
  // bool getFromCache(uint8_t ipv6[16], bool resolve, MacAddress* ent);

private:

  static Arp arpInstance;

  /// \todo Needs to support IPv6 as well - protocolSize here is assumed to be 4
  struct arpHeader
  {
    uint16_t  hwType;
    uint16_t  protocol;
    uint8_t   hwSize;
    uint8_t   protocolSize;
    uint16_t  opcode;
    uint8_t   hwSrc[6];
    uint32_t  ipSrc;
    uint8_t   hwDest[6];
    uint32_t  ipDest;
  } __attribute__ ((packed));
  
  // an entry in the arp cache
  /// \todo Will need to store *time* and *type* - time for removing from cache
  ///       and type for static entries
  struct arpEntry
  {
    arpEntry() :
      valid(false), station()
    {};
    
    bool valid;
    stationInfo station;
  };
  
  // an ARP request we've sent
  struct arpRequest
  {
    arpRequest() :
      destStation(), waitSem(0)
    {};
    
    stationInfo destStation;
    Semaphore waitSem;
  };
  
  // ARP cache
  Vector<arpEntry*> m_ArpCache;
  
  // ARP request list
  Vector<arpRequest*> m_ArpRequests;

};

#endif
