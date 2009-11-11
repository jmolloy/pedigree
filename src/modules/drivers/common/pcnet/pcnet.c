/*
 * Copyright (c) 2008 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
 * by Jörg Pfähler.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "pcnet.h"
#include "cdi/io.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static void pcnet_handle_interrupt(struct cdi_device* device);
static void pcnet_reset(struct pcnet_device *netcard);
static void pcnet_start(struct pcnet_device *netcard);
static void pcnet_stop(struct pcnet_device *netcard);
static void pcnet_dev_init(struct pcnet_device *netcard, int promiscuous);
static uint16_t pcnet_read_csr(struct pcnet_device *netcard, size_t csr);
static void pcnet_write_csr(struct pcnet_device *netcard, size_t csr, uint16_t value);
/*static uint16_t pcnet_read_bcr(struct pcnet_device *netcard, size_t bcr);*/
static void pcnet_write_bcr(struct pcnet_device *netcard, size_t bcr, uint16_t value);

//Hier koennen die Debug-Nachrichten aktiviert werden
#define DEBUG

#ifdef DEBUG
    #define DEBUG_MSG(s) printf("[PCNET] debug: %s() '%s'\n", __FUNCTION__, s)
#else
    #define DEBUG_MSG(s) //
#endif

void pcnet_init_device(struct cdi_device* device)
{
    struct pcnet_device* netcard = (struct pcnet_device*) device;
    netcard->net.send_packet = pcnet_send_packet;

    // PCI-bezogenes Zeug initialisieren
    DEBUG_MSG("Interrupthandler und Ports registrieren");
    cdi_register_irq(netcard->pci->irq, pcnet_handle_interrupt, device);
    cdi_pci_alloc_ioports(netcard->pci);

    // I/O Port raussuchen
    cdi_list_t reslist = netcard->pci->resources;
    struct cdi_pci_resource* res;
    int i;
    for (i = 0; (res = cdi_list_get(reslist, i)); i++) {
        if (res->type == CDI_PCI_IOPORTS) {
            netcard->port_base = res->start;
        }
    }

    #ifdef DEBUG
        printf("pcnet: I/O port @ %x\n", netcard->port_base);
    #endif

    // Read MAC address
    uint16_t mac0 = cdi_inw(netcard->port_base + APROM0);
    uint16_t mac1 = cdi_inw(netcard->port_base + APROM2);
    uint16_t mac2 = cdi_inw(netcard->port_base + APROM4);
    netcard->net.mac = mac0 | ((uint64_t) mac1 << 16) | ((uint64_t) mac2 << 32);
    #ifdef DEBUG
        printf("pcnet: MAC %x.%x.%x.%x.%x.%x\n", mac0 & 0xFF, mac0 >> 8, mac1 & 0xFF, mac1 >> 8, mac2 & 0xFF, mac2 >> 8);
    #endif

    // Reset ausführen
    pcnet_reset(netcard);

    // Stoppen NOTE Warum eigentlich? keine Ahnung
    pcnet_stop(netcard);

    // Initialisieren
    pcnet_dev_init(netcard, 0);

    // Starten!
    pcnet_start(netcard);

    // Netzwerkkarte registrieren
    cdi_net_device_init((struct cdi_net_device*) device);

    DEBUG_MSG("Fertig initialisiert");
}

void pcnet_remove_device(struct cdi_device* device)
{
    // TODO
    DEBUG_MSG("TODO");
}

void pcnet_send_packet
    (struct cdi_net_device* device, void* data, size_t size)
{
    struct pcnet_device *netcard = (struct pcnet_device*) device;

    if (netcard->last_transmit_descriptor > 7)netcard->last_transmit_descriptor = 0;
    
    // Copy data
    memcpy( netcard->transmit_buffer[netcard->last_transmit_descriptor],
            data,
            size);
    
    // Setup the descriptor
    netcard->transmit_descriptor[netcard->last_transmit_descriptor].res = 0;
    netcard->transmit_descriptor[netcard->last_transmit_descriptor].flags2 = 0;
    size_t flags = DESCRIPTOR_OWN | DESCRIPTOR_PACKET_START| DESCRIPTOR_PACKET_END| (uint16_t) ((-size) & 0xFFFF);
    netcard->transmit_descriptor[netcard->last_transmit_descriptor].flags = flags;
    
    // Do the transfer
    pcnet_write_csr(netcard, CSR_STATUS, STATUS_INTERRUPT_ENABLE| STATUS_TRANSMIT_DEMAND);
    
    ++netcard->last_transmit_descriptor;
    if (netcard->last_transmit_descriptor == 8)netcard->last_transmit_descriptor = 0;
}

static void pcnet_handle_interrupt(struct cdi_device* device)
{
    struct pcnet_device* netcard = (struct pcnet_device*) device;

    if (netcard->init_wait_for_irq == 1)
    {
        uint16_t csr0 = pcnet_read_csr(netcard, CSR_STATUS);
        pcnet_write_csr(netcard, CSR_STATUS, csr0);
        DEBUG_MSG("Interrupt (wait for irq)");
        netcard->init_wait_for_irq = 0;
        return;
    }

    uint16_t csr0 = pcnet_read_csr(netcard, CSR_STATUS);
    if ((csr0 & STATUS_ERROR) != 0)
    {
        pcnet_stop(netcard);
        printf("pcnet: error (csr0 = %x)\n", csr0);
    }
    if ((csr0 & STATUS_TRANSMIT_INTERRUPT) != 0)
    {
        // loop until all frames are captured
        while ( netcard->last_transmit_descriptor_eval != netcard->last_transmit_descriptor &&
                (netcard->transmit_descriptor[netcard->last_transmit_descriptor_eval].flags & DESCRIPTOR_OWN) == 0)
        {
            if ((netcard->transmit_descriptor[netcard->last_transmit_descriptor_eval].flags & DESCRIPTOR_ERROR) != 0)
            {
                printf("pcnet: transmit error (descriptor flags 0x%x flags2 0x%x)\n",
                       netcard->transmit_descriptor[netcard->last_transmit_descriptor_eval].flags,
                       netcard->transmit_descriptor[netcard->last_transmit_descriptor_eval].flags2);
            }

            //TODO: Send a reply to the tcpip module (packet successfully send)
            netcard->transmit_descriptor[netcard->last_transmit_descriptor_eval].flags = 0;
            netcard->transmit_descriptor[netcard->last_transmit_descriptor_eval].flags2 = 0;
            ++netcard->last_transmit_descriptor_eval;
            if (netcard->last_transmit_descriptor_eval == 8)netcard->last_transmit_descriptor_eval = 0;
        }

        // Clear the interrupt request
        pcnet_write_csr(netcard, CSR_STATUS, STATUS_INTERRUPT_ENABLE| STATUS_TRANSMIT_INTERRUPT);
    }
    if ((csr0 & STATUS_RECEIVE_INTERRUPT) != 0)
    {
        // loop until all frames are captured
        while ((netcard->receive_descriptor[netcard->last_receive_descriptor].flags & DESCRIPTOR_OWN) == 0)
        {
            if ((netcard->receive_descriptor[netcard->last_receive_descriptor].flags & DESCRIPTOR_ERROR) != 0 ||
                (netcard->receive_descriptor[netcard->last_receive_descriptor].flags & (DESCRIPTOR_PACKET_START | DESCRIPTOR_PACKET_END)) != (DESCRIPTOR_PACKET_START | DESCRIPTOR_PACKET_END))
            {
                printf("pcnet: receive error (descriptor flags 0x%x)\n", netcard->receive_descriptor[netcard->last_receive_descriptor].flags);
            }
            else
            {
                size_t size = netcard->receive_descriptor[netcard->last_receive_descriptor].flags2 & 0xFFFF;
                if (size > 64)size -= 4;
                cdi_net_receive((struct cdi_net_device*) netcard,
                                netcard->receive_buffer[netcard->last_receive_descriptor],
                                size);
            }
            netcard->receive_descriptor[netcard->last_receive_descriptor].flags = DESCRIPTOR_OWN | 0xF7FF;
            netcard->receive_descriptor[netcard->last_receive_descriptor].flags2 = 0;
            netcard->last_receive_descriptor++;
            if (netcard->last_receive_descriptor == 8)netcard->last_receive_descriptor = 0;
        }
        
        // Clear the interrupt request
        pcnet_write_csr(netcard, CSR_STATUS, STATUS_INTERRUPT_ENABLE | STATUS_RECEIVE_INTERRUPT);
    }
}

static void pcnet_reset(struct pcnet_device *netcard)
{
    cdi_inw(netcard->port_base + RESET);
    cdi_outw(netcard->port_base + RESET, 0);
    cdi_sleep_ms(10);

    // Set software style to PCnet-PCI 32bit
    pcnet_write_bcr(netcard, BCR_SOFTWARE_STYLE, 0x0102);

    DEBUG_MSG("Reset");
}
void pcnet_start(struct pcnet_device *netcard)
{
    pcnet_write_csr(netcard, CSR_STATUS, STATUS_START | STATUS_INTERRUPT_ENABLE);
}
void pcnet_stop(struct pcnet_device *netcard)
{
    pcnet_write_csr(netcard, CSR_STATUS, STATUS_STOP);
}
void pcnet_dev_init(struct pcnet_device *netcard, int promiscuous)
{
    // Allocate the receive & transmit descriptor buffer
    void *virt_desc_region;
    void *phys_desc_region;
    if (cdi_alloc_phys_mem(4096, &virt_desc_region, &phys_desc_region) == -1)
    {
        printf("pcnet: failed to allocate descriptor region\n");
        return;
    }

    #ifdef DEBUG
        printf("pcnet: descriptor region at 0x%p virtual and 0x%p physical\n", virt_desc_region, phys_desc_region);
    #endif
    netcard->receive_descriptor = virt_desc_region;
    netcard->transmit_descriptor = (struct transmit_descriptor*) (((char*)virt_desc_region) + 2 * 1024);
    netcard->phys_receive_descriptor = phys_desc_region;
    netcard->phys_transmit_descriptor = (void*) (((char*)phys_desc_region) + 2 * 1024);

    // Fill the initialization block
    // NOTE: Transmit and receive buffer contain 8 entries
    cdi_alloc_phys_mem(sizeof(struct initialization_block),
                       (void**) &netcard->initialization_block,
                       &netcard->phys_initialization_block);
    memset(netcard->initialization_block, 0, sizeof(struct initialization_block));
    netcard->initialization_block->mode = promiscuous ? PROMISCUOUS_MODE : 0;
    netcard->initialization_block->receive_length = 0x30;
    netcard->initialization_block->transfer_length = 0x30;
    netcard->initialization_block->receive_descriptor = (uint32_t) netcard->phys_receive_descriptor;
    netcard->initialization_block->transmit_descriptor = (uint32_t) netcard->phys_transmit_descriptor;
    netcard->initialization_block->physical_address = netcard->net.mac;

    #ifdef DEBUG
        printf("pcnet: initialization block at 0x%p virtual and 0x%p physical\n",
               netcard->initialization_block,
               netcard->phys_initialization_block);
    #endif
    
    // Allocate the buffers
    memset(netcard->transmit_descriptor, 0, 2048);
    void *virt_buffer;
    void *phys_buffer;
    int i = 0;
    for (;i < 4;i++)
    {
        if (cdi_alloc_phys_mem(4096, &virt_buffer, &phys_buffer) == -1)
        {
            DEBUG_MSG("cdi_alloc_phys_mem failed");
            return;
        }
        netcard->receive_buffer[2 * i] = virt_buffer;
        netcard->receive_buffer[2 * i + 1] = ((char*)virt_buffer) + 2048;
        netcard->receive_descriptor[2 * i].address = (uint32_t) phys_buffer;
        netcard->receive_descriptor[2 * i].flags = DESCRIPTOR_OWN | 0xF7FF;
        netcard->receive_descriptor[2 * i].flags2 = 0;
        netcard->receive_descriptor[2 * i].res = 0;
        netcard->receive_descriptor[2 * i + 1].address = (uint32_t) phys_buffer + 2048;
        netcard->receive_descriptor[2 * i + 1].flags = DESCRIPTOR_OWN | 0xF7FF;
        netcard->receive_descriptor[2 * i + 1].flags2 = 0;
        netcard->receive_descriptor[2 * i + 1].res = 0;

        if (cdi_alloc_phys_mem(4096, &virt_buffer, &phys_buffer) == -1)
        {
            DEBUG_MSG("cdi_alloc_phys_mem failed");
            return;
        }
        netcard->transmit_buffer[2 * i] = virt_buffer;
        netcard->transmit_buffer[2 * i + 1] = ((char*)virt_buffer) + 2048;
        netcard->transmit_descriptor[2 * i].address = (uint32_t) phys_buffer;
        netcard->transmit_descriptor[2 * i + 1].address = (uint32_t) phys_buffer + 2048;
    }

    // Set the address for the initialization block
    pcnet_write_csr(netcard, 1, (uintptr_t) netcard->phys_initialization_block);
    pcnet_write_csr(netcard, 2, (uintptr_t) netcard->phys_initialization_block >> 16);
    
    // TODO BCR18.BREADE / BWRITE
    
    // CSR0.INIT, Initialize
    netcard->init_wait_for_irq = 1;
    pcnet_write_csr(netcard, CSR_STATUS, STATUS_INIT | STATUS_INTERRUPT_ENABLE);
    
    // Wait until initialization completed, NOTE: see pcnet_handle_interrupt()
    int irq = cdi_wait_irq(netcard->pci->irq, 1000);
    if(irq < 0 || netcard->init_wait_for_irq == 1)
        printf("pcnet: waiting for IRQ failed: %d\n", irq);
    
    // CSR4 Enable transmit auto-padding and receive automatic padding removal
    // TODO DMAPLUS? 
    uint16_t tmp = pcnet_read_csr(netcard, CSR_FEATURE);
    pcnet_write_csr(netcard, CSR_FEATURE, tmp | FEATURE_AUTOSTRIP_RECEIVE| FEATURE_AUTOPAD_TRANSMIT);
    /* CSR46/47 Polling? */
    /* CSR80/82 ? */
    
    DEBUG_MSG("pcnet: Initialization done");
}

static uint16_t pcnet_read_csr(struct pcnet_device *netcard, size_t csr)
{
    cdi_outw(netcard->port_base + RAP, csr);
    return cdi_inw(netcard->port_base + RDP);
}
static void pcnet_write_csr(struct pcnet_device *netcard, size_t csr, uint16_t value)
{
    cdi_outw(netcard->port_base + RAP, csr);
    cdi_outw(netcard->port_base + RDP, value);
}
/*static uint16_t pcnet_read_bcr(struct pcnet_device *netcard, size_t bcr)
{
    cdi_outw(netcard->port_base + RAP, bcr);
    return cdi_inw(netcard->port_base + BDP);
}*/
static void pcnet_write_bcr(struct pcnet_device *netcard, size_t bcr, uint16_t value)
{
    cdi_outw(netcard->port_base + RAP, bcr);
    cdi_outw(netcard->port_base + BDP, value);
}
