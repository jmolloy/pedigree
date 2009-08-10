/*
 * Copyright (c) 2007, 2008 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
 * by Kevin Wolf.
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

#include <stdint.h>
#include <stdio.h>
#include <stddef.h>

#include "cdi.h"
#include "cdi/misc.h"

#include "device.h"
#include "e1000_io.h"

#define DEBUG

#define PHYS(netcard, field) \
    ((uintptr_t) netcard->phys + offsetof(struct e1000_device, field))

static void e1000_handle_interrupt(struct cdi_device* device);
static uint64_t get_mac_address(struct e1000_device* device);

static uint16_t e1000_eeprom_read(struct e1000_device* device, uint16_t offset)
{
    uint32_t eerd;

    reg_outl(device, REG_EEPROM_READ, (offset << 8) | EERD_START);
    while (((eerd = reg_inl(device, REG_EEPROM_READ)) & EERD_DONE) == 0);
    printf("eerd = %8x\n", eerd);
    return (eerd >> 16);
}

static void reset_nic(struct e1000_device* netcard)
{
    uint64_t mac;
    int i;

    // Rx/Tx deaktivieren
    reg_outl(netcard, REG_RX_CTL, 0);
    reg_outl(netcard, REG_TX_CTL, 0);

    // Reset ausfuehren
    reg_outl(netcard, REG_CTL, CTL_PHY_RESET);
    cdi_sleep_ms(10);

    reg_outl(netcard, REG_CTL, CTL_RESET);
    cdi_sleep_ms(10);
    while (reg_inl(netcard, REG_CTL) & CTL_RESET);

    // Kontrollregister initialisieren
    reg_outl(netcard, REG_CTL, CTL_AUTO_SPEED | CTL_LINK_UP);

    // Rx/Tx-Ring initialisieren
    reg_outl(netcard, REG_RXDESC_ADDR_HI, 0);
    reg_outl(netcard, REG_RXDESC_ADDR_LO, PHYS(netcard, rx_desc[0]));
    reg_outl(netcard, REG_RXDESC_LEN,
        RX_BUFFER_NUM * sizeof(struct e1000_rx_descriptor));
    reg_outl(netcard, REG_RXDESC_HEAD, 0);
    reg_outl(netcard, REG_RXDESC_TAIL, RX_BUFFER_NUM - 1);
    reg_outl(netcard, REG_RX_DELAY_TIMER, 0);
    reg_outl(netcard, REG_RADV, 0);

    reg_outl(netcard, REG_TXDESC_ADDR_HI, 0);
    reg_outl(netcard, REG_TXDESC_ADDR_LO, PHYS(netcard, tx_desc[0]));
    reg_outl(netcard, REG_TXDESC_LEN,
        TX_BUFFER_NUM * sizeof(struct e1000_tx_descriptor));
    reg_outl(netcard, REG_TXDESC_HEAD, 0);
    reg_outl(netcard, REG_TXDESC_TAIL, 0);
    reg_outl(netcard, REG_TX_DELAY_TIMER, 0);
    reg_outl(netcard, REG_TADV, 0);

    // VLANs deaktivieren
    reg_outl(netcard, REG_VET, 0);

    // MAC-Filter
    mac = get_mac_address(netcard);
    reg_outl(netcard, REG_RECV_ADDR_LIST, mac & 0xFFFFFFFF);
    reg_outl(netcard, REG_RECV_ADDR_LIST + 4,
        ((mac >> 32) & 0xFFFF) | RAH_VALID);

    // Rx-Deskriptoren aufsetzen
    for (i = 0; i < RX_BUFFER_NUM; i++) {
        netcard->rx_desc[i].length = RX_BUFFER_SIZE;
        netcard->rx_desc[i].buffer =
            PHYS(netcard, rx_buffer[i * RX_BUFFER_SIZE]);

#ifdef DEBUG
        printf("e1000: [%d] Rx: Buffer @ phys %08x, Desc @ phys %08x\n",
            i,
            netcard->rx_desc[i].buffer, 
            PHYS(netcard, rx_desc[i]));
#endif
    }

    netcard->tx_cur_buffer = 0;
    netcard->rx_cur_buffer = 0;

    // Interrupts aktivieren
    reg_outl(netcard, REG_INTR_MASK_CLR, 0xFFFF);
    reg_outl(netcard, REG_INTR_MASK, 0xFFFF);

    // Rx/Tx aktivieren
    reg_outl(netcard, REG_RX_CTL, RCTL_ENABLE | RCTL_BROADCAST);
    reg_outl(netcard, REG_TX_CTL, TCTL_ENABLE | TCTL_PADDING
        | TCTL_COLL_TSH | TCTL_COLL_DIST);
}


static uint64_t get_mac_address(struct e1000_device* device)
{
    uint64_t mac = 0;
    int i;

    for (i = 0; i < 3; i++) {
        mac |= (uint64_t) e1000_eeprom_read(
            device, EEPROM_OFS_MAC + i) << (i * 16);
    }

    return mac;
}

void e1000_init_device(struct cdi_device* device)
{
    struct e1000_device* netcard = (struct e1000_device*) device;
    netcard->net.send_packet = e1000_send_packet;

    // PCI-bezogenes Zeug initialisieren
    netcard->revision = netcard->pci->rev_id;
    cdi_register_irq(netcard->pci->irq, e1000_handle_interrupt, device);
    cdi_pci_alloc_ioports(netcard->pci);

    cdi_list_t reslist = netcard->pci->resources;
    struct cdi_pci_resource* res;
    int i;
    for (i = 0; (res = cdi_list_get(reslist, i)); i++) {
        if (res->type == CDI_PCI_MEMORY) {
            netcard->mem_base =
                cdi_alloc_phys_addr(res->length, res->start);
        }
    }

    // Karte initialisieren
    printf("e1000: IRQ %d, MMIO an %x  Revision:%d\n",
        netcard->pci->irq, netcard->mem_base, netcard->revision);

    printf("e1000: Fuehre Reset der Karte durch\n");
    reset_nic(netcard);

    netcard->net.mac = get_mac_address(netcard);
    printf("e1000: MAC-Adresse: %012llx\n", netcard->net.mac);

    cdi_net_device_init((struct cdi_net_device*) device);
}

void e1000_remove_device(struct cdi_device* device)
{
}

/**
 * Die Uebertragung von Daten geschieht durch einen Ring von
 * Transmit-Deskriptoren, die jeweils ein zu uebertragendes Paket
 * beschreiben.
 *
 * Die Hardware kennt dabei zwei besondere Deskriptoren, die Head und
 * Tail heissen. Wenn der Treiber ein neues Paket zum Senden einstellt,
 * fuegt er einen neuen Deskriptor nach Tail ein und erhoeht Tail.
 *
 * Die Hardware erhoeht ihrerseits Head, wenn sie ein Paket abgeschickt hat.
 * Wenn Head = Tail ist, ist die Sendewarteschlange leer.
 */
void e1000_send_packet(struct cdi_net_device* device, void* data, size_t size)
{
    struct e1000_device* netcard = (struct e1000_device*) device;
    uint32_t cur, head;

#ifdef DEBUG
    printf("e1000: e1000_send_packet\n");
#endif

    // Aktuellen Deskriptor erhoehen
    cur = netcard->tx_cur_buffer;
    netcard->tx_cur_buffer++;
    netcard->tx_cur_buffer %= TX_BUFFER_NUM;

    // Head auslesen
    head = reg_inl(netcard, REG_TXDESC_HEAD);
    if (netcard->tx_cur_buffer == head) {
        printf("e1000: Kein Platz in der Sendewarteschlange!\n");
        return;
    }

    // Buffer befuellen
    if (size > TX_BUFFER_SIZE) {
        size = TX_BUFFER_SIZE;
    }
    memcpy(netcard->tx_buffer + cur * TX_BUFFER_SIZE, data, size);

    // TX-Deskriptor setzen und Tail erhoehen
    netcard->tx_desc[cur].cmd = TX_CMD_EOP | TX_CMD_IFCS;
    netcard->tx_desc[cur].length = size;
    netcard->tx_desc[cur].buffer =
        PHYS(netcard, tx_buffer) + (cur * TX_BUFFER_SIZE);

#ifdef DEBUG
    printf("e1000: Setze Tail auf %d, Head = %d\n", netcard->tx_cur_buffer, head);
#endif
    reg_outl(netcard, REG_TXDESC_TAIL, netcard->tx_cur_buffer);
}

static void e1000_handle_interrupt(struct cdi_device* device)
{
    struct e1000_device* netcard = (struct e1000_device*) device;

    uint32_t icr = reg_inl(netcard, REG_INTR_CAUSE);

#ifdef DEBUG
    printf("e1000: Interrupt, ICR = %08x\n", icr);
#endif

    if (icr & ICR_RECEIVE) {

        uint32_t head = reg_inl(netcard, REG_RXDESC_HEAD);

        while (netcard->rx_cur_buffer != head) {

            size_t size = netcard->rx_desc[netcard->rx_cur_buffer].length;
            uint8_t status = netcard->rx_desc[netcard->rx_cur_buffer].status;

            // Wenn Descriptor Done nicht gesetzt ist, war die Hardware
            // noch nicht gant fertig mit Kopieren
            if ((status & 0x1) == 0) {
                break;
            }

            // 4 Bytes CRC von der Laenge abziehen
            size -= 4;

#ifdef DEBUG
            printf("e1000: %d Bytes empfangen (status = %x)\n", size, status);
/*
            int i;
            for (i = 0; i < (size < 49 ? size : 49); i++) {
                printf("%02hhx ", netcard->rx_buffer[
                    netcard->rx_cur_buffer * RX_BUFFER_SIZE + i]);
                if (i % 25 == 0) {
                    printf("\n");
                }
            }
            printf("\n\n");
*/
#endif

            cdi_net_receive(
                (struct cdi_net_device*) netcard,
                &netcard->rx_buffer[netcard->rx_cur_buffer * RX_BUFFER_SIZE],
                size);

            netcard->rx_cur_buffer++;
            netcard->rx_cur_buffer %= RX_BUFFER_NUM;
        }

        if (netcard->rx_cur_buffer == head) {
            reg_outl(netcard, REG_RXDESC_TAIL,
                (head + RX_BUFFER_NUM - 1) % RX_BUFFER_NUM);
        } else {
            reg_outl(netcard, REG_RXDESC_TAIL, netcard->rx_cur_buffer);
        }

    } else if (icr & ICR_TRANSMIT) {
        // Nichts zu tun
    } else {
        printf("e1000: Unerwarteter Interrupt.\n");
    }
}
