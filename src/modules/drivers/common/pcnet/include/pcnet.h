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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *     This product includes software developed by the tyndur Project
 *     and its contributors.
 * 4. Neither the name of the tyndur Project nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
#ifndef __PCNET_H__
#define __PCNET_H__

#include "cdi/net.h"
#include "cdi/pci.h"
#include "cdi/misc.h"

// PCI Device/Vendor ID
#define VENDOR_ID 0x1022
#define DEVICE_ID 0x2000

// PCI I/O Register
#define APROM0  0x00
#define APROM2  0x02
#define APROM4  0x04
#define APROM6  0x06
#define APROM8  0x08
#define APROM10 0x0A
#define APROM12 0x0C
#define APROM14 0x0E
#define RDP     0x10
#define RAP     0x12
#define RESET   0x14
#define BDP     0x16

/* CSR Registerindex */
#define CSR_STATUS         0
#define CSR_INTERRUPT_MASK 3
#define CSR_FEATURE        4

/* CSR0 bits */
#define STATUS_INIT                 0x01
#define STATUS_START                0x02
#define STATUS_STOP                 0x04
#define STATUS_TRANSMIT_DEMAND      0x08
#define STATUS_INTERRUPT_ENABLE     0x40
#define STATUS_INIT_DONE            0x100
#define STATUS_TRANSMIT_INTERRUPT   0x200
#define STATUS_RECEIVE_INTERRUPT    0x400
#define STATUS_ERROR                0x8000

/* CSR4 bits */
#define FEATURE_AUTOSTRIP_RECEIVE   0x400
#define FEATURE_AUTOPAD_TRANSMIT    0x800
                
/* CSR15 bits */
#define PROMISCUOUS_MODE            0x8000

/* BCR registers */
#define BCR_SOFTWARE_STYLE          20

/* flags in the receive/transmit descriptors */
#define DESCRIPTOR_OWN              0x80000000
#define DESCRIPTOR_ERROR            0x40000000
#define DESCRIPTOR_PACKET_START     0x02000000
#define DESCRIPTOR_PACKET_END       0x01000000

struct initialization_block {
    uint16_t    mode;
    uint8_t     receive_length;
    uint8_t     transfer_length;
    uint64_t    physical_address;
    uint64_t    logical_address;
    uint32_t    receive_descriptor;
    uint32_t    transmit_descriptor;
} __attribute__((packed));

struct receive_descriptor{
    uint32_t    address;
    uint32_t    flags;
    uint32_t    flags2;
    uint32_t    res;
} __attribute__((packed));

struct transmit_descriptor{
    uint32_t    address;
    uint32_t    flags;
    uint32_t    flags2;
    uint32_t    res;
} __attribute__((packed));

struct pcnet_device {
    struct cdi_net_device       net;
    struct cdi_pci_device*      pci;

    uint16_t                    port_base;
    struct receive_descriptor   *receive_descriptor;
    struct transmit_descriptor  *transmit_descriptor;
    void                        *phys_receive_descriptor;
    void                        *phys_transmit_descriptor;

    struct initialization_block *initialization_block;
    void                        *phys_initialization_block;

    void                        *receive_buffer[8];
    void                        *transmit_buffer[8];
    int                         last_transmit_descriptor;
    int                         last_transmit_descriptor_eval;
    int                         last_receive_descriptor;

    volatile int                init_wait_for_irq;
};

void pcnet_init_device(struct cdi_device* device);
void pcnet_remove_device(struct cdi_device* device);
void pcnet_send_packet
    (struct cdi_net_device* device, void* data, size_t size);

#endif
