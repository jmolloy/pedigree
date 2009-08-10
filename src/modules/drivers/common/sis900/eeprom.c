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

#include <stdint.h>

#include "cdi.h"

#include "device.h"
#include "sis900_io.h"

uint16_t sis900_eeprom_read(struct sis900_device* device, uint16_t offset)
{
    // EEPROM-Zugriff initialisieren
    reg_outl(device, REG_EEPROM, 0);
    reg_inl(device, REG_EEPROM);
    
    reg_outl(device, REG_EEPROM, EEPROM_CLOCK);
    reg_inl(device, REG_EEPROM);

    // Befehl bitweise verfuettern
    uint16_t cmd = EEPROM_OP_READ | offset;
    int i;
    for (i = 8; i >= 0; i--) {
        uint32_t regvalue = EEPROM_CHIPSEL;
        if (cmd & (1 << i)) {        
            regvalue |= EEPROM_DATA_IN;
        }
    
        reg_outl(device, REG_EEPROM, regvalue);
        reg_inl(device, REG_EEPROM);
        
        reg_outl(device, REG_EEPROM, EEPROM_CLOCK | regvalue);
        reg_inl(device, REG_EEPROM);
    }
        
    reg_outl(device, REG_EEPROM, EEPROM_CHIPSEL);
    reg_inl(device, REG_EEPROM);

    // Und dasselbe umgekehrt mit dem Ergebnis
    uint16_t result = 0;
    for (i = 15; i >= 0; i--) {
        reg_outl(device, REG_EEPROM, EEPROM_CHIPSEL);
        reg_inl(device, REG_EEPROM);
        
        reg_outl(device, REG_EEPROM, EEPROM_CLOCK | EEPROM_CHIPSEL);
        reg_inl(device, REG_EEPROM);

        result |= 
            ((reg_inl(device, REG_EEPROM) & EEPROM_DATA_OUT) ? 1 : 0) << i;
    }

    // EEPROM-Zugriff beenden
    reg_outl(device, REG_EEPROM, 0);
    reg_inl(device, REG_EEPROM);
    
    reg_outl(device, REG_EEPROM, EEPROM_CLOCK);
    reg_inl(device, REG_EEPROM);

    return result;
}
