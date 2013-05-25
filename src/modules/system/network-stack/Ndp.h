/*
 * Copyright (c) 2011 Matthew Iselin, Heights College
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
#ifndef _NETWORK_NDP_H
#define _NETWORK_NDP_H

#include <processor/types.h>
#include <machine/Network.h>

#include <utilities/RadixTree.h>

#include "IpCommon.h"

#define NADVERT_FLAGS_ROUTER        1
#define NADVERT_FLAGS_SOLICIT       2
#define NADVERT_FLAGS_OVERRIDE      4

class Ndp
{
    public:
        Ndp();
        virtual ~Ndp();

        static Ndp &instance()
        {
            return ndpInstance;
        }

        void receive(IpAddress from, IpAddress to, uint8_t icmpType, uint8_t icmpCode, uintptr_t payload, size_t nBytes, Network *pCard);

        /// Solicit a neighbour for an address. Uses cache where possible.
        bool neighbourSolicit(IpAddress addr, MacAddress *pMac, Network *pCard);

        /// Solicit a router for routing information. Should be called if
        /// DHCPv6 is not used, in order to obtain a routable address for full
        /// IPv6 connectivity.
        /// Will automatically modify the StationInfo of pCard.
        bool routerSolicit(Network *pCard);

        /// Adds a given IP->LinkLayer association to the cache.
        void addEntry(IpAddress addr, MacAddress mac);

    private:
        static Ndp ndpInstance;

        RadixTree<MacAddress*> m_LookupCache;

        struct RouterSolicitation
        {
            uint32_t reserved;
        } __attribute__((packed));

        struct RouterAdvertisement
        {
            uint16_t lifetime;
            uint16_t hopLimitFlags;
            uint32_t reachableTime;
            uint32_t retransTimer;
        } __attribute__((packed));

        struct NeighbourSolicitation
        {
            uint32_t reserved;
            uint8_t  target[16];
        } __attribute__((packed));

        struct NeighbourAdvertisement
        {
            uint32_t flags;
            uint8_t  target[16];
        } __attribute__((packed));

        struct Option
        {
            uint8_t type;
            uint8_t length;
        } __attribute__((packed));

        struct LinkLayerAddressOption
        {
            uint8_t type;
            uint8_t length;
            uint8_t address[6];
        } __attribute__((packed));

        struct PrefixInformationOption
        {
            uint8_t type;
            uint8_t length;
            uint8_t prefixLength;
            uint8_t rsvdFlags;
            uint32_t validLifetime;
            uint32_t preferredLifetime;
            uint32_t rsvd2;
            uint8_t prefix[16];
        } __attribute__((packed));
};

#endif
