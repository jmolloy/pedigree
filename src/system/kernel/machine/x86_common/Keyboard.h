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

#ifndef MACHINE_X86_KEYBOARD_H
#define MACHINE_X86_KEYBOARD_H

#include <machine/Keyboard.h>
#include <machine/IrqManager.h>
#include <machine/KeymapManager.h>
#include <processor/types.h>
#include <processor/IoPort.h>
#include <process/Semaphore.h>

#define BUFLEN 256

/**
 * Keyboard device implementation
 */
class X86Keyboard : public Keyboard, private IrqHandler
{
    public:
        X86Keyboard (uint32_t portBase);
        virtual ~X86Keyboard();

        /// Initialises the device
        virtual void initialise();

        virtual void setDebugState(bool enableDebugState);
        virtual bool getDebugState();

        virtual char getChar();
        virtual char getCharNonBlock();
    
        virtual char getLedState();
        virtual void setLedState(char state);

        /// IrqHandler interface
        virtual bool irq(irq_id_t number, InterruptState &state);

    private:
        /// Converts a scancode into an ASCII character (for use in debug state)
        char scancodeToAscii(uint8_t scancode);

        /// True if we're in debug state
        bool m_bDebugState;

        /// The current escape state
        KeymapManager::EscapeState m_Escape;

        /// The IO port through which to access the keyboard
        IoBase *m_pBase;

        /// Circular input buffer
        uint64_t m_Buffer[BUFLEN];
        int m_BufStart, m_BufEnd;

        /// Semaphore for how many items are in the buffer
        Semaphore m_BufLength;

        /// IRQ id
        irq_id_t m_IrqId;
    
        /// Current LED state
        char m_LedState;
};

#endif
