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

#include <usb/UsbDevice.h>
#include <usb/FtdiSerialDevice.h>

#define FTDI_BAUD_RATE 9600

FtdiSerialDevice::FtdiSerialDevice(UsbDevice *dev) : Device(dev), UsbDevice(dev)
{
    controlRequest(0x40, 0, 0, 0);

    uint16_t nDivisor = (48000000 / 2) / FTDI_BAUD_RATE, nSubdivisor = nDivisor % 8, nSubdivisors[8] = {0, 3, 2, 4, 1, 5, 6, 7};
    nDivisor /= 8;
    nSubdivisor = nSubdivisors[nSubdivisor];
    NOTICE("DD "<<((nSubdivisor&3)<<14 | nDivisor)<<" "<<(nSubdivisor>>2));
    controlRequest(0x40, 3, (nSubdivisor&3)<<14 | nDivisor, nSubdivisor>>2);// 0x4138
    /*write('A');
    write(' ');write(' ');write(' ');write(' ');write(' ');write(' ');write(' ');write(' ');
    write('B');
    write(' ');write(' ');write(' ');write(' ');write(' ');
    write('C');
    write(' ');write(' ');write(' ');write(' ');write(' ');write(' ');write(' ');write(' ');
    write('B');*/
}

FtdiSerialDevice::~FtdiSerialDevice()
{
}

char FtdiSerialDevice::read()
{
    char c = 0;
    syncIn(1, reinterpret_cast<uintptr_t>(&c), 1);
    return c;
}

void FtdiSerialDevice::write(char c)
{
    syncOut(2, reinterpret_cast<uintptr_t>(&c), 1);
}
