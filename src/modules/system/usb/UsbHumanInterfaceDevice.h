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

#ifndef USBHUMANINTERFACEDEVICE_H
#define USBHUMANINTERFACEDEVICE_H

//#include <machine/Keyboard.h>
#include <processor/types.h>
//#include <process/Semaphore.h>
//#include <usb/UsbConstants.h>
#include <usb/UsbDevice.h>

//#define BUFLEN 256

/**
* Keyboard device abstraction.
*/
class UsbHumanInterfaceDevice : /*public Keyboard, */public UsbDevice
{
    public:
        UsbHumanInterfaceDevice (UsbDevice *dev);
        virtual ~UsbHumanInterfaceDevice();

        //void initialise() {};

        virtual void getName(String &str)
        {
            str = "USB Human Interface Device";
        }

        /*void setDebugState(bool enableDebugState);
        bool getDebugState();

        char getChar()
        {return 0;};
        char getCharNonBlock()
        {return 0;};

        uint64_t getCharacter();
        uint64_t getCharacterNonBlock();

        void registerCallback(KeyPressedCallback callback)
        {
            m_Callback = callback;
        }*/

    private:

        typedef struct HidDescriptor
        {
            inline HidDescriptor(UnknownDescriptor *pDes) :  pDescriptor(static_cast<struct Descriptor*>(pDes->pDescriptor)) {}

            struct Descriptor
            {
                uint8_t nLength;
                uint8_t nType;
                uint16_t nBcdHidRelease;
                uint8_t nCountryCode;
                uint8_t nDescriptors;
                uint8_t nDescriptorType;
                uint16_t nDescriptorLength;
            } PACKED *pDescriptor;
        } HidDescriptor;

        typedef struct HidReportBlock
        {
            enum
            {
                Ignore,
                Absolute,
                Relative,
                Array
            } type;
            size_t nCount;
            size_t nSize;
            uint32_t nUsagePage;
            uint32_t nUsageBase;
            uint8_t nLogSize;
            Vector<size_t> *pUsages;
        } HidReportBlock;

        typedef struct HidReport
        {
            size_t nBytes;
            List<HidReportBlock *> pBlockList;
        } HidReport;

        /**
        * Converts a scancode into a "real" character in UTF-32 format, plus
        * modifier flags in the top 32-bits.
        */
        /*uint64_t scancodeToCharacter(uint8_t scancode);

        struct table_entry *getTableEntry(uint8_t combinator, bool bAlt, bool bAltGr, bool bCtrl, bool bShift, uint8_t scancode);*/

        static void callback(uintptr_t pParam, ssize_t ret);
        void inputHandler();

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
        * Index of the current active combinator, if any
        */
        uint8_t m_Combinator;

        /**
        * Circular input buffer.
        */
        //uint64_t m_Buffer[BUFLEN];
        //int m_BufStart, m_BufEnd;

        HidReport *m_pReport;
        uint8_t *m_pReportBuffer;
        uint8_t *m_pOldReportBuffer;

        /**
        * Semaphore for how many items are in the buffer.
        */
        //Semaphore m_BufLength;

        /** Callback. */
        //KeyPressedCallback m_Callback;
};

#endif
