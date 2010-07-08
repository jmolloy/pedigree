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
#include "I2C.h"
#include "Prcm.h"
#include <processor/PhysicalMemoryManager.h>
#include <processor/VirtualAddressSpace.h>
#include <processor/Processor.h>
#include <process/Semaphore.h>
#include <Log.h>

I2C I2C::m_Instance[3];

#define delay(n) do{Semaphore semWAIT(0);semWAIT.acquire(1, 0, n*1000);}while(0)

void I2C::initialise(uintptr_t baseAddr)
{
    // Map in the base
    if(!PhysicalMemoryManager::instance().allocateRegion(m_MmioBase,
                                                         1,
                                                         PhysicalMemoryManager::continuous,
                                                         VirtualAddressSpace::Write | VirtualAddressSpace::KernelMode,
                                                         baseAddr))
    {
        // Failed to allocate the region!
        return;
    }

    // Enable functional clocks for all three I2C modules (even though we're
    // only initialising one now)
    Prcm::instance().SetFuncClockCORE(1, 15, true);
    Prcm::instance().SetFuncClockCORE(1, 16, true);
    Prcm::instance().SetFuncClockCORE(1, 17, true);

    // Same for the interface clocks
    Prcm::instance().SetIfaceClockCORE(1, 15, true);
    Prcm::instance().SetIfaceClockCORE(1, 16, true);
    Prcm::instance().SetIfaceClockCORE(1, 17, true);

    // Dump information about this module
    volatile uint16_t *base = reinterpret_cast<volatile uint16_t*>(m_MmioBase.virtualAddress());
    NOTICE("I2C Module at " << baseAddr << " (" << reinterpret_cast<uintptr_t>(m_MmioBase.virtualAddress()) << "): Revision " << Dec << ((base[I2C_REV] >> 4) & 0xF) << "." << (base[I2C_REV] & 0xF) << Hex << ".");

    // Reset the module
    base[I2C_SYSC] = 2;
    base[I2C_CON] = 0x8000;
    while(!(base[I2C_SYSS] & 1));
    base[I2C_SYSC] = 0;

    // Disable the module once again while we set it up
    base[I2C_CON] = 0;

    // Set up a 100 kbps transfer rate
    base[I2C_PSC] = 23; // 96 MHz / 23 = 4 MHz (looking for 100 kbps rate)
    base[I2C_SCLL] = 13;
    base[I2C_SCLH] = 15;

    // Configure the "own address"
    base[I2C_OA0] = 1;

    // 1-byte FIFO
    base[I2C_BUF] = 0;

    // Disable interrupts completely
    base[I2C_IE] = 0;

    // Start the module
    base[I2C_CON] = 0; // 0x8000;

    // Clear status etc
    base[I2C_STAT] = 0xFFFF;
    base[I2C_CNT] = 0;
}

bool I2C::write(uint8_t addr, uint8_t reg, uint8_t data)
{
    uint8_t buffer[] = {reg, data};
    return transmit(addr, reinterpret_cast<uintptr_t>(buffer), 2);
}

uint8_t I2C::read(uint8_t addr, uint8_t reg)
{
    uint8_t buffer[] = {reg};
    if(!transmit(addr, reinterpret_cast<uintptr_t>(buffer), 1))
        return 0;
    if(!receive(addr, reinterpret_cast<uintptr_t>(buffer), 1))
        return 0;
    return buffer[0];
}

bool I2C::transmit(uint8_t addr, uintptr_t buffer, size_t len)
{
    volatile uint16_t *base = reinterpret_cast<volatile uint16_t*>(m_MmioBase.virtualAddress());
    uint8_t *buf = reinterpret_cast<uint8_t*>(buffer);

    waitForBus();

    // Program the transfer
    base[I2C_SA] = addr;
    base[I2C_CNT] = len;
    base[I2C_CON] = 0x8603; // Transmit

    // Perform the transfer itself
    bool success = true;
    while(1)
    {
        delay(1);
        uint16_t status = base[I2C_STAT];
        if(status & 0x1)
        {
            // Arbitration lost
            NOTICE("I2C: Arbitration lost");
            success = false;
            break;
        }
        else if(status & 0x2)
        {
            // NACK
            NOTICE("I2C: NACK");
            success = false;
            break;
        }
        else if(status & 0x4)
        {
            // ARDY - transfer complete
            break;
        }
        else if(status & 0x10)
        {
            // XRDY, transmit a byte
            *reinterpret_cast<volatile uint8_t*>(&base[I2C_DATA]) = *buf++;
        }

        delay(50);
        base[I2C_STAT] = status;
    }

    base[I2C_STAT] = 0xFFFF;
    base[I2C_CNT] = 0;

    return success;
}

bool I2C::receive(uint8_t addr, uintptr_t buffer, size_t maxlen)
{
    volatile uint16_t *base = reinterpret_cast<volatile uint16_t*>(m_MmioBase.virtualAddress());
    uint8_t *buf = reinterpret_cast<uint8_t*>(buffer);

    waitForBus();

    // Program the DMA transfer
    base[I2C_SA] = addr;
    base[I2C_CNT] = maxlen;
    base[I2C_CON] = 0x8403; // No transmit

    // Perform the transfer itself
    bool success = true;
    while(1)
    {
        delay(1);
        uint16_t status = base[I2C_STAT];
        if(status & 0x1)
        {
            // Arbitration lost
            NOTICE("I2C: Arbitration lost");
            success = false;
            break;
        }
        else if(status & 0x2)
        {
            // NACK
            NOTICE("I2C: NACK");
            success = false;
            break;
        }
        else if(status & 0x4)
        {
            // ARDY - transfer complete
            break;
        }
        if(status & 0x8)
        {
            // RRDY, transmit a byte
            *buf++ = *reinterpret_cast<volatile uint8_t*>(&base[I2C_DATA]);
        }

        delay(50);
        base[I2C_STAT] = status;
    }

    base[I2C_STAT] = 0xFFFF;
    base[I2C_CNT] = 0;

    return success;
}

void I2C::waitForBus()
{
    volatile uint16_t *base = reinterpret_cast<volatile uint16_t*>(m_MmioBase.virtualAddress());

    // Wait for the bus to be available
    base[I2C_STAT] = 0xFFFF;
    uint32_t status = 0;
    while((status = base[I2C_STAT]) & 0x1000)
    {
        base[I2C_STAT] = status;
        delay(50);
    }
    base[I2C_STAT] = 0xFFFF;
}
