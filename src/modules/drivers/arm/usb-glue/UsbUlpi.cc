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
#include <Log.h>

/// \todo super specific to a machine
#include <Prcm.h>

UsbUlpi UsbUlpi::m_Instance;

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
    pctl_base[(0x400) / 4] = 1;
    pctl_base[(0x400 + 0x10) / 4] = 1;
    pctl_base[(0x400 + 0x20) / 4] = 0;
    pctl_base[(0x400 + 0x4C) / 4] = 1;

    Prcm::instance().SetIfaceClockCORE(3, 2, true);

    volatile uint8_t *uhh_base = reinterpret_cast<volatile uint8_t*>(m_MemRegionUHH.virtualAddress());
    NOTICE("USB UHH: Revision " << Dec << ((uhh_base[0] >> 4) & 0xF) << "." << (uhh_base[0] & 0xF) << Hex << ".");

    // Reset the entire USB module
    uhh_base[0x10] = 2;
    while(!(uhh_base[0x14]));

    // Set up power management etc
    uhh_base[0x10] = 8; // No idle
    uhh_base[0x11] = 0x11; // No standby and clock is always on
}
