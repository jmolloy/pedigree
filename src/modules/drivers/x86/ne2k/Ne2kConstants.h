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
#ifndef NE2K_CONSTANTS_H
#define NE2K_CONSTANTS_H

enum Ne2kConstants {
    NE_CMD    = 0x0,
    NE_PSTART = 0x1, // write
    NE_PSTOP  = 0x2, // write
    NE_BNDRY  = 0x3,

    NE_TSR    = 0x4, // read

    NE_TPSR   = 0x4, // write
    NE_TBCR0  = 0x5, // write
    NE_TBCR1  = 0x6, // write

    NE_ISR    = 0x7,

    NE_RSAR0  = 0x8, // write
    NE_RSAR1  = 0x9, // write
    NE_RBCR0  = 0xa, // write
    NE_RBCR1  = 0xb, // write
    NE_RCR    = 0xc, // write
    NE_TCR    = 0xd, // write
    NE_DCR    = 0xe, // write

    NE_IMR    = 0xf, // write

    NE_PAR    = 0x1, // page 1, really 0x1 - 0x6
    NE_CURR   = 0x7, // page 1
    NE_MAR    = 0x8, // page 1, really 0x8 - 0xf
};

#define PAGE_TX           0x40
#define PAGE_RX           0x50
#define PAGE_STOP         0x80

#define NE_RESET				  0x1f
#define NE_DATA					  0x10

// E8390 chip constants
#define E8390_TX_IRQ_MASK 0xa   /* For register EN0_ISR */
#define E8390_RX_IRQ_MASK 0x5
#define E8390_RXCONFIG    0x4   /* EN0_RXCR: broadcasts, no multicast,errors */
#define E8390_RXOFF       0x20  /* EN0_RXCR: Accept no packets */
#define E8390_TXCONFIG    0x00  /* EN0_TXCR: Normal transmit mode */
#define E8390_TXOFF       0x02  /* EN0_TXCR: Transmitter off */

// E8390 chip commands
#define E8390_STOP      0x01    /* Stop and reset the chip */
#define E8390_START     0x02    /* Start the chip, clear reset */
#define E8390_TRANS     0x04    /* Transmit a frame */
#define E8390_RREAD     0x08    /* Remote read */
#define E8390_RWRITE    0x10    /* Remote write  */
#define E8390_NODMA     0x20    /* Remote DMA */
#define E8390_PAGE0     0x00    /* Select page chip registers */
#define E8390_PAGE1     0x40    /* using the two high-order bits */
#define E8390_PAGE2     0x80    /* Page 3 is invalid. */

#endif
