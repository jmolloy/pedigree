/*
 * Copyright (c) 2008 The tyndur Project. All rights reserved.
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

#include "device.h"
#include "sis900_io.h"

uint16_t sis900_mii_read(struct sis900_device* netcard, int phy, int reg)
{
    int i;
    uint16_t result = 0;
    uint16_t cmd = EEPROM_MII_READ | phy << 7 | reg << 2;

    // mdio_reset
    for (i = 32; i > 0; i--) {
        reg_outl(netcard, REG_EEPROM, EEPROM_MII_MDIO | EEPROM_MII_MDDIR);
        reg_inl(netcard, REG_EEPROM);
        reg_outl(netcard, REG_EEPROM, EEPROM_MII_MDIO|EEPROM_MII_MDDIR|EEPROM_MII_MDC);
        reg_inl(netcard, REG_EEPROM);
    }

    // Lesen
    // return sis900_eeprom_read(netcard, phy << 7 | reg << 2); 

    // Befehl bitweise verfuettern
    for (i = 15; i >= 0; i--) {
        uint32_t regvalue = EEPROM_MII_MDDIR;
        if (cmd & (1 << i)) {        
            regvalue |= EEPROM_MII_MDIO;
        }
    
        reg_outl(netcard, REG_EEPROM, regvalue);
        reg_inl(netcard, REG_EEPROM);
        
        reg_outl(netcard, REG_EEPROM, regvalue | EEPROM_MII_MDC);
        reg_inl(netcard, REG_EEPROM);
    }
        
    // Und dasselbe umgekehrt mit dem Ergebnis
    for (i = 15; i >= 0; i--) {
        reg_outl(netcard, REG_EEPROM, 0);
        reg_inl(netcard, REG_EEPROM);

        result |= 
            ((reg_inl(netcard, REG_EEPROM) & EEPROM_MII_MDIO) ? 1 : 0) << i;
        
        reg_outl(netcard, REG_EEPROM, EEPROM_MII_MDC);
        reg_inl(netcard, REG_EEPROM);
    }
        
    reg_outl(netcard, REG_EEPROM, 0);
    return result;
}

void sis900_mii_write(struct sis900_device* netcard, int phy, int reg, uint16_t value)
{
    int i;
    uint16_t cmd = EEPROM_MII_WRITE | phy << 7 | reg << 2;

    // mdio_reset
    for (i = 32; i > 0; i--) {
        reg_outl(netcard, REG_EEPROM, EEPROM_MII_MDIO | EEPROM_MII_MDDIR);
        reg_inl(netcard, REG_EEPROM);
        reg_outl(netcard, REG_EEPROM, EEPROM_MII_MDIO|EEPROM_MII_MDDIR|EEPROM_MII_MDC);
        reg_inl(netcard, REG_EEPROM);
    }

    // Befehl bitweise verfuettern
    for (i = 15; i >= 0; i--) {
        uint32_t regvalue = EEPROM_MII_MDDIR;
        if (cmd & (1 << i)) {        
            regvalue |= EEPROM_MII_MDIO;
        }
    
        reg_outl(netcard, REG_EEPROM, regvalue);
        reg_inl(netcard, REG_EEPROM);
        
        reg_outl(netcard, REG_EEPROM, regvalue | EEPROM_MII_MDC);
        reg_inl(netcard, REG_EEPROM);
    }
        
    // Und dasselbe umgekehrt mit dem Ergebnis
    for (i = 15; i >= 0; i--) {
        uint32_t regvalue = EEPROM_MII_MDDIR;
        if (value & (1 << i)) {        
            regvalue |= EEPROM_MII_MDIO;
        }

        reg_outl(netcard, REG_EEPROM, regvalue);
        reg_inl(netcard, REG_EEPROM);
        
        reg_outl(netcard, REG_EEPROM,regvalue | EEPROM_MII_MDC);
        reg_inl(netcard, REG_EEPROM);
    }

    // clear extra bits
    for (i = 2; i-- > 0;) {
        reg_outb(netcard, REG_EEPROM, 0);
        reg_inl(netcard, REG_EEPROM);
        reg_outb(netcard, REG_EEPROM, EEPROM_MII_MDC);
        reg_inl(netcard, REG_EEPROM);
    }

    reg_outl(netcard, REG_EEPROM, 0);
}

#if 0
uint16_t sis900_mii_read(struct sis900_device* netcard, int reg)
{
    uint32_t ret;

    reg_outl(netcard, REG_MII_ACC, 
        reg << MII_ACC_REG_SHIFT | MII_ACC_READ | MII_ACC_ACCESS);
    
    while ((ret = reg_inl(netcard, REG_MII_ACC)) & MII_ACC_ACCESS);

    return ret >> 16;
}

void sis900_mii_write(struct sis900_device* netcard, int reg, uint16_t value)
{
    uint32_t ret;

    reg_outl(netcard, REG_MII_ACC, (uint32_t) value << 16 | 
        reg << MII_ACC_REG_SHIFT | MII_ACC_READ | MII_ACC_ACCESS);
    
    while ((ret = reg_inl(netcard, REG_MII_ACC)) & MII_ACC_ACCESS);
}
#endif
