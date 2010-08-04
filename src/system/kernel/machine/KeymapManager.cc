/*
 * Copyright (c) 2010 Burtescu Eduard
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

#include <machine/KeymapManager.h>
#include <machine/HidInputManager.h>
#include <machine/Keyboard.h>
#include <Log.h>

#include <machine/keymaps/KeymapEnUs.h>

#define KEYMAP_INDEX(combinator, modifiers, scancode) (((combinator & 0xFF) << 11) | ((modifiers & 0xF) << 7) | (scancode & 0x7F))
#define KEYMAP_MAX_INDEX KEYMAP_INDEX(0xFF,0xF,0x7F)

KeymapManager KeymapManager::m_Instance;

KeymapManager::KeymapManager() :
    m_pSparseTable(reinterpret_cast<uint8_t*>(sparseBuff)),
    m_pDataTable(reinterpret_cast<uint8_t*>(dataBuff)),
    m_bLeftCtrl(false), m_bLeftShift(false), m_bLeftAlt(false),
    m_bRightCtrl(false), m_bRightShift(false), m_bRightAlt(false),
    m_bCapsLock(false), m_nCombinator(0)
{
}

KeymapManager::~KeymapManager()
{
}

void KeymapManager::useKeymap(uint8_t *pSparseTable, uint8_t *pDataTable)
{
    // Set the table pointers
    m_pSparseTable = pSparseTable;
    m_pDataTable = pDataTable;

    // Make the HID input manager update all its keys
    HidInputManager::instance().updateKeys();
}

bool KeymapManager::handleHidModifier(uint8_t keyCode, bool bDown)
{
    // If the key isn't a modifier, return false right away
    if(!((keyCode >= HidLeftCtrl && keyCode <= HidRightGui) || keyCode == HidCapsLock))
        return false;
    // Handle modifier keys, enabled on keyDown, disabled on keyUp
    if(keyCode == HidLeftCtrl)
        m_bLeftCtrl = bDown;
    if(keyCode == HidRightCtrl)
        m_bRightCtrl = bDown;
    if(keyCode == HidLeftShift)
        m_bLeftShift = bDown;
    if(keyCode == HidRightShift)
        m_bRightShift = bDown;
    if(keyCode == HidLeftAlt)
        m_bLeftAlt = bDown;
    if(keyCode == HidRightAlt)
        m_bRightAlt = bDown;
    // Handle CapsLock key, changes on keyUp
    if(keyCode == HidCapsLock && !bDown)
        m_bCapsLock = !m_bCapsLock;
    return true;
}

uint64_t KeymapManager::resolveHidKeycode(uint8_t keyCode)
{
    // Get the modifiers
    bool bCtrl = m_bLeftCtrl || m_bRightCtrl;
    bool bShift = m_bLeftShift || m_bRightShift;
    bool bAlt = m_bLeftAlt, bAltGr = m_bRightAlt;

    bool bUseUpper = false; // Use the upper case keymap

    if(m_bCapsLock ^ bShift)
        bUseUpper = true;

    // Try and grab a keymap entry for the scancode with all modifiers enabled.
    KeymapEntry *pKeymapEntry = getKeymapEntry(bCtrl, bShift, bAlt, bAltGr, m_nCombinator, keyCode);
    // Fallback and try without combinator
    if(!pKeymapEntry || (!pKeymapEntry->value && !pKeymapEntry->flags))
        pKeymapEntry = getKeymapEntry(bCtrl, bShift, bAlt, bAltGr, 0, keyCode);
    // Fallback and try without combinator and Ctrl
    if(!pKeymapEntry || (!pKeymapEntry->value && !pKeymapEntry->flags))
        pKeymapEntry = getKeymapEntry(false, bShift, bAlt, bAltGr, 0, keyCode);
    // Fallback and try with only Shift
    if(!pKeymapEntry || (!pKeymapEntry->value && !pKeymapEntry->flags))
        pKeymapEntry = getKeymapEntry(false, bShift, false, false, 0, keyCode);
    // Fallback and try with no modifier enabled
    if(!pKeymapEntry || (!pKeymapEntry->value && !pKeymapEntry->flags))
        pKeymapEntry = getKeymapEntry(false, false, false, false, 0, keyCode);
    // This key has no entry at all in the keymap
    if(!pKeymapEntry || (!pKeymapEntry->value && !pKeymapEntry->flags))
        return 0;

    // Does this key set any combinator?
    uint32_t nCombinator = pKeymapEntry->flags & 0xFF;
    if(nCombinator)
    {

        // If the key sets the same combinator that we're currently using,
        // the "dead" key becomes live and shows the default key.
        if(nCombinator == m_nCombinator)
        {
            // Unset combinator and fall through to display default glyph
            m_nCombinator = 0;
        }
        else
        {
            // Change combinator
            m_nCombinator = nCombinator;
            return 0;
        }
    }
    else
    {
        // Dead keys should be reset here
        m_nCombinator = 0;
    }

    uint64_t key = pKeymapEntry->value;

    if(pKeymapEntry->flags & KeymapEntry::Special)
        key |= Keyboard::Special;
    else if(bUseUpper && key >= 'a' && key <= 'z')
        key -= ('a' - 'A');
    else if(!bUseUpper && key >= 'A' && key <= 'Z')
        key += ('a' - 'A');

    if(bCtrl)
        key |= Keyboard::Ctrl;
    if(bShift)
        key |= Keyboard::Shift;
    if(bAlt)
        key |= Keyboard::Alt;
    if(bAltGr)
        key |= Keyboard::AltGr;

    return key;
}

KeymapManager::KeymapEntry *KeymapManager::getKeymapEntry(bool bCtrl, bool bShift, bool bAlt, bool bAltGr, uint8_t nCombinator, uint8_t keyCode)
{
    // Grab the keymap table index for this key
    size_t modifiers = 0;
    if(bCtrl)
        modifiers |= IndexCtrl;
    if(bShift)
        modifiers |= IndexShift;
    if(bAlt)
        modifiers |= IndexAlt;
    if(bAltGr)
        modifiers |= IndexAltGr;
    size_t nIndex = KEYMAP_INDEX(nCombinator, modifiers, keyCode);

    // Now walk the sparse tree
    size_t bisect = (KEYMAP_MAX_INDEX + 1) / 2;
    size_t size = (KEYMAP_MAX_INDEX + 1) / 2;
    SparseEntry *pSparse = reinterpret_cast<SparseEntry*>(&m_pSparseTable[0]);
    size_t nDataIndex = 0;
    while(true)
    {
        if(nIndex < bisect)
        {
            if(!pSparse->left)
                return 0;
            if(pSparse->left & SparseEntry::DataFlag)
            {
                size_t nOffset = nIndex - (bisect - size);
                nDataIndex = (pSparse->left & ~SparseEntry::DataFlag) + nOffset * sizeof(KeymapEntry);
                break;
            }
            size /= 2;
            bisect = bisect - size;
            pSparse = reinterpret_cast<SparseEntry*>(&m_pSparseTable[pSparse->left]);
        }
        else
        {
            if(!pSparse->right)
                return 0;
            if(pSparse->right & SparseEntry::DataFlag)
            {
                size_t nOffset = nIndex - bisect;
                nDataIndex = (pSparse->right & ~SparseEntry::DataFlag) + nOffset * sizeof(KeymapEntry);
                break;
            }
            size /= 2;
            bisect = bisect + size;
            pSparse = reinterpret_cast<SparseEntry*>(&m_pSparseTable[pSparse->right]);
        }
    }

    // Return the found keymap entry
    return reinterpret_cast<KeymapEntry*>(&m_pDataTable[nDataIndex]);
}

// These tables originated from qemu
// Copyright (c) 2007 OpenMoko, Inc.  (andrew@openedhand.com)
static const uint8_t pc102ToHidTableNormal[] =
{
    0x00, 0x29, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23,
    0x24, 0x25, 0x26, 0x27, 0x2d, 0x2e, 0x2a, 0x2b,
    0x14, 0x1a, 0x08, 0x15, 0x17, 0x1c, 0x18, 0x0c,
    0x12, 0x13, 0x2f, 0x30, 0x28, 0xe0, 0x04, 0x16,
    0x07, 0x09, 0x0a, 0x0b, 0x0d, 0x0e, 0x0f, 0x33,
    0x34, 0x35, 0xe1, 0x31, 0x1d, 0x1b, 0x06, 0x19,
    0x05, 0x11, 0x10, 0x36, 0x37, 0x38, 0xe5, 0x55,
    0xe2, 0x2c, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e,
    0x3f, 0x40, 0x41, 0x42, 0x43, 0x53, 0x47, 0x5f,
    0x60, 0x61, 0x56, 0x5c, 0x5d, 0x5e, 0x57, 0x59,
    0x5a, 0x5b, 0x62, 0x63, 0x00, 0x00, 0x64, 0x44,
    0x45, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e,
    0x00, 0x00, 0x71, 0x72, 0x73, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x85, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0xe3, 0xe7, 0x65
};

static const uint8_t pc102ToHidTableEscape[] =
{
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x58, 0xe4, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x54, 0x00, 0x46,
    0xe6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x48, 0x00, 0x4a,
    0x52, 0x4b, 0x00, 0x50, 0x00, 0x4f, 0x00, 0x4d,
    0x51, 0x4e, 0x49, 0x4c, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

uint8_t KeymapManager::convertPc102ScancodeToHidKeycode(uint8_t scancode, EscapeState &escape)
{
    // Handle escape scancode 0xE0
    if(scancode == 0xE0)
    {
        escape = EscapeE0;
        return 0;
    }

    // Handle escape scancode 0xE1
    if(scancode == 0xE1)
    {
        escape = EscapeE1;
        return 0;
    }

    // Treat 0xE1 0x1D as 0xE0 (escape scancode)
    if(scancode == 0x1D && escape == EscapeE1)
    {
        escape = EscapeE0;
        return 0;
    }

    if(escape)
    {
        escape = EscapeNone;
        return pc102ToHidTableEscape[scancode & 0x7F];
    }
    else
        return pc102ToHidTableNormal[scancode & 0x7F];
}