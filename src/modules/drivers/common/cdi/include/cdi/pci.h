/*
 * Copyright (c) 2007 Kevin Wolf
 *
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it 
 * and/or modify it under the terms of the Do What The Fuck You Want 
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/projects/COPYING.WTFPL for more details.
 */  

#ifndef _CDI_PCI_H_
#define _CDI_PCI_H_

//#include <stdint.h>
#include <processor/types.h>

#include "cdi.h"
#include "cdi/lists.h"

struct cdi_pci_device {
    uint16_t    bus;
    uint16_t    dev;
    uint16_t    function;

    uint16_t    vendor_id;
    uint16_t    device_id;

    uint8_t     class_id;
    uint8_t     subclass_id;
    uint8_t     interface_id;

    uint8_t     rev_id;

    uint8_t     irq;

    cdi_list_t resources;

    // Pedigree specific
    void*       pDev;
};

typedef enum {
    CDI_PCI_MEMORY,
    CDI_PCI_IOPORTS
} cdi_res_t;

struct cdi_pci_resource {
    cdi_res_t    type;
    uintptr_t    start;
    size_t       length;
    unsigned int index;
    void*        address;
};


/**
 * Gibt alle PCI-Geraete im System zurueck. Die Geraete werden dazu
 * in die uebergebene Liste eingefuegt.
 */
void cdi_pci_get_all_devices(cdi_list_t list);

/**
 * Gibt die Information zu einem PCI-Geraet frei
 */
void cdi_pci_device_destroy(struct cdi_pci_device* device);

/**
 * Reserviert die IO-Ports des PCI-Geraets fuer den Treiber
 */
void cdi_pci_alloc_ioports(struct cdi_pci_device* device);

/**
 * Gibt die IO-Ports des PCI-Geraets frei
 */
void cdi_pci_free_ioports(struct cdi_pci_device* device);

/**
 * Reserviert den MMIO-Speicher des PCI-Geraets fuer den Treiber
 */
void cdi_pci_alloc_memory(struct cdi_pci_device* device);

/**
 * Gibt den MMIO-Speicher des PCI-Geraets frei
 */
void cdi_pci_free_memory(struct cdi_pci_device* device);

#endif

