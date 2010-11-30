/*
 * Copyright (c) 2008 James Molloy, Jörg Pfähler, Matthew Iselin
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

#ifndef MACHINE_KEYBOARD_H
#define MACHINE_KEYBOARD_H

#include <processor/types.h>

/**
 * Keyboard device abstraction.
 */
class Keyboard
{
    public:

        enum KeyFlags
        {
            Special = 1ULL << 63,
            Ctrl    = 1ULL << 62,
            Shift   = 1ULL << 61,
            Alt     = 1ULL << 60,
            AltGr   = 1ULL << 59
        };
    
        /// Bit numbers follow the same format as the PS/2 keyboard LED byte
        enum KeyboardLeds
        {
            /// Scroll Lock LED
            ScrollLock  = 1 << 0,
        
            /// Number Lock LED
            NumLock     = 1 << 1,
            
            /// Caps Lock LED
            CapsLock    = 1 << 2,
            
            /// LEDs 1-5 cover non-standard LEDs that might be present on some
            /// keyboards - very vendor-specific.
            Led1        = 1 << 3,
            Led2        = 1 << 4,
            Led3        = 1 << 5,
            Led4        = 1 << 6,
            Led5        = 1 << 7,
        };

        virtual ~Keyboard() {}

        /**
         * Initialises the device.
         */
        virtual void initialise() =0;

        /**
         * Sets the state of the device. When debugging, it is unwise to rely on interrupt-
         * driven I/O, however in normal use polling is extremely slow and CPU-intensive.
         *
         * The debugger therefore will set the device to "debug state" by calling this function with
         * the argument "true". In "debug state", any buffered input will be discarded, the device's interrupt
         * masked, and the device will rely on polling only. This will be the default state.
         *
         * When the device is set to "normal state" by calling this function with the argument
         * "false", interrupts may be used, along with buffered input, and it is recommended that
         * during blocking I/O a Semaphore is used to signal incoming interrupts, so that the blocked thread
         * may go to sleep.
         */
        virtual void setDebugState(bool enableDebugState) =0;
        virtual bool getDebugState() =0;

        /**
         * Retrieves a character from the keyboard. Blocking I/O.
         * If DebugState is false this returns zero.
         * If DebugState is true this returns the next character received, or zero if
         * the character is non-ASCII.
         */
        virtual char getChar() =0;

        /**
         * Retrieves a character from the keyboard. Non blocking I/O.
         * If DebugState is false this returns zero.
         * If DebugState is true this returns the next character received, or zero if
         * the character is non-ASCII.
         */
        virtual char getCharNonBlock() =0;
    
        /**
         * Gets the current state of the LEDs on the keyboard.
         * A single byte bitmap is returned with flags from KeyboardLeds
         * identifying which LEDs are on or off.
         */
        virtual char getLedState()
        {
            return 0;
        }
    
        /**
         * Sets the current state of LEDs on the keyboard.
         * If a keyboard does not have any LEDs this is essentially a no-op.
         */
        virtual void setLedState(char state)
        {
        }
};

#endif
