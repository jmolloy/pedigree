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
#ifndef MACHINE_KEYMAP_MANAGER_H
#define MACHINE_KEYMAP_MANAGER_H

#include <processor/Processor.h>
#include <processor/types.h>

/**
 * Global manager for keymaps
 */
class KeymapManager
{
    public:
        /// Default constructor
        KeymapManager();

        /// Default destructor
        virtual ~KeymapManager();

        /// Singleton design
        static KeymapManager& instance()
        {
            return m_Instance;
        }

        /// Changes the current keymap to the given one
        void useKeymap(uint8_t *pSparseTable, uint8_t *pDataTable);

        /// If the given keycode is a modifier, applies it and
        /// returns true, otherwise it returns false.
        bool handleHidModifier(uint8_t keyCode, bool bDown);

        /// Resolves a HID keycode using the current keymap and
        /// returns a "real" character in UTF-32 format, plus
        /// modifier flags in the top 32-bits.
        uint64_t resolveHidKeycode(uint8_t keyCode);

        enum EscapeState
        {
            EscapeNone = 0,
            EscapeE0,
            EscapeE1,
        };

        /// Converts a pc102 scancode into a HID keycode
        uint8_t convertPc102ScancodeToHidKeycode(uint8_t scancode, EscapeState &escape);

    private:

        enum IndexModifiers
        {
            IndexCtrl   = 1,
            IndexShift  = 2,
            IndexAlt    = 4,
            IndexAltGr  = 8
        };

        /// HID keycodes corresponding to modifiers
        enum HidModifiers
        {
            HidCapsLock     = 0x39,
            HidLeftCtrl     = 0xE0,
            HidLeftShift    = 0xE1,
            HidLeftAlt      = 0xE2,
            HidLeftGui      = 0xE3,
            HidRightCtrl    = 0xE4,
            HidRightShift   = 0xE5,
            HidRightAlt     = 0xE6,
            HidRightGui     = 0xE7,
        };

        /// Structure representing an entry in the keymap table
        struct KeymapEntry
        {
            enum Flags
            {
                Special = 0x80000000
            };
            uint32_t flags;
            uint32_t value;
        };

        /// Structure representing an entry in the sparse table
        struct SparseEntry
        {
            enum Flags
            {
                DataFlag = 0x8000
            };
            uint16_t left;
            uint16_t right;
        };

        /// Returns the keymap entry corresponding to given keycode and modifiers
        KeymapEntry *getKeymapEntry(bool bCtrl, bool bShift, bool bAlt, bool bAltGr, uint8_t nCombinator, uint8_t keyCode);

        /// Static instance
        static KeymapManager m_Instance;

        /// The sparse and data tables for the current keymap
        uint8_t *m_pSparseTable;
        uint8_t *m_pDataTable;

        /// State of the modifiers, true if down, false if up
        bool m_bLeftCtrl;
        bool m_bLeftShift;
        bool m_bLeftAlt;
        bool m_bRightCtrl;
        bool m_bRightShift;
        bool m_bRightAlt;

        /// True if caps lock is on
        bool m_bCapsLock;

        /// Index of the current active combinator, if any
        uint8_t m_nCombinator;

        /// Item in the key state list. This stores information needed
        /// for periodic callbacks and applying modifiers.
        struct KeyState
        {
            /// The resolved key
            uint64_t key;

            /// The time left until the next key repeat
            uint64_t nLeftTicks;
        };

        /// Current key states (for periodic callbacks while a key is down)
        Tree<uint8_t, KeyState*> m_KeyStates;

        /// Spinlock for work on keys.
        /// \note Using a Spinlock here because a lot of our work will happen
        ///       in the middle of an IRQ where it's potentially dangerous to
        ///       reschedule (which may happen with a Mutex or Semaphore).
        Spinlock m_KeyLock;
};

#endif
