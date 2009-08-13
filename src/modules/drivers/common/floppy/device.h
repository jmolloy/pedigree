/*
 * Copyright (c) 2007 The tyndur Project. All rights reserved.
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

#ifndef _FLOPPY_DEVICE_H_
#define _FLOPPY_DEVICE_H_

#include <stdint.h>

#include "cdi.h"
#include "cdi/storage.h"

#define FLOPPY_REG_DOR 0
#define FLOPPY_REG_MSR 1
#define FLOPPY_REG_DSR 2
#define FLOPPY_REG_DATA 3
#define __FLOPPY_REG_MAX 4

// Digital output register
#define FLOPPY_DOR_MOTOR(d)         (1 << (4 + d->id))
#define FLOPPY_DOR_DMAGATE          (1 << 3)
#define FLOPPY_DOR_NRST             (1 << 2)
#define FLOPPY_DOR_DRIVE(d)         (d->id)
#define FLOPPY_DOR_DRIVE_MASK       0x03

// Data rate select register
#define FLOPPY_DSR_500KBPS          0x00
#define FLOPPY_DSR_300KBPS          0x01
#define FLOPPY_DSR_250KBPS          0x02
#define FLOPPY_DSR_1MBPS            0x03

// Master status register
#define FLOPPY_MSR_DIO              (1 << 6)
#define FLOPPY_MSR_RQM              (1 << 7)
#define FLOPPY_MSR_CMD_BSY          (1 << 4)
#define FLOPPY_MSR_DRV_BSY(d)       (1 << (d->id))

// Status 0 Register (nur bei Kommandosequenzen)
#define FLOPPY_ST0_IC_MASK          (3 << 6)
#define FLOPPY_ST0_IC_NORMAL        (0 << 6)
#define FLOPPY_ST0_SEEK_END         (1 << 5)  

// Befehle
#define FLOPPY_CMD_SPECIFY          0x3
#define FLOPPY_CMD_RECALIBRATE      0x07
#define FLOPPY_CMD_INT_SENSE        0x08
#define FLOPPY_CMD_SEEK             0x0F
#define FLOPPY_CMD_CONFIGURE        0x13
#define FLOPPY_CMD_READ             0x46
#define FLOPPY_CMD_WRITE            0x45

// Alle Delays in Millisekunden
#define FLOPPY_DELAY_SPINUP(d)      500
#define FLOPPY_DELAY_SPINDOWN(d)    500

// Anzahl der Versuche die unternommen werden wenn Daten an den Kotroller
// gesendet bzw. von ihm geholt werden sollen, und er beschaeftigt ist
#define FLOPPY_READ_DATA_TRIES(d)   50
#define FLOPPY_READ_DATA_DELAY(d)   10
#define FLOPPY_WRITE_DATA_TRIES(d)  50
#define FLOPPY_WRITE_DATA_DELAY(d)  10

// Timeout beim Warten auf den Interrupt nach einem Reset
#define FLOPPY_RESET_TIMEOUT(d)     100
#define FLOPPY_RECAL_TIMEOUT(d)     100
#define FLOPPY_SEEK_TIMEOUT(d)      100
#define FLOPPY_READ_TIMEOUT(d)      200

// Anzahl der Versuche wenn das neu kalibrieren nicht klappt
#define FLOPPY_RECAL_TRIES(d)       5

// Die Zeit die gewartet werden soll, nachdem der Kopf neu positioniert wurde
#define FLOPPY_SEEK_DELAY(d)        15


#define FLOPPY_SECTORS_PER_TRACK(d) 18
#define FLOPPY_HEAD_COUNT(d)        2
#define FLOPPY_SECTOR_SIZE(d)       512
#define FLOPPY_SECTOR_SIZE_CODE(d)  2
#define FLOPPY_GAP_LENGTH(d)        27
#define FLOPPY_SECTOR_COUNT(d)      2880
#define FLOPPY_DATA_RATE(d)         500000
#define FLOPPY_DMA_CHANNEL(d)       2

#define FLOPPY_FIFO_SIZE            16
#define FLOPPY_FIFO_THRESHOLD       4

struct floppy_controller {
    volatile uint16_t           irq_count;
    uint16_t                    ports[__FLOPPY_REG_MAX];
    uint16_t                    use_dma;
};

struct floppy_device {
    struct cdi_storage_device   dev;

    uint16_t                    id;
    struct floppy_controller*   controller;
    
    int                         motor;
    uint8_t                     current_cylinder;
    uint8_t                     current_status;
};

int floppy_init_controller(struct floppy_controller* controller);

void floppy_init_device(struct cdi_device* device);
void floppy_remove_device(struct cdi_device* device);
int floppy_read_blocks(struct cdi_storage_device* device, uint64_t block,
    uint64_t count, void* buffer);
int floppy_write_blocks(struct cdi_storage_device* device, uint64_t block,
    uint64_t count, void* buffer);

void floppy_handle_interrupt(struct cdi_device* device);

int floppy_device_probe(struct floppy_device* device);

#endif
