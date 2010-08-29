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

#ifndef HIDUSAGES_H
#define HIDUSAGES_H

#include <processor/types.h>

namespace HidUsagePages
{
    /// A list of HID usage pages
    enum HidUsagePages
    {
        Undefined       = 0x00,
        GenericDesktop  = 0x01,
        Simulation      = 0x02,
        VR              = 0x03,
        Sport           = 0x04,
        Game            = 0x05,
        GenericDevice   = 0x06,
        Keyboard        = 0x07,
        Led             = 0x08,
        Button          = 0x09,
        Ordinal         = 0x0a,
        Telephony       = 0x0b,
        Consumer        = 0x0c,
        Digitizer       = 0x0d,
        Pid             = 0x0f,
        Unicode         = 0x10
    };
};

namespace HidUsages
{
    /// A list of HID usages
    enum HidUsages
    {
        // Generic Desktop Page
        Pointer     = 0x01,
        Mouse       = 0x02,
        Joystick    = 0x04,
        GamePad     = 0x05,
        Keyboard    = 0x06,
        Keypad      = 0x07,
        X           = 0x30,
        Y           = 0x31,
        Z           = 0x32,
        Rx          = 0x33,
        Ry          = 0x34,
        Rz          = 0x35,
        Wheel       = 0x38,
    };
};

#endif
