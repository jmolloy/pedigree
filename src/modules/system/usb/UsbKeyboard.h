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

#ifndef USBKEYBOARD_H
#define USBKEYBOARD_H

#include "../../../system/kernel/machine/x86_common/Keyboard.h"
#include <machine/Keyboard.h>
#include <processor/types.h>
#include <process/Semaphore.h>
#include <usb/UsbConstants.h>
#include <usb/UsbDevice.h>
#include <usb/UsbManager.h>

#define BUFLEN 256

/**
* Keyboard device abstraction.
*/
class UsbKeyboard : public Keyboard, public UsbDevice
{
    public:
        UsbKeyboard (UsbDevice *dev);
        virtual ~UsbKeyboard();

        virtual void initialise()
        {};

        virtual void setDebugState(bool enableDebugState);
        virtual bool getDebugState();

        virtual char getChar()
        {return 0;};
        virtual char getCharNonBlock()
        {return 0;};

        virtual uint64_t getCharacter();
        virtual uint64_t getCharacterNonBlock();

        virtual void registerCallback(KeyPressedCallback callback)
        {
            m_Callback = callback;
            new Thread(Processor::information().getCurrentThread()->getParent(),
                    reinterpret_cast<Thread::ThreadStartFunc> (&trampoline),
                    reinterpret_cast<void*> (this));
        }

    private:
        /**
        * Converts a scancode into a "real" character in UTF-32 format, plus
        * modifier flags in the top 32-bits.
        */
        uint64_t scancodeToCharacter(uint8_t scancode);

        struct table_entry *getTableEntry(bool bAlt, bool bAltGr, bool bCtrl, bool bShift, uint8_t scancode);

        static int trampoline(void* p);
        void thread();

        /**
        * True if we're in debug state.
        */
        bool m_bDebugState;

        /**
        * True if shift is held.
        */
        bool m_bShift;

        /**
        * True if ctrl is held.
        */
        bool m_bCtrl;

        /**
        * True if alt is held.
        */
        bool m_bAlt;

        bool m_bAltGr;

        /**
        * True if caps lock is on.
        */
        bool m_bCapsLock;

        /**
        * Circular input buffer.
        */
        uint64_t m_Buffer[BUFLEN];
        int m_BufStart, m_BufEnd;

        /**
        * Semaphore for how many items are in the buffer.
        */
        Semaphore m_BufLength;

        /** Callback. */
        KeyPressedCallback m_Callback;
};

#endif
