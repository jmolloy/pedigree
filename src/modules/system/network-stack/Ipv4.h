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
#ifndef MACHINE_IPV4_H
#define MACHINE_IPV4_H

#include <utilities/String.h>
#include <utilities/Vector.h>
#include <processor/types.h>
#include <process/Semaphore.h>
#include <machine/Network.h>
#include <LockGuard.h>
#include <Spinlock.h>

#include "NetworkStack.h"
#include "Ethernet.h"

/// \todo Move to a proper utilities header, called RingBuffer or something
#include "TcpMisc.h"

#include "IpCommon.h"

#define IP_FLAG_RSVD    (1 << 3)

/// If used, this specifies to software at each hop that the packet must not
/// be fragmented. This means it will drop packets that are too small for a
/// link's MTU, even if that link is between two hosts on the Internet.
#define IP_FLAG_DF      (1 << 2)

/// More fragments flag: this means the packet is part of a set of packets
/// that are to be reassembled by the IPv4 code.
#define IP_FLAG_MF      (1 << 1)

/**
 * The Pedigree network stack - IPv4 layer
 */
class Ipv4
{
public:
  Ipv4();
  virtual ~Ipv4();

  /** For access to the stack without declaring an instance of it */
  static Ipv4& instance()
  {
    return ipInstance;
  }

  /** Packet arrival callback */
  void receive(size_t nBytes, uintptr_t packet, Network* pCard, uint32_t offset);

  /** Sends an IP packet */
  static bool send(IpAddress dest, IpAddress from, uint8_t type, size_t nBytes, uintptr_t packet, Network *pCard = 0);

  /** Injects an IPv4 header into a given buffer and returns the size
    * of the header. */
  size_t injectHeader(uintptr_t packet, IpAddress dest, IpAddress from, uint8_t type);

  void injectChecksumAndDataFields(uintptr_t ipv4HeaderStart, size_t payloadSize);

  /**
   * Calculates a checksum for an upper-layer protocol.
   * Will return zero on an incoming packet with a non-zero checksum if the
   * calculated checksum matches.
   * @param length Length of data to checksum, in HOST byte order.
   */
  static uint16_t ipChecksum(IpAddress &from, IpAddress &to, uint8_t proto, uintptr_t data, uint16_t length);

  struct ipHeader
  {
#ifdef LITTLE_ENDIAN
    uint32_t header_len : 4;
    uint32_t ipver : 4;
#else
    uint32_t ipver : 4;
    uint32_t header_len : 4;
#endif
    uint8_t   tos;
    uint16_t  len;
    uint16_t  id;
    uint16_t  frag_offset;
    uint8_t   ttl;
    uint8_t   type;
    uint16_t  checksum;
    uint32_t  ipSrc;
    uint32_t  ipDest;
  } __attribute__ ((packed));

  /** Gets the next IP Packet ID */
  uint16_t getNextId()
  {
    LockGuard<Spinlock> guard(m_NextIdLock);
    return m_IpId++;
  }

private:

  static Ipv4 ipInstance;

  /// IPv4 psuedo-header for upper-layer checksums.
  struct PsuedoHeader
  {
    uint32_t  src_addr;
    uint32_t  dest_addr;
    uint8_t   zero;
    uint8_t   proto;
    uint16_t  datalen;
  } __attribute__ ((packed));

  /// An actual fragment
  struct fragment
  {
      char *data;
      size_t length;
  };

  /// Stores all the segments for an IPv4 fragmented packet.
  /// In its own type so that it can be extended in the future as needed.
  struct fragmentWrapper
  {
      /// Original IPv4 header to be applied to the packet during reassembly
      char *originalIpHeader;

      /// Original IPv4 header length
      size_t originalIpHeaderLen;

      /// Map of offsets to actual buffers
      Tree<size_t, fragment *> fragments;
  };

  /// Identifies an IPv4 packet by linking the ID and IP together
  class Ipv4Identifier
  {
    public:
        Ipv4Identifier() : m_Id(0), m_Ip(IpAddress::IPv4)
        {
        }

        Ipv4Identifier(uint16_t id, IpAddress ip) : m_Id(id), m_Ip(ip)
        {
        }

        inline uint16_t getId()
        {
            return m_Id;
        }

        inline IpAddress &getIp()
        {
            return m_Ip;
        }

        inline bool operator < (Ipv4Identifier &a)
        {
            return (m_Id < a.m_Id);
        }

        inline bool operator > (Ipv4Identifier &a)
        {
            return (m_Id > a.m_Id);
        }

        inline bool operator == (Ipv4Identifier &a)
        {
            return ((m_Id == a.m_Id) && (m_Ip == a.m_Ip));
        }

    private:
        /// ID for all ipv4 packets in this identification block
        uint16_t m_Id;

        /// The IP address the ID is linked to
        IpAddress m_Ip;
  };

  /// Lock for the "Next ID" variable
  Spinlock m_NextIdLock;

  /// Next ID to use for an IPv4 packet
  uint16_t m_IpId;

  /// Maps IP packet IDs to fragment blocks
  Tree<Ipv4Identifier, fragmentWrapper *> m_Fragments;

};

#endif
