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

#include <Log.h>
#include <Module.h>
#include <network-stack/NetworkStack.h>
#include <network-stack/RoutingTable.h>
#include <machine/DeviceHashTree.h>
#include <processor/Processor.h>
#include <Service.h>
#include <ServiceManager.h>

static int configureInterfaces(void *p)
{
    // Fill out the device hash table (needed in RoutingTable)
    DeviceHashTree::instance().fill(&Device::root());

    // Build routing tables - try to find a default configuration that can
    // connect to the outside world
    IpAddress empty;
    Network *pDefaultCard = 0;
    for (size_t i = 0; i < NetworkStack::instance().getNumDevices(); i++)
    {
        /// \todo Perhaps try and ping a remote host?
        Network* card = NetworkStack::instance().getDevice(i);

        StationInfo info = card->getStationInfo();

        // IPv6 stateless autoconfiguration and DHCP/DHCPv6 must not happen on
        // the loopback device, which has a fixed address.
        if(info.ipv4.getIp() != Network::convertToIpv4(127, 0, 0, 1))
        {
            // Auto-configure IPv6 on this card.
            ServiceFeatures *pFeatures = ServiceManager::instance().enumerateOperations(String("ipv6"));
            Service         *pService  = ServiceManager::instance().getService(String("ipv6"));
            if(pFeatures->provides(ServiceFeatures::touch))
                if(pService)
                    pService->serve(ServiceFeatures::touch, reinterpret_cast<void*>(card), sizeof(*card));

            // Ask for a DHCP lease on this card
            /// \todo Static configuration
            pFeatures = ServiceManager::instance().enumerateOperations(String("dhcp"));
            pService  = ServiceManager::instance().getService(String("dhcp"));
            if(pFeatures->provides(ServiceFeatures::touch))
                if(pService)
                    pService->serve(ServiceFeatures::touch, reinterpret_cast<void*>(card), sizeof(*card));
        }

        StationInfo newInfo = card->getStationInfo();

        // List IPv6 addresses
        for(size_t i = 0; i < info.nIpv6Addresses; i++)
            NOTICE("Interface " << i << " has IPv6 address " << info.ipv6[i].toString() << " (" << Dec << i << Hex << " out of " << info.nIpv6Addresses << ")");

        // If the device has a gateway, set it as the default and continue
        if (newInfo.gateway != empty)
        {
            if(!pDefaultCard)
                pDefaultCard = card;

            // Additionally route the complement of its subnet to the gateway
            RoutingTable::instance().Add(RoutingTable::DestSubnetComplement,
                                         newInfo.ipv4,
                                         newInfo.subnetMask,
                                         newInfo.gateway,
                                         String(""),
                                         card);
        }

        // And the actual subnet that the card is on needs to route to... the card.
        RoutingTable::instance().Add(RoutingTable::DestSubnet,
                newInfo.ipv4,
                newInfo.subnetMask,
                empty,
                String(""),
                card);

        // If this isn't already the loopback device, redirect our own IP to 127.0.0.1
        if(newInfo.ipv4.getIp() != Network::convertToIpv4(127, 0, 0, 1))
            RoutingTable::instance().Add(RoutingTable::DestIpSub, newInfo.ipv4, Network::convertToIpv4(127, 0, 0, 1), String(""), NetworkStack::instance().getLoopback());
        else
            RoutingTable::instance().Add(RoutingTable::DestIp, newInfo.ipv4, empty, String(""), card);
    }

    // Otherwise, just assume the default is interface zero
    if (!pDefaultCard)
        RoutingTable::instance().Add(RoutingTable::Named, empty, empty, String("default"), NetworkStack::instance().getDevice(0));
    else
        RoutingTable::instance().Add(RoutingTable::Named, empty, empty, String("default"), pDefaultCard);

    return 0;
}

static bool init()
{
#ifdef THREADS
    // Spin up network interfaces, but don't wait.
    Process *me = Processor::information().getCurrentThread()->getParent();
    Thread *pInterfaceThread = new Thread(me, configureInterfaces, 0);
    pInterfaceThread->detach();
#else
    configureInterfaces(0);
#endif
    return true;
}

static void destroy()
{
}

MODULE_INFO("confignics", &init, &destroy, "dhcpclient", "network-stack");
