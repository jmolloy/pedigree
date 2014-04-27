/*
 * 
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

#ifndef MACHINE_ROUTINGTABLE_H
#define MACHINE_ROUTINGTABLE_H

#include <utilities/String.h>
#include <utilities/Tree.h>
#include <utilities/List.h>
#include <utilities/RadixTree.h>
#include <process/Semaphore.h>
#include <machine/Network.h>
#include <config/Config.h>

/**
 * The Pedigree routing table supports three different ways to route packets:
 * 1. Destination IP match
 * 2. Destination IP match with substitution
 * 3. Destination subnet match
 * 4. Complement of destination subnet match
 * 5. Named route
 *
 * A destination IP match merely uses a specific interface if the IP to send a
 *     a packet to matches the IP in the table. There is no substitution
 *     performed at all.
 * A destination IP match with substitution acts like a normal destination IP
 *     match, except it also performs IP substitution.
 * A destination subnet match uses a specific interface if the IP to send a
 *     packet to is within the subnet defined in the table. No substitution is
 *     performed at all.
 * A complement of destination subnet match uses a specific interface if the IP
 *     to send a packet to is *not* within the subnet defined in the table.
 *     This substitutes the destination IP to that of a device that can route
 *     the packets to the given subnet.
 * A named route is a route with a specific name, such as "default".
 *
 * All of these are stored in the routes table in the configuration database.
 */

/** Routing table implementation */
class RoutingTable
{
    public:

        enum Type
        {
            DestIp = 0,
            DestIpSub,
            DestSubnet,
            DestSubnetComplement,
            DestIpv6,
            DestIpv6Sub,
            DestPrefix,
            DestPrefixComplement,
            Named,
            NamedV6
        };

        RoutingTable();
        virtual ~RoutingTable();

        static RoutingTable& instance()
        {
            return m_Instance;
        }

        inline bool hasRoutes()
        {
            return m_bHasRoutes;
        }

        /** Adds a route to the table */
        void Add(Type type, IpAddress dest, IpAddress subIp, String meta, Network *card);

        /** Adds a subnet-based route to the table */
        void Add(Type type, IpAddress dest, IpAddress subnet, IpAddress subIp, String meta, Network *card);

        /// \todo Functions to remove routes

        /**
         * Determines the routing for a specific IP
         * \param ip A pointer to the IP to look for. A potential side effect
         *           of this function is for this IP address to be overwritten.
         * \param bGiveDefault If no match is found, return the default route.
         * \return A pointer to the Network device to be used to transmit the
         *         packet.
         */
        Network *DetermineRoute(IpAddress *ip, bool bGiveDefault = true);

        /** Obtains a route by name rather than by address (assuming it has one) */
        Network *DetermineRoute(String name, bool bGiveDefault = true);

        /** Grabs the default route */
        Network *DefaultRoute();

        /** Grabs the default route for IPv6 */
        Network *DefaultRouteV6();

    private:

        static RoutingTable m_Instance;

        bool m_bHasRoutes;

        Mutex m_TableLock;

        /** Used to finalise the determined route */
        Network *route(IpAddress *ip, Config::Result *pResult);
};

#endif
