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

#ifndef HIDUTILS_H
#define HIDUTILS_H

#include <processor/types.h>

/// Enum used to distinguish between various input devices
enum HidDeviceType
{
    Keyboard,
    Mouse,
    Joystick,
    UnknownDevice = ~0
};

/// Various utility functions used in HID
namespace HidUtils
{
    /// Retrieves a field in a buffer
    uint64_t getBufferField(uint8_t *pBuffer, size_t nStart, size_t nLength);

    /// Converts \param nMin to a negative value, if it has the signed bit set
    void fixNegativeMinimum(int64_t &nMin, int64_t nMax);

    /// Converts \param nValue to a negative value, if it has the signed bit set
    void fixNegativeValue(int64_t nMin, int64_t nMax, int64_t &nValue);

    /// Sends the input to the right handler in HidInputManager or InputManager
    void sendInputToManager(HidDeviceType deviceType, uint16_t nUsagePage, uint16_t nUsage, int64_t nRelativeValue);
};

#endif
