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

UsbUlpi UsbUlpi::m_Instance;

#define delay(n) do{Semaphore semWAIT(0);semWAIT.acquire(1, 0, n*1000);}while(0)

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
                            m_MemRegionPCtl,
                            2,
                            PhysicalMemoryManager::continuous | PhysicalMemoryManager::force,
                            VirtualAddressSpace::KernelMode | VirtualAddressSpace::Write,
                            0x48005000))
    {
        ERROR("USB TLL: Couldn't get a memory region!");
        return;
    }

    volatile uint32_t *pctl_base = reinterpret_cast<volatile uint32_t*>(m_MemRegionPCtl.virtualAddress());
    pctl_base[(0x400 + 0x20) / 4] = 0; // USB HOST is active
    pctl_base[(0x400 + 0x10) / 4] = 1; // Interface clock enabled
    pctl_base[(0x400 + 0x4C) / 4] = 1; // Domain interface clock active
    pctl_base[(0x400) / 4] = 3; // Both functional clocks enabled

    volatile uint32_t *uhh_base = reinterpret_cast<volatile uint32_t*>(m_MemRegionUHH.virtualAddress());
    NOTICE("USB UHH: Revision " << Dec << ((uhh_base[0] >> 4) & 0xF) << "." << (uhh_base[0] & 0xF) << Hex << ".");

    // Reset the entire USB module
    uhh_base[0x10 / 4] = 2;
    while(!(uhh_base[0x14 / 4])) delay(5);

    // Set up power management etc
    uhh_base[0x10 / 4] = 0; // No idle
    uhh_base[0x40 / 4] = 0x700; // Enable all three USB ports

    // 0x33 = LEDA, LEDB ON
    // LEDA = nEN_USB_PWR, drives power to the USB port
    // LEDB = PMU STAT LED on the BeagleBoard
    uint8_t buffer[2] = {0xEE, 0x33};
    I2C::instance(0).transmit(0x4A, reinterpret_cast<uintptr_t>(buffer), 2);
}
