/*
 * Copyright (c) 2007-2009 Kevin Wolf
 *
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it 
 * and/or modify it under the terms of the Do What The Fuck You Want 
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/projects/COPYING.WTFPL for more details.
 */

#include <machine/Device.h>

extern "C" {

#include <stdint.h>
#include <stdlib.h>

#include "cdi.h"
#include "cdi/lists.h"
#include "cdi/pci.h"

static void add_child_device(cdi_list_t list, Device *pDev)
{
    // Add device to list
    // Let's hope the available information is enough
    struct cdi_pci_device* dev = (struct cdi_pci_device*) malloc(sizeof(*dev));
    ByteSet(dev, 0, sizeof(*dev));

    dev->vendor_id = pDev->getPciVendorId();
    dev->device_id = pDev->getPciDeviceId();
    dev->class_id = pDev->getPciClassCode();
    dev->subclass_id = pDev->getPciSubclassCode();
    dev->irq = pDev->getInterruptNumber();
    dev->meta.backdev = reinterpret_cast<void*>(pDev);

    // Add BARs
    Vector<Device::Address*> addresses = pDev->addresses();
    dev->resources = cdi_list_create();
    for (unsigned int j = 0; j < addresses.size(); j++) {
        Device::Address* bar = addresses[j];

        struct cdi_pci_resource* res = (struct cdi_pci_resource*) malloc(sizeof(*res));
        ByteSet(res, 0, sizeof(*res));

        res->type = bar->m_IsIoSpace ? CDI_PCI_IOPORTS : CDI_PCI_MEMORY;
        res->start = bar->m_Address;
        res->length = bar->m_Size;
        res->index = j;
        res->address = 0;

        cdi_list_push(dev->resources, res);
    }

    cdi_list_push(list, dev);
}

/**
 * Gibt alle PCI-Geraete im System zurueck. Die Geraete werden dazu
 * in die uebergebene Liste eingefuegt.
 */
void cdi_pci_get_all_devices(cdi_list_t list)
{
    auto f = [list] (Device *p) {
        add_child_device(list, p);
        return p;
    };

    auto c = pedigree_std::make_callable(f);
    Device::foreach(c, 0);
}

/**
 * Gibt die Information zu einem PCI-Geraet frei
 */
void cdi_pci_device_destroy(struct cdi_pci_device* device)
{
    // TODO Liste abbauen
    free(device);
}

/**
 * Reserviert die IO-Ports des PCI-Geraets fuer den Treiber
 */
void cdi_pci_alloc_ioports(struct cdi_pci_device* device)
{
    struct cdi_pci_resource* res;
    int i = 0;

    for (i = 0; (res = (struct cdi_pci_resource*) cdi_list_get(device->resources, i)); i++) {
        if (res->type == CDI_PCI_IOPORTS) {
//            request_ports(res->start, res->length);
        }
    }
}

/**
 * Gibt die IO-Ports des PCI-Geraets frei
 */
void cdi_pci_free_ioports(struct cdi_pci_device* device)
{
    struct cdi_pci_resource* res;
    int i = 0;

    for (i = 0; (res = (struct cdi_pci_resource*)cdi_list_get(device->resources, i)); i++) {
        if (res->type == CDI_PCI_IOPORTS) {
//            release_ports(res->start, res->length);
        }
    }
}
/**
 * Reserviert den MMIO-Speicher des PCI-Geraets fuer den Treiber
 */
void cdi_pci_alloc_memory(struct cdi_pci_device* device)
{
    struct cdi_pci_resource* res;
    int i = 0;

    for (i = 0; (res = (struct cdi_pci_resource*)cdi_list_get(device->resources, i)); i++) {
        if (res->type == CDI_PCI_MEMORY) {
// TODO            res->address = mem_allocate_physical(res->length, res->start, 0);
        }
    }
}

/**
 * Gibt den MMIO-Speicher des PCI-Geraets frei
 */
void cdi_pci_free_memory(struct cdi_pci_device* device)
{
    struct cdi_pci_resource* res;
    int i = 0;

    for (i = 0; (res = (struct cdi_pci_resource*)cdi_list_get(device->resources, i)); i++) {
        if (res->type == CDI_PCI_MEMORY) {
// TODO            mem_free_physical(res->address, res->length);
            res->address = 0;
        }
    }
}

}
