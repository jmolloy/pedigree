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
#ifndef MACHINE_ROUTINGTABLE_H
#define MACHINE_ROUTINGTABLE_H

#include <utilities/String.h>
#include <utilities/Tree.h>
#include <utilities/List.h>
#include <utilities/RadixTree.h>
#include <process/Semaphore.h>
#include <machine/Network.h>

/** #define'd to keep out of compiles. It's currently only an idea, there's still so much
    to figure out before this sort of concept is ready to go */
#define UNUSUAL_ROUTING_METHOD 0

#if UNUSUAL_ROUTING_METHOD

/** A specific route */
class Route
{
  public:
    Route() : m_Type(SUBNET)
    {};
    virtual ~Route()
    {};

    /** Defines the type of a route.
      * SUBNET means this uses the subnet mask to determine destination
      * RANGE means the route defines a range of IPs
      * IP means a single IP address
      */
    enum RouteType
    {
      SUBNET = 0,
      IP
    };

    /** Gets this route's card */
    Network* getCard(IpAddress incoming)
    {
      switch(m_Type)
      {
        case SUBNET:
          if(incoming.getIp() & 
      return 0;
    }

  private:

    RouteType m_Type;

    Network* m_Card;

    IpAddress m_Subnet;
    IpAddress m_Ip;
};

#endif

/** Routing table implementation */
class RoutingTable
{
  public:
    RoutingTable() :
      m_Table()
    {};
    virtual ~RoutingTable()
    {};

    static RoutingTable& instance()
    {
      return m_Instance;
    }

    /** Adds a route to the table */
    void Add(IpAddress dest, Network* card)
    {
      m_Table.insert(dest.toString(), card);
    }

    /** Adds a named route to the table */
    void AddNamed(String route, Network* card)
    {
      m_Table.insert(route, card);
    }

    /** Removes a route from the table */
    void Remove(IpAddress routeip)
    {
      m_Table.remove(routeip.toString());
    }

    /** Determines the routing for a specific IP */
    Network* DetermineRoute(IpAddress dest, bool bGiveDefault = true)
    {
      // look up the IP
      Network* ret = 0;
      if((ret = m_Table.lookup(dest.toString())))
        return ret;

      // none found so try the default route if needed
      if(bGiveDefault)
        return m_Table.lookup(String("default"));
      else
        return 0;
    }

    /** Gets the default route */
    Network* DefaultRoute()
    {
      return m_Table.lookup(String("default"));
    }
  
  private:

    static RoutingTable m_Instance;

    RadixTree<Network*> m_Table;
};

#endif
