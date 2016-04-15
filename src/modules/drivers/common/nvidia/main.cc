/*
 * Copyright (c) 2008-2014, Pedigree Developers
 *
 * Please see the CONTRIB file in the root of the source tree for a full
 * list of contributors.
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

#include <Log.h>
#include <Module.h>
#include <processor/types.h>
#include <processor/Processor.h>
#include <utilities/StaticString.h>
#include <utilities/List.h>
#include <machine/Machine.h>
#include <machine/Display.h>
#include <processor/MemoryMappedIo.h>

#include "Dma.h"

static bool bFound = false;

void probeDevice(Device *pDev)
{
    NOTICE("Nvidia found");

    IoBase *pRegs = 0;
    for (size_t i = 0; i < pDev->addresses().count(); i++)
        if (!StringCompare(pDev->addresses()[i]->m_Name, "bar0"))
            pRegs = pDev->addresses()[i]->m_Io;

    IoBase *pFb = 0;
    for (size_t i = 0; i < pDev->addresses().count(); i++)
        if (!StringCompare(pDev->addresses()[i]->m_Name, "bar1"))
            pFb = pDev->addresses()[i]->m_Io;

    // I have an NV34 type, NV30A arch chip.
    /// \todo Fix this driver.
    return;
    uint32_t strapinfo = pRegs->read32(NV32_NV10STRAPINFO);
    NOTICE("Strapinfo: " << Hex << strapinfo << ", ram: " << ((strapinfo&0x3ff00000) >> 20));
    // Processor::breakpoint();
    // Pink background to make sure we see any effects - engine may accidentally blit black pixels for example, and we won't pick that
    // up on a black background.
    for (int x = 0; x < 512; x++)
        for (int y = 0; y < 512; y++)
            pFb->write16(0xF00F, (y * 1024 + x) * 2);

    // Draw an image.
    for (int x = 0; x < 128; x++)
        for (int y = 256; y < 128+256; y++)
            pFb->write16(0xFFFF, (y * 1024 + x) * 2);

    // Card detection is done but not coded yet - hardcoded to the device class in my testbed.
    Dma *pDma = new Dma(pRegs, pFb, NV40A, NV40/*NV30A, NV34*/, 0x300000 /* Card RAM - hardcoded for now */);
    pDma->init();
    for(;;);
    pDma->screenToScreenBlit(0, 256, 400, 100, 100, 100);

    pDma->fillRectangle(600, 600, 100, 100);

    for(;;);

    bFound = true;
}

bool entry()
{
    Device::root().searchByVendorId(0x10DE, probeDevice);

    return bFound;
}

void exit()
{
}

MODULE_INFO("nvidia", &entry, &exit, "pci");
