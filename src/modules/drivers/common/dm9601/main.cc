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

#include <Module.h>
#include <usb/UsbPnP.h>

#include "Dm9601.h"

static struct
{
    uint16_t vendor;
    uint16_t product;
} g_Devices[] = {
    {0x2630, 0x9601}, // Specification defines these for the chip
    {0x0fe6, 0x8101}, // Kontron product
};

#define NUM_DEVICES (sizeof(g_Devices) / sizeof(g_Devices[0]))

UsbDevice *dm9601Connected(UsbDevice *pDevice)
{
    return new Dm9601(pDevice);
}

static void entry()
{
    for(size_t i = 0; i < NUM_DEVICES; i++)
        UsbPnP::instance().registerCallback(g_Devices[i].vendor, g_Devices[i].product, dm9601Connected);
}

static void exit()
{
}

MODULE_INFO("dm9601", &entry, &exit, "usb", "network-stack");
