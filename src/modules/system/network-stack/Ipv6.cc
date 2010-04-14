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

#include "Ipv6.h"
#include <Module.h>
#include <Log.h>

// Protocols we use in IPv6
#include "Ethernet.h"
#include "Arp.h" /// \todo ARP6

// Child protocols of IPv6
#include "Icmp.h"
#include "Udp.h"
#include "Tcp.h"

#include "NetManager.h"
#include "RawManager.h"
#include "Endpoint.h"

#include "RoutingTable.h"

#include "Filter.h"

Ipv6 Ipv6::ipInstance;

Ipv6::Ipv6() :
  m_NextIdLock(), m_IpId(0)
{
}

Ipv6::~Ipv6()
{
}

bool Ipv6::send(IpAddress dest, IpAddress from, uint8_t type, size_t nBytes, uintptr_t packet, Network *pCard)
{
    /// \todo Implement
    return false;
}

void Ipv6::receive(size_t nBytes, uintptr_t packet, Network* pCard, uint32_t offset)
{
    // Verify the inputs. Drivers may directly dump this information on us so
    // we cannot ever be too sure.
    if(!packet || !nBytes || !pCard)
        return;

    // Check for filtering
    /// \todo Add statistics to NICs
    if(!NetworkFilter::instance().filter(2, packet + offset, nBytes - offset))
        return;
    
    NOTICE("IPv6: received a packet!");

    /// \todo Implement
}
