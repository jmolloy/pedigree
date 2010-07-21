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
#include <utilities/List.h>

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

struct QHMetaData
{
    // uintptr_t pCallback;
    List<uint32_t*> pParam; /// Stores all results from each qTD in this queue head
	uintptr_t pSemaphore;
    uintptr_t pBuffer;
    uint16_t nBufferSize;
    uint16_t nBufferOffset;
	size_t qTDCount; /// Number of qTDs related to this queue head, for semaphore wakeup
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
