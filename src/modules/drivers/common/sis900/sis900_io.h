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

#ifndef _SIS900_IO_H_
#define _SIS900_IO_H_

#include <stdint.h>
#include "cdi/io.h"

#include "device.h"

static inline void reg_outb
    (struct sis900_device* netcard, uint8_t reg, uint8_t value)
{
    cdi_outb(netcard->port_base + reg, value);
}

static inline void reg_outw
    (struct sis900_device* netcard, uint8_t reg, uint16_t value) 
{
    cdi_outw(netcard->port_base + reg, value);
}

static inline void reg_outl
    (struct sis900_device* netcard, uint8_t reg, uint32_t value) 
{
    cdi_outl(netcard->port_base + reg, value);
}


static inline uint8_t reg_inb(struct sis900_device* netcard, uint8_t reg) 
{
    return cdi_inb(netcard->port_base + reg);
}

static inline uint16_t reg_inw(struct sis900_device* netcard, uint8_t reg) 
{
    return cdi_inw(netcard->port_base + reg);
}

static inline uint32_t reg_inl(struct sis900_device* netcard, uint8_t reg) 
{
    return cdi_inl(netcard->port_base + reg);
}

uint16_t sis900_mii_read(struct sis900_device* netcard, int phy, int reg);
void sis900_mii_write(struct sis900_device* netcard, int phy, int reg, uint16_t value);

#endif
