/*
 * Copyright (c) 2010 Matthew Iselin
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
#ifndef _I2C_H
#define _I2C_H

#include <processor/types.h>
#include <processor/MemoryRegion.h>

class I2C
{
    public:
        I2C() : m_MmioBase("I2C")
        {
        }
        virtual ~I2C()
        {
        }

        static I2C &instance(size_t n)
        {
            return m_Instance[n];
        }

        void initialise(uintptr_t baseAddr);

        bool transmit(uint8_t addr, uintptr_t buffer, size_t len);
        bool receive(uint8_t addr, uintptr_t buffer, size_t maxlen);

        bool write(uint8_t addr, uint8_t reg, uint8_t data);
        uint8_t read(uint8_t addr, uint8_t reg);

    private:

        static I2C m_Instance[3];

        MemoryRegion m_MmioBase;

        void waitForBus();

        enum Registers
        {
            I2C_REV     = 0x00 / 2, // R
            I2C_IE      = 0x04 / 2, // RW
            I2C_STAT    = 0x08 / 2, // RW
            I2C_WE      = 0x0C / 2, // RW
            I2C_SYSS    = 0x10 / 2, // R
            I2C_BUF     = 0x14 / 2, // RW
            I2C_CNT     = 0x18 / 2, // RW
            I2C_DATA    = 0x1C / 2, // RW
            I2C_SYSC    = 0x20 / 2, // RW
            I2C_CON     = 0x24 / 2, // RW
            I2C_OA0     = 0x28 / 2, // RW
            I2C_SA      = 0x2C / 2, // RW
            I2C_PSC     = 0x30 / 2, // RW
            I2C_SCLL    = 0x34 / 2, // RW
            I2C_SCLH    = 0x38 / 2, // RW
            I2C_SYSTEST = 0x3C / 2, // RW
            I2C_BUFSTAT = 0x40 / 2, // R
            I2C_OA1     = 0x44 / 2, // RW
            I2C_OA2     = 0x48 / 2, // RW
            I2C_OA3     = 0x4C / 2, // RW
            I2C_ACTOA   = 0x50 / 2, // R
            I2C_SBLOCK  = 0x54 / 2, // RW
        };
};

#endif
