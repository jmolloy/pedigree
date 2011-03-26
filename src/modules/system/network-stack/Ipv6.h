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
#ifndef MACHINE_IPV6_H
#define MACHINE_IPV6_H

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

/**
 * The Pedigree network stack - IPv4 layer
 */
class Ipv6 : public IpBase
{
public:
    Ipv6();
    virtual ~Ipv6();

    /** For access to the stack without declaring an instance of it */
    static Ipv6& instance()
    {
        return ipInstance;
    }

    /** Packet arrival callback */
    void receive(size_t nBytes, uintptr_t packet, Network* pCard, uint32_t offset);

    /** Sends an IP packet */
    static bool send(IpAddress dest, IpAddress from, uint8_t type, size_t nBytes, uintptr_t packet, Network *pCard = 0);

    virtual uint16_t ipChecksum(IpAddress &from, IpAddress &to, uint8_t proto, uintptr_t data, uint16_t length);

    virtual size_t injectHeader(uintptr_t packet, IpAddress dest, IpAddress from, uint8_t type);

    virtual void injectChecksumAndDataFields(uintptr_t ipv4HeaderStart, size_t payloadSize);

    struct ip6Header
    {
        uint32_t version : 4;
        uint32_t trafficClass : 8;
        uint32_t flowLabel : 20;
        uint16_t payloadLength;
        uint8_t nextHeader;
        uint8_t hopLimit;
        uint8_t sourceAddress[16];
        uint8_t destAddress[16];
    };

    /** Gets the next IP Packet ID */
    uint16_t getNextId()
    {
        LockGuard<Spinlock> guard(m_NextIdLock);
        return m_IpId++;
    }

private:

    static Ipv6 ipInstance;

    /// Lock for the "Next ID" variable
    Spinlock m_NextIdLock;

    /// Next ID to use for an IPv4 packet
    uint16_t m_IpId;

};

#endif
