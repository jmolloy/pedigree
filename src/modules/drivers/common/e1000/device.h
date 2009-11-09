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

#ifndef _SIS900_DEVICES_H_
#define _SIS900_DEVICES_H_

#include <stdint.h>

#include "cdi.h"
#include "cdi/net.h"
#include "cdi/pci.h"

/* Register */

enum {
    REG_CTL             =    0x0,
    REG_STATUS          =    0x8,
    REG_EECD            =   0x10,
    REG_EEPROM_READ     =   0x14, /* EERD */
    REG_VET             =   0x38, /* VLAN */

    REG_INTR_CAUSE      =   0xc0, /* ICR */
    REG_INTR_MASK       =   0xd0, /* IMS */
    REG_INTR_MASK_CLR   =   0xd8, /* IMC */


    REG_RX_CTL          =  0x100,
    REG_TX_CTL          =  0x400,

    REG_RXDESC_ADDR_LO  = 0x2800,
    REG_RXDESC_ADDR_HI  = 0x2804,
    REG_RXDESC_LEN      = 0x2808,
    REG_RXDESC_HEAD     = 0x2810,
    REG_RXDESC_TAIL     = 0x2818,

    REG_RX_DELAY_TIMER  = 0x2820,
    REG_RADV            = 0x282c,


    REG_TXDESC_ADDR_LO  = 0x3800,
    REG_TXDESC_ADDR_HI  = 0x3804,
    REG_TXDESC_LEN      = 0x3808,
    REG_TXDESC_HEAD     = 0x3810,
    REG_TXDESC_TAIL     = 0x3818,

    REG_TX_DELAY_TIMER  = 0x3820,
    REG_TADV            = 0x382c,

    REG_RECV_ADDR_LIST  = 0x5400, /* RAL */
};

enum {
    CTL_AUTO_SPEED  = (1 <<  5), /* ASDE */
    CTL_LINK_UP     = (1 <<  6), /* SLU  */
    CTL_RESET       = (1 << 26), /* RST */
    CTL_PHY_RESET   = (1 << 31),
};

enum {
    RCTL_ENABLE     = (1 <<  1),
    RCTL_BROADCAST  = (1 << 15), /* BAM */
};

enum {
    TCTL_ENABLE     = (1 <<  1),
    TCTL_PADDING    = (1 <<  2),
    TCTL_COLL_TSH   = (0x0f <<  4), /* CT - Collision Threshold */
    TCTL_COLL_DIST  = (0x40 << 12), /* COLD - Collision Distance */    
};

enum {
    ICR_TRANSMIT    = (1 <<  0),
    ICR_RECEIVE     = (1 <<  7),
};

enum {
    EEPROM_OFS_MAC      = 0x0,
};

enum {
    EECD_SK             = 1,
    EECD_CS             = 2,
    EECD_DI             = 4,
    EECD_DO             = 8,
    EECD_EE_REQ         = 64,
    EECD_EE_GNT         = 128,
    EECD_EE_PRES        = 256,
    EECD_EE_SIZE        = 512,
    EECD_EE_ABITS       = 1024,
    EECD_EE_TYPE        = 8192
};

#define UWIRE_OP_ERASE      0x4
#define UWIRE_OP_WRITE      0x5
#define UWIRE_OP_READ       0x6

#define EECD_FWE(x)         ((x) << 4)
#define EECD_FWE_DISABLED   EECD_FWE(1)
#define EECD_FWE_ENABLED    EECD_FWE(2)


#define RAH_VALID   (1 << 31) /* AV */

#define EERD_START  (1 <<  0)
#define EERD_DONE   (1 <<  4)

/* Allgemeine Definitionen */

#define TX_BUFFER_SIZE  2048
//#define TX_BUFFER_NUM   64

#define RX_BUFFER_SIZE  1536
//#define RX_BUFFER_NUM   64

// Die Anzahl von Deskriptoren muss jeweils ein vielfaches von 8 sein
#define RX_BUFFER_NUM   8
#define TX_BUFFER_NUM   8

struct e1000_tx_descriptor {
    uint64_t            buffer;
    uint16_t            length;
    uint8_t             checksum_offset;
    uint8_t             cmd;
    uint8_t             status;
    uint8_t             checksum_start;
    uint16_t            special;
} __attribute__((packed)) __attribute__((aligned (4)));

enum {
    TX_CMD_EOP  = 0x01,
    TX_CMD_IFCS = 0x02,
};

struct e1000_rx_descriptor {
    uint64_t            buffer;
    uint16_t            length;
    uint16_t            padding;
    uint8_t             status;
    uint8_t             error;
    uint16_t            padding2;
} __attribute__((packed)) __attribute__((aligned (4)));

struct e1000_device {
    struct cdi_net_device       net;
    struct cdi_pci_device*      pci;

    void*                       phys;

    struct e1000_tx_descriptor  tx_desc[TX_BUFFER_NUM];
    uint8_t                     tx_buffer[TX_BUFFER_NUM * TX_BUFFER_SIZE];
    int                         tx_cur_buffer;

    struct e1000_rx_descriptor  rx_desc[RX_BUFFER_NUM];
    uint8_t                     rx_buffer[RX_BUFFER_NUM * RX_BUFFER_SIZE];
    int                         rx_cur_buffer;

    void*                       mem_base;
    uint8_t                     revision;
};

void e1000_init_device(struct cdi_device* device);
void e1000_remove_device(struct cdi_device* device);

void e1000_send_packet
    (struct cdi_net_device* device, void* data, size_t size);

#endif
