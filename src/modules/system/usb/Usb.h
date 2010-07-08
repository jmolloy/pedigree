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

#ifndef USB_H
#define USB_H

#include <processor/types.h>

enum UsbPid {
    UsbPidSetup = 0x2d,
    UsbPidIn = 0x69,
    UsbPidOut = 0xe1,
};

enum UsbSpeed
{
    LowSpeed = 0,
    FullSpeed,
    HighSpeed,
    SuperSpeed
};

typedef struct UsbEndpoint
{
    inline UsbEndpoint(uint8_t address, uint8_t endpoint, bool dataToggle, UsbSpeed _speed) :
        nAddress(address), nEndpoint(endpoint), bDataToggle(dataToggle), speed(_speed), nHubAddress(0) {}

    uint8_t nAddress;
    uint8_t nEndpoint;
    bool bDataToggle;
    UsbSpeed speed;
    uint8_t nHubAddress;
    uint8_t nHubPort;

    const char *dumpSpeed()
    {
        if(speed == HighSpeed)
            return "High Speed";
        if(speed == FullSpeed)
            return "Full Speed";
        if(speed == LowSpeed)
            return "Low Speed";
        return "";
    }
} UsbEndpoint;

#endif
