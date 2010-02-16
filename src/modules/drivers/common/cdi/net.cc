/*
 * Copyright (c) 2007 Kevin Wolf
 *
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it 
 * and/or modify it under the terms of the Do What The Fuck You Want 
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/projects/COPYING.WTFPL for more details.
 */

#include <stdint.h>
#include <cdi/lists.h>
#include <cdi/net.h>
#include <cdi/pci.h>
#include <Log.h>

static unsigned long netcard_highest_id = 0;
static cdi_list_t netcard_list;
// FIXME: Sollten wahrscheinlich pro device sein und nicht nur pro driver
static cdi_list_t ethernet_packet_receivers;

static bool bListsDefined = false;

typedef unsigned long dword;

extern "C"
{
    void cdi_cpp_net_register(void* void_pdev, struct cdi_net_device* dev);
};

/*
static void rpc_send_packet
    (pid_t pid, dword cid, size_t data_size, void* data);
static void rpc_register_receiver
    (pid_t pid, dword cid, size_t data_size, void* data);
*/

/**
 * Initialisiert die Datenstrukturen fuer einen Netzerktreiber
 */
void cdi_net_driver_init(struct cdi_net_driver* driver)
{
    driver->drv.type = CDI_NETWORK;
    cdi_driver_init(reinterpret_cast<struct cdi_driver*>(driver));
    
    if(!bListsDefined)
    {
        netcard_list = cdi_list_create();
        ethernet_packet_receivers = cdi_list_create();
        bListsDefined = true;
    }

//    register_message_handler(RPC_ETHERNET_SEND_PACKET, rpc_send_packet);
//    register_message_handler(RPC_ETHERNET_REGISTER_RECEIVER, rpc_register_receiver);
}

/**
 * Deinitialisiert die Datenstrukturen fuer einen Netzwerktreiber
 */
void cdi_net_driver_destroy(struct cdi_net_driver* driver)
{
    cdi_driver_destroy(reinterpret_cast<struct cdi_driver*>(driver));
}

/**
 * Initialisiert eine neue Netzwerkkarte
 */
void cdi_net_device_init(struct cdi_net_device* device)
{
    device->number = netcard_highest_id;

    // Zur Liste der Netzwerkkarten hinzufügen
    cdi_list_push(netcard_list, device);

    // Beim tcpip Modul registrieren
    void *pDev = 0;
    cdi_device_type_t type = device->dev.bus_data->bus_type;
    if(type == CDI_PCI)
    {
        pDev = reinterpret_cast<struct cdi_pci_device*>(device->dev.bus_data)->meta.backdev;
    }
    if(!pDev)
        return;
    cdi_cpp_net_register(pDev, device);

    ++netcard_highest_id;
}

/**
 * Gibt die Netzwerkkarte mit der uebergebenen Geraetenummer
 * zureck
 */
struct cdi_net_device* cdi_net_get_device(int num)
{
    for (size_t i = 0; i < cdi_list_size(netcard_list); i++)
    {
        struct cdi_net_device *device = reinterpret_cast<struct cdi_net_device*>(cdi_list_get(netcard_list, i));
        if (device->number == num)
            return device;
    }
    return NULL;
}
