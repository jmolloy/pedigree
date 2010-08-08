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

#ifndef USBCONSTANTS_H
#define USBCONSTANTS_H

#include <processor/types.h>

namespace UsbRequestType
{
    enum RequestType
    {
        Standard    = 0x00,
        Class       = 0x20,
        Vendor      = 0x40
    };
};

namespace UsbRequestRecipient
{
    enum RequestRecipient
    {
        Device      = 0x00,
        Interface   = 0x01,
        Endpoint    = 0x02,
        Other       = 0x03
    };
};

namespace UsbRequestDirection
{
    enum RequestDirection
    {
        Out = 0x00,
        In  = 0x80
    };
};

namespace UsbRequest
{
    enum Request
    {
        GetStatus       = 0,
        ClearFeature    = 1,
        SetFeature      = 3,
        SetAddress      = 5,
        GetDescriptor   = 6,
        SetDescriptor   = 7,
        GetConfiguration= 8,
        SetConfiguration= 9,
        GetInterface    = 10,
        SetInterface    = 11,
        SynchFrame      = 12,
    };
};

namespace UsbDescriptor
{
    enum Descriptor
    {
        Device                  = 1,
        Configuration           = 2,
        String                  = 3,
        Interface               = 4,
        Endpoint                = 5,
        DeviceQualifier         = 6,
        OtherSpeedConfiguration = 7,
    };
};

#endif
