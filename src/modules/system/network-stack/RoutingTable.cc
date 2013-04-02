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
#include <LockGuard.h>

RoutingTable RoutingTable::m_Instance;

RoutingTable::RoutingTable() : m_bHasRoutes(false), m_TableLock(false)
{
}

RoutingTable::~RoutingTable()
{
}

void RoutingTable::Add(Type type, IpAddress dest, IpAddress subIp, String meta, Network *card)
{
    LockGuard<Mutex> guard(m_TableLock);

    // Add to the hash tree, if not already done. This ensures the loopback
    // device is definitely available as an interface.
    DeviceHashTree::instance().add(card);

    Config::Result *pResult = 0;
    size_t hash = DeviceHashTree::instance().getHash(card);

    if((dest.getType() == IpAddress::IPv4) && (type != NamedV6))
    {
        if(type != Named)
        NOTICE("RoutingTable: Adding IPv4 match route for " << dest.toString() << ", sub " << subIp.toString() << ".");

        // Add the route to the database directly
        String str;
        str.sprintf("INSERT INTO routes (ipaddr, subip, name, type, iface) VALUES (%u, %u, '%s', %u, %u)", dest.getIp(), subIp.getIp(), static_cast<const char*>(meta), static_cast<int>(type), hash);
        pResult = Config::instance().query(str);
        if(!pResult->succeeded())
        {
            ERROR("Routing table query failed: " << pResult->errorMessage());
        }
    }
    else
    {
        if(type != NamedV6)
        NOTICE("RoutingTable: Adding IPv6 match route for " << dest.prefixString(128) << ", sub " << subIp.prefixString(128) << ".");

        // Develop the subip parameter - four integers.
        uint32_t subipTemp[4];
        subIp.getIp(reinterpret_cast<uint8_t*>(subipTemp));

        // Add the route to the database
        String str;
        str.sprintf("INSERT INTO routesv6 (ipaddr, subip1, subip2, subip3, subip4, name, type, iface, metric) VALUES ('%s', %u, %u, %u, %u, '%s', %u, %u, %u)",
                    static_cast<const char *>(dest.prefixString(128)),
                    subipTemp[0],
                    subipTemp[1],
                    subipTemp[2],
                    subipTemp[3],
                    static_cast<const char*>(meta),
                    static_cast<int>(type),
                    hash,
                    1); /// Default metric is 1. \todo Make configureable.
        pResult = Config::instance().query(str);
        if(!pResult->succeeded())
        {
            ERROR("Routing table query failed: " << pResult->errorMessage());
        }
    }

    m_bHasRoutes = true;

    delete pResult;
}

void RoutingTable::Add(Type type, IpAddress dest, IpAddress subnet, IpAddress subIp, String meta, Network *card)
{
    LockGuard<Mutex> guard(m_TableLock);

    // Add to the hash tree, if not already done. This ensures the loopback
    // device is definitely available as an interface.
    DeviceHashTree::instance().add(card);

    Config::Result *pResult = 0;

    if(dest.getType() == IpAddress::IPv4 && (type != NamedV6))
    {
        // Invert the subnet to get an IP range
        IpAddress invSubnet(subnet.getIp() ^ 0xFFFFFFFF);
        IpAddress bottomOfRange = dest & subnet;
        IpAddress topOfRange = (dest & subnet) + invSubnet;

        if(type != Named)
            NOTICE("RoutingTable: Adding IPv4 " << (type == DestSubnetComplement ? "complement of " : "") << "subnet match for range " << bottomOfRange.toString() << " - " << topOfRange.toString());

        // Add to the database
        String str;
        size_t hash = DeviceHashTree::instance().getHash(card);
        str.sprintf("INSERT INTO routes (ipstart, ipend, subip, name, type, iface) VALUES (%u, %u, %u, '%s', %u, %u)",
                    BIG_TO_HOST32(bottomOfRange.getIp()),
                    BIG_TO_HOST32(topOfRange.getIp()),
                    BIG_TO_HOST32(subIp.getIp()),
                    static_cast<const char*>(meta),
                    static_cast<int>(type),
                    hash);
        pResult = Config::instance().query(str);
        if(!pResult->succeeded())
        {
            ERROR("Routing table query failed: " << pResult->errorMessage());
        }
    }
    else
    {
        // Add it in.
        // Note - 'subnet' parameter ignored.
        if(type != NamedV6)
            NOTICE("RoutingTable: Adding IPv6 " << (type == DestPrefixComplement ? "complement of " : "") << "prefix match for " << dest.prefixString());

        // Develop the subip parameter - four integers.
        uint32_t subipTemp[4];
        subIp.getIp(reinterpret_cast<uint8_t*>(subipTemp));

        // Add to the database
        String str;
        size_t hash = DeviceHashTree::instance().getHash(card);
        str.sprintf("INSERT INTO routesv6 (prefix, subip1, subip2, subip3, subip4, prefixNum, name, type, iface, metric) VALUES ('%s', %u, %u, %u, %u, %u, '%s', %u, %u, %u)",
                    static_cast<const char*>(dest.prefixString()),
                    subipTemp[0],
                    subipTemp[1],
                    subipTemp[2],
                    subipTemp[3],
                    dest.getIpv6Prefix(),
                    static_cast<const char*>(meta),
                    static_cast<int>(type),
                    hash,
                    1024); /// Default metric is 1024. \todo Make configurable.
        pResult = Config::instance().query(str);
        if(!pResult->succeeded())
        {
            ERROR("Routing table query failed: " << pResult->errorMessage());
        }
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
        ip->setIp(HOST_TO_BIG32(pResult->getNum(0, "subip")));
    else if(t == DestIpv6Sub || t == DestPrefixComplement)
    {
        uint32_t subIp[4];
        subIp[0] = pResult->getNum(0, "subip1");
        subIp[1] = pResult->getNum(0, "subip2");
        subIp[2] = pResult->getNum(0, "subip3");
        subIp[3] = pResult->getNum(0, "subip4");

        ip->setIp(reinterpret_cast<uint8_t*>(subIp));
    }

    // Return the interface to use
    delete pResult;
    return pCard;
}

Network *RoutingTable::DetermineRoute(IpAddress *ip, bool bGiveDefault)
{
    LockGuard<Mutex> guard(m_TableLock);

    // Use the IPv6 route table?
    if(ip->getType() == IpAddress::IPv6)
    {
        // Can we directly match a route (/128)?
        String str;
        str.sprintf("SELECT * FROM routesv6 WHERE ipaddr='%s'", static_cast<const char *>(ip->prefixString(128))); // prefixString doesn't add /xx on the end.
        Config::Result *pResult = Config::instance().query(str);
        if(!pResult->succeeded())
            ERROR("Routing table query failed: " << pResult->errorMessage());
        else if(pResult->rows())
        {
            return route(ip, pResult);
        }
        delete pResult;

        // Try a prefix lookup. This involves finding all the potential prefix numbers and then
        // using those to find a usable prefix.
        str.sprintf("SELECT id, prefixNum FROM routesv6 WHERE type = %u ORDER BY metric ASC", static_cast<int>(DestPrefix));
        pResult = Config::instance().query(str);
        if(!pResult->succeeded())
            ERROR("Routing table query failed: " << pResult->errorMessage());
        else if(pResult->rows())
        {
            // Got a bunch of prefixes!
            for(size_t i = 0; i < pResult->rows(); i++)
            {
                // Check this prefix.
                size_t prefixNum = pResult->getNum(i, "prefixNum");
                str.sprintf("SELECT * FROM routesv6 WHERE prefix = '%s' AND type = %u ORDER BY metric ASC", static_cast<const char *>(ip->prefixString(prefixNum)), static_cast<int>(DestPrefix));
                Config::Result *pTempResult = Config::instance().query(str);
                if(!pTempResult->succeeded())
                    continue;
                else if(pTempResult->rows())
                {
                    delete pResult;
                    return route(ip, pTempResult);
                }
            }
        }
        else
            NOTICE("No prefixes");
        delete pResult;

        // Still nothing, try a complement prefix search
        str.sprintf("SELECT * FROM routesv6 WHERE (NOT prefix='%s') AND type=%u ORDER BY metric ASC", static_cast<const char *>(ip->prefixString()), static_cast<int>(DestPrefixComplement));
        pResult = Config::instance().query(str);
        if(!pResult->succeeded())
            ERROR("Routing table query failed: " << pResult->errorMessage());
        else if(pResult->rows())
        {
            return route(ip, pResult);
        }
        delete pResult;

        // Nothing even still, try the default route if we're allowed
        if(bGiveDefault)
        {
            return DefaultRouteV6();
        }
    }
    else
    {
        // Build a query to search for a direct match
        String str;
        str.sprintf("SELECT * FROM routes WHERE ipaddr=%u", BIG_TO_HOST32(ip->getIp()));
        Config::Result *pResult = Config::instance().query(str);
        if(!pResult->succeeded())
            ERROR("Routing table query failed: " << pResult->errorMessage());
        else if(pResult->rows())
        {
            return route(ip, pResult);
        }
        delete pResult;

        // No rows! Try a subnet lookup first, without a complement.
        str.sprintf("SELECT * FROM routes WHERE ((ipstart <= %u) AND (ipend >= %u)) AND (type == %u)", BIG_TO_HOST32(ip->getIp()), BIG_TO_HOST32(ip->getIp()), static_cast<int>(DestSubnet));
        pResult = Config::instance().query(str);
        if(!pResult->succeeded())
            ERROR("Routing table query failed: " << pResult->errorMessage());
        else if(pResult->rows())
        {
            return route(ip, pResult);
        }
        delete pResult;

        // Still nothing, try a complement subnet search
        str.sprintf("SELECT * FROM routes WHERE (NOT ((ipstart <= %u) AND (ipend >= %u))) AND (type == %u)", BIG_TO_HOST32(ip->getIp()), BIG_TO_HOST32(ip->getIp()), static_cast<int>(DestSubnetComplement));
        pResult = Config::instance().query(str);
        if(!pResult->succeeded())
            ERROR("Routing table query failed: " << pResult->errorMessage());
        else if(pResult->rows())
        {
            return route(ip, pResult);
        }
        delete pResult;

        // Nothing even still, try the default route if we're allowed
        if(bGiveDefault)
        {
            return DefaultRoute();
        }
    }

    // Not even the default route worked!
    return 0;
}

Network *RoutingTable::DefaultRoute()
{
    // If already locked, will return false, so we don't unlock (DetermineRoute calls this function)
    bool bLocked = m_TableLock.tryAcquire();

    String str;
    str.sprintf("SELECT * FROM routes WHERE name='default'");
    Config::Result *pResult = Config::instance().query(str);
    if(!pResult->succeeded())
        ERROR("Routing table query failed: " << pResult->errorMessage());
    else if(pResult->rows())
    {
        if(bLocked)
            m_TableLock.release();
        return route(0, pResult);
    }

    delete pResult;

    if(bLocked)
        m_TableLock.release();

    return 0;
}

Network *RoutingTable::DefaultRouteV6()
{
    // If already locked, will return false, so we don't unlock (DetermineRoute calls this function)
    bool bLocked = m_TableLock.tryAcquire();

    String str;
    str.sprintf("SELECT * FROM routesv6 WHERE name='default'");
    Config::Result *pResult = Config::instance().query(str);
    if(!pResult->succeeded())
        ERROR("Routing table query failed: " << pResult->errorMessage());
    else if(pResult->rows())
    {
        if(bLocked)
            m_TableLock.release();
        return route(0, pResult);
    }

    delete pResult;

    if(bLocked)
        m_TableLock.release();

    return 0;
}

