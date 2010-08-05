/*
 * Copyright (c) 2009 Eduard Burtescu
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
#ifndef RTL8139_CONSTANTS_H
#define RTL8139_CONSTANTS_H

enum Rtl8139Constants {
    RTL_MAC = 0,                // MAC addresses registers
    RTL_MAR = 8,                // Multicast addresses registers
    RTL_TXSTS0 = 0x10,          // Tx Status register
    RTL_TXADDR0 = 0x20,         // Tx Address register
    RTL_RXBUFF = 0x30,          // Rx Buffer address register
    RTL_CMD = 0x37,             // Command register
    RTL_RXCURR = 0x38,          // Rx Buffer current offset register
    RTL_IMR = 0x3C,             // Intrerrupt Mask register
    RTL_ISR = 0x3E,             // Intrerrupt Status register
    RTL_TXCFG = 0x40,           // Tx Config register
    RTL_RXCFG = 0x44,           // Rx Config register
    RTL_RXMIS = 0x4C,           // Rx Missed register
    RTL_CFG9346 = 0x50,         // 9346 Config register
    RTL_CFG1 = 0x52,            // Config1 register
    RTL_MSR = 0x58,             // Media Status register
    RTL_BMCR = 0x62,            // Basic Mode Control register

    RTL_CMD_RES = 0x10,         // Reset command
    RTL_CMD_RXEN = 0x08,        // Rx Enable command
    RTL_CMD_TXEN = 0x04,        // Tx Enable command

    RTL_ISR_TXERR = 0x08,       // Tx Error irq status bit
    RTL_ISR_TXOK = 0x04,        // Tx OK irq status bit
    RTL_ISR_RXERR = 0x02,       // Rx Error irq status bit
    RTL_ISR_RXOK = 0x01,        // Rx OK irq status bit

    RTL_IMR_TXERR = 0x08,       // Tx Error irq mask bit
    RTL_IMR_TXOK = 0x04,        // Tx OK irq mask bit
    RTL_IMR_RXERR = 0x02,       // Rx Error irq mask bit
    RTL_IMR_RXOK = 0x01,        // Rx OK irq mask bit

    RTL_RXCFG_FTH_NONE = 0xE000,// No FIFO treshhold
    RTL_RXCFG_RBLN_64K = 0x1800,// 64K Rx Buffer length
    RTL_RXCFG_MDMA_UNLM = 0x700,// Unlimited DMA Burst size
    RTL_RXCFG_AR = 0x10,        // Accept Runt packets
    RTL_RXCFG_AB = 0x08,        // Accept Broadcast packets
    RTL_RXCFG_AM = 0x04,        // Accept Multicast packets
    RTL_RXCFG_APM = 0x02,       // Accept Physical Match packets
    RTL_RXCFG_AAP = 0x01,       // Accept All packets

    RTL_RXSTS_MAR = 0x8000,     // Multicast address
    RTL_RXSTS_PAM = 0x4000,     // Physical address matched
    RTL_RXSTS_BAR = 0x2000,     // Broadcast address
    RTL_RXSTS_ISE = 0x20,       // Invalid Symbol error
    RTL_RXSTS_RUNT = 0x10,      // Runt packet
    RTL_RXSTS_LONG = 0x08,      // Long packet
    RTL_RXSTS_CRC = 0x04,       // CRC error
    RTL_RXSTS_FAE = 0x02,       // Frame Alignment error
    RTL_RXSTS_RXOK = 0x01,      // Rx OK

    RTL_TXCFG_MDMA_1K = 0x600,  // 1K DMA Burst
    RTL_TXCFG_MDMA_2K = 0x700,  // 2K DMA Burst
    RTL_TXCFG_RR_48 = 0x20,     // 48 (16 + 2 * 16) Tx Retry count

    RTL_CFG9346_LOCK = 0x00,    // Lock BMCR registers
    RTL_CFG9346_UNLOCK = 0xC0,  // Unlock BMCR registers

    RTL_MSR_RXFCE = 0x40,       // Rx Flow Control Enable bit
    RTL_MSR_LINK = 0x4,         // Inverse of Link Status bit

    RTL_BMCR_SPEED = 0x2000,    // Speed(100Mbps/10Mbps) bit
    RTL_BMCR_ANE = 0x1000,      // Auto Negotiation Enable bit
    RTL_BMCR_DUPLEX = 0x100,    // Speed(100Mbps/10Mbps) bit

    RTL_BUFF_SIZE = 0x10000,    // The size of the Rx and Tx buffers

    RTL_PACK_MAX = 0xFFFF,      // The maximal size of a packet
    RTL_PACK_MIN = 0x16,        // The minimal size of a packet
};

#endif
