/*
 * Copyright (c) 2010 Matthew Iselin
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
#ifndef IP_COMMON_H
#define IP_COMMON_H

#include <processor/types.h>

class IpAddress;
class Network;

// This file contains definitions common to IPv4 and IPv6

/// IP ICMP protocol type
#define IP_ICMP     0x01

/// IP UDP protocol type
#define IP_UDP      0x11

/// IP TCP protocol type
#define IP_TCP      0x06

/// IP ICMPv6 protocol type
#define IP_ICMPV6   0x3A

/// Base class for IPv4/IPv6, for generic checksums
class IpBase
{
    public:
        IpBase() {}
        virtual ~IpBase() {}

        virtual bool send(IpAddress dest, IpAddress from, uint8_t type, size_t nBytes, uintptr_t packet, Network *pCard = 0) = 0;

        virtual uint16_t ipChecksum(IpAddress &from, IpAddress &to, uint8_t proto, uintptr_t data, uint16_t length) = 0;
};

#endif

