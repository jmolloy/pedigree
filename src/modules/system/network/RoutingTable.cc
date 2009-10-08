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

#include "RoutingTable.h"
#include <config/Config.h>
#include <machine/DeviceHashTree.h>

RoutingTable RoutingTable::m_Instance;

RoutingTable::RoutingTable() : m_bHasRoutes(false)
{
}

RoutingTable::~RoutingTable()
{
}

void RoutingTable::Add(Type type, IpAddress dest, IpAddress subIp, String meta, Network *card)
{
    // Add the route to the database directly
    String str;
    size_t hash = DeviceHashTree::instance().getHash(card);
    str.sprintf("INSERT INTO routes (ipaddr, subip, name, type, iface) VALUES (%d, %d, '%s', %d, %d)", dest.getIp(), subIp.getIp(), static_cast<const char*>(meta), static_cast<int>(type), hash);
    Config::Result *pResult = Config::instance().query(str);
    if(!pResult->succeeded())
    {
        ERROR("Routing table query failed: " << pResult->errorMessage());
    }

    m_bHasRoutes = true;

    delete pResult;
}

void RoutingTable::Add(Type type, IpAddress dest, IpAddress subnet, IpAddress subIp, String meta, Network *card)
{
    // Invert the subnet to get an IP range
    IpAddress invSubnet(subnet.getIp() ^ 0xFFFFFFFF);
    IpAddress bottomOfRange = dest & subnet;
    IpAddress topOfRange = (dest & subnet) + invSubnet;

    NOTICE("Adding " << (type == DestSubnetComplement ? "complement of " : "") << " subnet match for range " << bottomOfRange.toString() << "-" << topOfRange.toString());

    // Add to the database
    String str;
    size_t hash = DeviceHashTree::instance().getHash(card);
    str.sprintf("INSERT INTO routes (ipstart, ipend, subip, name, type, iface) VALUES (%d, %d, %d, '%s', %d, %d)",
                bottomOfRange.getIp(),
                topOfRange.getIp(),
                subIp.getIp(),
                static_cast<const char*>(meta),
                static_cast<int>(type),
                hash);
    Config::Result *pResult = Config::instance().query(str);
    if(!pResult->succeeded())
    {
        ERROR("Routing table query failed: " << pResult->errorMessage());
    }

    m_bHasRoutes = true;

    delete pResult;
}

Network *RoutingTable::route(IpAddress *ip, Config::Result *pResult)
{
    // Grab the interface
    Network *pCard = static_cast<Network*>(DeviceHashTree::instance().getDevice(pResult->getNum(0, "iface")));

    // If we are to perform substitution, do so
    Type t = static_cast<Type>(pResult->getNum(0, "type"));
    if(t == DestIpSub || t == DestSubnetComplement)
        ip->setIp(pResult->getNum(0, "subip"));

    // Return the interface to use
    delete pResult;
    return pCard;
}

Network *RoutingTable::DetermineRoute(IpAddress *ip, bool bGiveDefault)
{
    // Build a query to search for a direct match
    String str;
    str.sprintf("SELECT * FROM routes WHERE ipaddr=%d", ip->getIp());
    Config::Result *pResult = Config::instance().query(str);
    if(!pResult->succeeded())
        ERROR("Routing table query failed: " << pResult->errorMessage());
    else if(pResult->rows())
    {
        NOTICE("RoutingTable: Direct match found");
        return route(ip, pResult);
    }
    delete pResult;

    // No rows! Try a subnet lookup first, without a complement.
    str.sprintf("SELECT * FROM routes WHERE (ipstart <= %d AND ipend >= %d)", ip->getIp(), ip->getIp());
    pResult = Config::instance().query(str);
    if(!pResult->succeeded())
        ERROR("Routing table query failed: " << pResult->errorMessage());
    else if(pResult->rows())
    {
        NOTICE("RoutingTable: Subnet match found");
        return route(ip, pResult);
    }
    delete pResult;

    // Still nothing, try a complement subnet search
    str.sprintf("SELECT * FROM routes WHERE NOT (ipstart <= %d AND ipend >= %d)", ip->getIp(), ip->getIp());
    pResult = Config::instance().query(str);
    if(!pResult->succeeded())
        ERROR("Routing table query failed: " << pResult->errorMessage());
    else if(pResult->rows())
    {
        NOTICE("RoutingTable: Subnet complement match found");
        return route(ip, pResult);
    }
    delete pResult;

    // Nothing even still, try the default route if we're allowed
    if(bGiveDefault)
        return DefaultRoute();

    // Not even the default route worked!
    return 0;
}

Network *RoutingTable::DefaultRoute()
{
    String str;
    str.sprintf("SELECT * FROM routes WHERE name='default'");
    Config::Result *pResult = Config::instance().query(str);
    if(!pResult->succeeded())
        ERROR("Routing table query failed: " << pResult->errorMessage());
    else if(pResult->rows())
    {
        NOTICE("RoutingTable: Default route used");
        return route(0, pResult);
    }
    delete pResult;

    return 0;
}
