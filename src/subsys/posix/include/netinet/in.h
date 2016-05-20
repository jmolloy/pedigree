/*
 * Copyright (c) 2008-2014, Pedigree Developers
 *
 * Please see the CONTRIB file in the root of the source tree for a full
 * list of contributors.
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

#ifndef _NETINET_IN_H
#define _NETINET_IN_H

#include <inttypes.h>
#include <sys/socket.h>

/* Grab htons & friends, if needed */
#ifndef htons
#include <net/hton.h>
#endif

typedef uint16_t in_port_t;
typedef uint32_t in_addr_t;

struct in_addr
{
  in_addr_t s_addr;
};


#define _SOCKADDR_SIZE 16
struct sockaddr_in
{
  sa_family_t sin_family;
  in_port_t sin_port;
  struct in_addr sin_addr;
  char sin_zero[_SOCKADDR_SIZE - sizeof(sa_family_t) - sizeof(in_port_t) - sizeof(struct in_addr)];
};

struct ip_mreq {
  struct in_addr imr_multiaddr;
  struct in_addr imr_interface;
};

#define IP_TTL              1

#define IP_MULTICAST_LOOP   64
#define IP_MULTICAST_TTL    65
#define IP_ADD_MEMBERSHIP   66
#define IP_DROP_MEMBERSHIP  67

#ifndef IN_PROTOCOLS_DEFINED
#define IN_PROTOCOLS_DEFINED
#define IPPROTO_IP    0
#define IPPROTO_IPV6  1
#define IPPROTO_ICMP  2
#define IPPROTO_RAW   3
#define IPPROTO_TCP   4
#define IPPROTO_UDP   5
#define IPPROTO_MAX   6
#endif

#define IPPORT_RESERVED         1024
#define IPPORT_USERRESERVED     1024

#define INADDR_ANY        0
#define INADDR_BROADCAST  0xffffffff
#define INADDR_LOOPBACK   0x0100007f /// \todo endianness
#define INADDR_LOCALHOST  INADDR_LOOPBACK

#define INET_ADDRSTRLEN   16


#define IN_CLASSA(a)        (!((a) & 0x80000000)) // No starting bits.
#define IN_CLASSB(a)        ((((a) & 0xC0000000) == 0x80000000)) // Starting bits = 10
#define IN_CLASSC(a)        ((((a) & 0xE0000000) == 0xC0000000)) // Starting bits = 110
#define IN_CLASSD(a)        ((((a) & 0xF0000000) == 0xE0000000)) // Starting bits = 1110

#define IN_MULTICAST(a)     IN_CLASSD(a)

#define IN_CLASSA_NSHIFT    24
#define IN_CLASSA_NET       0xFF000000
#define IN_CLASSA_HOST      0x00FFFFFF

#define IN_CLASSB_NSHIFT    16
#define IN_CLASSB_NET       0xFFFF0000
#define IN_CLASSB_HOST      0x0000FFFF

#define IN_CLASSC_NSHIFT    8
#define IN_CLASSC_NET       0xFFFFFF00
#define IN_CLASSC_HOST      0x000000FF

#define IN_CLASSD_NET       0xF0000000

#include <netinet/in6.h>

#endif
