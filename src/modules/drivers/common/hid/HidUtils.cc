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

#include <hid/HidUsages.h>
#include <hid/HidUtils.h>
#include <machine/HidInputManager.h>
#include <machine/InputManager.h>

uint64_t HidUtils::getBufferField(uint8_t *pBuffer, size_t nStart, size_t nLength)
{
    uint64_t nValue = 0;
    size_t i = 0;
    while(i < nLength)
    {
        uint8_t nBits = ((nStart % 8) + nLength - i) < 8 ? nLength % 8 : 8 - (nStart % 8);
        nValue |= ((pBuffer[nStart / 8] >> (nStart % 8)) & ((1 << nBits) - 1)) << i;
        i += nBits;
        nStart += nBits;
    }
    return nValue;
}

void HidUtils::fixNegativeMinimum(int64_t &nMin, int64_t nMax)
{
    // Ignore the call if nMax is larger than nMin (no sign bit set in nMin)
    if(nMax > nMin)
      return;

    // Check for all possible sign bits
    // NOTE this was "nMin = -((1 << bitNum) - nMin)" originally, now may seem confusing
    if(nMin < (1LL << 8) && nMin & (1LL << 7))          // Signed 8-bit
        nMin -= 1LL << 8;     // Turn nMin negative
    else if(nMin < (1LL << 16) && nMin & (1LL << 15))   // Signed 16-bit
        nMin -= 1LL << 16;    // Turn nMin negative
    else if(nMin < (1LL << 32) && nMin & (1LL << 31))   // Signed 32-bit
        nMin -= 1LL << 32;    // Turn nMin negative
}

void HidUtils::fixNegativeValue(int64_t nMin, int64_t nMax, int64_t &nValue)
{
    // Ignore the call if nMin is not negative at all
    if(nMin >= 0)
        return;

    // Ignore the call if nMax is larger than nValue (no sign bit set in nValue)
    if(nMax > nValue)
      return;

    // Check for all possible sign bits
    if(nMin > -(1LL << 8) && nValue < (1LL << 8) && nValue & (1LL << 7))          // Signed 8-bit
        nValue -= 1LL << 8;   // Turn nValue negative
    else if(nMin > -(1LL << 16) && nValue < (1LL << 16) && nValue & (1LL << 15))  // Signed 16-bit
        nValue -= 1LL << 16;  // Turn nValue negative
    else if(nMin > -(1LL << 32) && nValue < (1LL << 32) && nValue & (1LL << 31))  // Signed 32-bit
        nValue -= 1LL << 32;  // Turn nValue negative
}

void HidUtils::sendInputToManager(HidDeviceType deviceType, uint16_t nUsagePage, uint16_t nUsage, int64_t nRelativeValue)
{
    // Button bitmaps
    /// \todo Matt, fix the damn input manager!!!
    static uint32_t mouseButtons = 0;
    static uint32_t joystickButtons = 0;

    // Is this a key on a keyboard/keypad?
    if((deviceType == Keyboard) && (nUsagePage == HidUsagePages::Keyboard))
    {
        if(nRelativeValue > 0)
            HidInputManager::instance().keyDown(nUsage);
        else
            HidInputManager::instance().keyUp(nUsage);
    }

    // Is this an axis on a mouse?
    if((deviceType == Mouse) && (nUsagePage == HidUsagePages::GenericDesktop))
    {
        if(nUsage == HidUsages::X)
            InputManager::instance().mouseUpdate(nRelativeValue, 0, 0, mouseButtons);
        if(nUsage == HidUsages::Y)
            InputManager::instance().mouseUpdate(0, nRelativeValue, 0, mouseButtons);
        if(nUsage == HidUsages::Wheel)
            InputManager::instance().mouseUpdate(0, 0, nRelativeValue, mouseButtons);
    }

    // Is this an axis on a joystick?
    if((deviceType == Joystick) && (nUsagePage == HidUsagePages::GenericDesktop))
    {
        if(nUsage == HidUsages::X)
            InputManager::instance().joystickUpdate(nRelativeValue, 0, 0, joystickButtons);
        if(nUsage == HidUsages::Y)
            InputManager::instance().joystickUpdate(0, nRelativeValue, 0, joystickButtons);
        if(nUsage == HidUsages::Z)
            InputManager::instance().joystickUpdate(0, 0, nRelativeValue, joystickButtons);
    }

    // Is this a button on a mouse?
    if((deviceType == Mouse) && (nUsagePage == HidUsagePages::Button))
    {
        // Set/unset the bit in the bitmap
        if(nRelativeValue > 0)
            mouseButtons |= 1 << (nUsage - 1);
        else
            mouseButtons &= ~(1 << (nUsage - 1));

        // Send the new bitmap to the input manager
        InputManager::instance().mouseUpdate(0, 0, 0, mouseButtons);
    }

    // Is this a button on a joystick?
    if((deviceType == Joystick) && (nUsagePage == HidUsagePages::Button))
    {
        // Set/unset the bit in the bitmap
        if(nRelativeValue > 0)
            joystickButtons |= 1 << (nUsage - 1);
        else
            joystickButtons &= ~(1 << (nUsage - 1));

        // Send the new bitmap to the input manager
        InputManager::instance().joystickUpdate(0, 0, 0, joystickButtons);
    }
}
