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

// PID types, ordered as they appear in the EHCI spec
enum UsbPid {
    // Token PID Types
    UsbPidOut   = 0xe1,
    UsbPidIn    = 0x69,
    UsbPidSOF   = 0xa5,
    UsbPidSetup = 0x2d,

    // Data PID Types
    UsbPidData0 = 0xc3,
    UsbPidData1 = 0x4b,
    UsbPidData2 = 0x87,
    UsbPidMdata = 0x0f,

    // Handshake PID Types
    UsbPidAck   = 0xd2,
    UsbPidNak   = 0x5a,
    UsbPidStall = 0x1e,
    UsbPidNyet  = 0x96,

    // Special PID Types
    UsbPidPre   = 0x3c, // Token
    UsbPidErr   = 0x3c, // Handshake
    UsbPidSplit = 0x78,
    UsbPidPing  = 0xb4,
    UsbPidRsvd  = 0xf0
};

enum UsbSpeed
{
    LowSpeed = 0,
    FullSpeed,
    HighSpeed,
    SuperSpeed
};

enum UsbError
{
    Stall = 1,
    NakNyet,
    Timeout,
    Babble,
    CrcError,
    TransactionError
};

struct UsbEndpoint
{
    inline UsbEndpoint() :
        nAddress(0), nEndpoint(0), speed(LowSpeed), nMaxPacketSize(8), nHubAddress(0), nHubPort(0) {}
    inline UsbEndpoint(uint8_t address, uint8_t hubPort, uint8_t endpoint, UsbSpeed _speed, size_t maxPacketSize) :
        nAddress(address), nEndpoint(endpoint), speed(_speed), nMaxPacketSize(maxPacketSize), nHubAddress(0), nHubPort(hubPort) {}

    uint8_t nAddress;
    uint8_t nEndpoint;
    UsbSpeed speed;
    size_t nMaxPacketSize;
    uint8_t nHubAddress;
    uint8_t nHubPort;

    UsbEndpoint &operator = (UsbEndpoint &e)
    {
        nAddress = e.nAddress;
        nEndpoint = e.nEndpoint;
        speed = e.speed;
        nMaxPacketSize = e.nMaxPacketSize;
        nHubAddress = e.nHubAddress;
        nHubPort = e.nHubPort;

        return *const_cast<UsbEndpoint*>(this);
    }
};

#endif
