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
#ifndef MACHINE_HID_INPUT_MANAGER_H
#define MACHINE_HID_INPUT_MANAGER_H

#include <processor/Processor.h>
#include <processor/types.h>
#include <utilities/Tree.h>
#include <machine/Timer.h>
#include <Spinlock.h>

/**
 * Global manager for all input from HID devices.
 */
class HidInputManager : public TimerHandler
{
    public:

        /// Default constructor
        HidInputManager();

        /// Default destructor
        virtual ~HidInputManager();

        /// Singleton design
        static HidInputManager& instance()
        {
            return m_Instance;
        }

        /// Called when a key transitions to the "down" state
        void keyDown(uint8_t keyCode);

        /// Called when a key transitions to the "up" state
        void keyUp(uint8_t keyCode);

        /// Apply modifier or keymap changes to all keys in down state
        void updateKeys();

        /// Timer callback to handle repeating key press states
        /// when a key is held in the down state.
        void timer(uint64_t delta, InterruptState &state);

    private:
        /// Static instance
        static HidInputManager m_Instance;

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
