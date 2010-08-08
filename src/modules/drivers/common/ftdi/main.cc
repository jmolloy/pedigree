/*
 * Copyright (c) 2010 Eduard Burtescu
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

#include "FtdiSerialDevice.h"

void ftdiConnected(UsbDevice *pDevice)
{
    FtdiSerialDevice *pFtdi = new FtdiSerialDevice(pDevice);
    pFtdi->getParent()->replaceChild(pDevice, pFtdi);
}

static void entry()
{
    UsbPnP::instance().registerCallback(0x0403, 0x6001, ftdiConnected);
}

void exit()
{
}

MODULE_INFO("ftdi", &entry, &exit, "usb");