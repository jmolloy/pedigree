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

#include "UsbUlpi.h"
#include <processor/PhysicalMemoryManager.h>
#include <processor/VirtualAddressSpace.h>
#include <process/Semaphore.h>
#include <Log.h>

/// \todo super specific to a machine
#include <Prcm.h>
#include <I2C.h>
#include <Gpio.h>

UsbUlpi UsbUlpi::m_Instance;

#define delay(n) do{Semaphore semWAIT(0);semWAIT.acquire(1, 0, n*1000);}while(0)

bool usbWriteTwl4030(uint8_t addr, uint8_t data)
{
    return I2C::instance(0).write(0x48, addr, data);
}

uint8_t usbReadTwl4030(uint8_t addr)
{
    return I2C::instance(0).read(0x48, addr);
}

/// usbSetBits: In the ULPI register bank, a single RW register is followed
///             by an _SET and _CLR register. Writing a 1 to any bit in these
///             registers will set or clear a bit in the main register.
bool usbSetBits(uint8_t reg, uint8_t bits)
{
    return usbWriteTwl4030(reg + 1, bits);
}

/// usbClearBits: see usbSetBits
bool usbClearBits(uint8_t reg, uint8_t bits)
{
    return usbWriteTwl4030(reg + 2, bits);
}

/// Enable access to the USB PHY registers over the I2C bus
void enablePhyAccess(bool which)
{
    uint8_t clock = usbReadTwl4030(0xFE);
    if(clock)
    {
        if(which)
        {
            clock |= 1; // Request DPLL clock
            usbWriteTwl4030(0xFE, clock);
            while(!(usbReadTwl4030(0xFF) & 1))
                delay(10);
        }
        else
        {
            clock &= ~1;
            usbWriteTwl4030(0xFE, clock);
        }
    }
}

void UsbUlpi::initialise()
{
    // Allocate the pages we need
    if(!PhysicalMemoryManager::instance().allocateRegion(
                            m_MemRegionUHH,
                            1,
                            PhysicalMemoryManager::continuous | PhysicalMemoryManager::force,
                            VirtualAddressSpace::KernelMode | VirtualAddressSpace::Write,
                            0x48064000))
    {
        ERROR("USB UHH_CONFIG: Couldn't get a memory region!");
        return;
    }
    if(!PhysicalMemoryManager::instance().allocateRegion(
                            m_MemRegionTLL,
                            1,
                            PhysicalMemoryManager::continuous | PhysicalMemoryManager::force,
                            VirtualAddressSpace::KernelMode | VirtualAddressSpace::Write,
                            0x48062000))
    {
        ERROR("USB TLL: Couldn't get a memory region!");
        return;
    }
    if(!PhysicalMemoryManager::instance().allocateRegion(
                            m_MemRegionPCtl,
                            2,
                            PhysicalMemoryManager::continuous | PhysicalMemoryManager::force,
                            VirtualAddressSpace::KernelMode | VirtualAddressSpace::Write,
                            0x48005000))
    {
        ERROR("USB Power Control: Couldn't get a memory region!");
        return;
    }

    NOTICE("Initialising USB Controller...");

    // 0x33 = LEDA, LEDB ON
    // LEDA = nEN_USB_PWR, drives power to the USB port
    // LEDB = PMU STAT LED on the BeagleBoard
    uint8_t buffer[2] = {0xEE, 0x33};
    I2C::instance(0).transmit(0x4A, reinterpret_cast<uintptr_t>(buffer), 2);

    // Enable the power configuration registers, so we can enable the voltage
    // regulators for USB.
    I2C::instance(0).write(0x4b, 0x44, 0xC0); // PROTECT_KEY
    I2C::instance(0).write(0x4b, 0x44, 0x0C);

    // VUSB3V1 = active
    I2C::instance(0).write(0x4b, VUSB_DEDICATED2, 0);

    // VUSB3V1 input = VBAT
    I2C::instance(0).write(0x4b, VUSB_DEDICATED1, 0x14);
    
    // Turn on the 3.1 volt regulator
    I2C::instance(0).write(0x4b, VUSB3V1_DEV_GRP, 0x20);
    I2C::instance(0).write(0x4b, VUSB3V1_TYPE, 0);

    // Turn on the 1.5 volt regulator
    I2C::instance(0).write(0x4b, VUSB1V5_DEV_GRP, 0x20);
    I2C::instance(0).write(0x4b, VUSB1V5_TYPE, 0);

    // Turn on the 1.8 volt regulator
    I2C::instance(0).write(0x4b, VUSB1V8_DEV_GRP, 0x20);
    I2C::instance(0).write(0x4b, VUSB1V8_TYPE, 0);

    // Disable access to the power configuration registers... we're done
    I2C::instance(0).write(0x4b, 0x44, 0);

    // Enable the USB PHY for all three ports
    uint8_t power = usbReadTwl4030(0xFD);
    power &= ~1;
    usbWriteTwl4030(0xFD, power);
    usbWriteTwl4030(0xFE, usbReadTwl4030(0xFE) | (1 << 2) | (1 << 1));

    // Setup ULPI
    enablePhyAccess(true);
    usbClearBits(InterfaceControl, 1 << 2); // Disable carkit mode

    // Set the controller as active
    usbClearBits(OtgControl, 2); // Disable the D+ pull-down resistor
    usbSetBits(0xAC, 1 << 5); // Enable OTG - 0xAC = POWER_CTRL
    usbSetBits(FunctionControl, 4); // FS termination enabled
    usbClearBits(FunctionControl, 0x1B); // Enable the HS transceiver
    enablePhyAccess(false);

    // Configure the DPLL5 clock
    Prcm::instance().SelectClockPLL(4, (12 << 0) | (120 << 8));
    Prcm::instance().SelectClockPLL(5, 1);
    Prcm::instance().SetClockPLL(2, (7 << 4) | 7);
    Prcm::instance().WaitPllIdleStatus(2, 0, false); // Waiting for the bit to go to one

    // Configure the L3 and L4 clocks
    Prcm::instance().SelectClockCORE(0, Prcm::L3_CLK_DIV2);
    Prcm::instance().SelectClockCORE(2, Prcm::L3_CLK_DIV2);

    volatile uint32_t *pctl_base = reinterpret_cast<volatile uint32_t*>(m_MemRegionPCtl.virtualAddress());
    pctl_base[(0x400) / 4] = 3; // Both functional clocks enabled
    pctl_base[(0x400 + 0x10) / 4] = 1; // Interface clock enabled
    pctl_base[(0x400 + 0x30) / 4] = 0; // Disable automatic control of the interface clock enabled

    volatile uint32_t *tll_base = reinterpret_cast<volatile uint32_t*>(m_MemRegionTLL.virtualAddress());
    volatile uint8_t *ulpi_base = reinterpret_cast<volatile uint8_t*>(reinterpret_cast<uintptr_t>(m_MemRegionTLL.virtualAddress()) + 0x800);

    // Perform a PHY reset
    Gpio::instance().enableoutput(147);
    Gpio::instance().clearpin(147);
    delay(10); // Hold reset long enough

    // Enable the TLL clocks
    Prcm::instance().SetFuncClockCORE(3, 2, true);
    Prcm::instance().SetIfaceClockCORE(3, 2, true);

    // Wait for the TLL to come online - we must not access any TLL registers
    // while it's still powering up
    Prcm::instance().WaitCoreIdleStatus(3, 2, true);

    // Reset TLL
    uint32_t rev = tll_base[0];
    NOTICE("USB TLL: Revision " << Dec << ((rev >> 4) & 0xF) << "." << (rev & 0xF) << Hex << ".");
    tll_base[0x10 / 4] = 2;
    while(!(tll_base[0x14 / 4])) delay(5);

    // Disable all IDLE modes
    tll_base[0x10 / 4] = (1 << 2) | (1 << 3) | (1 << 8);

    volatile uint32_t *uhh_base = reinterpret_cast<volatile uint32_t*>(m_MemRegionUHH.virtualAddress());
    NOTICE("USB UHH: Revision " << Dec << ((uhh_base[0] >> 4) & 0xF) << "." << (uhh_base[0] & 0xF) << Hex << ".");

    // Reset the entire USB module
    uhh_base[0x10 / 4] = 2;
    while(!(uhh_base[0x14 / 4])) delay(5);

    // Set up idle mode
    uint32_t cfg = (1 << 2) | (1 << 3) | (1 << 8) | (1 << 12); // No idle
    uhh_base[0x10 / 4] = cfg;

    // Configure ULPI bypass configuration
    cfg = (1 << 2) | (1 << 3) | (1 << 4) | (1 << 5) | (1 << 8) | (1 << 9) | (1 << 10); // Connect all three ports, ULPI not UTMI
    uhh_base[0x40 / 4] = cfg;

    // Restore the PHY
    delay(10);
    Gpio::instance().drivepin(147);
}
