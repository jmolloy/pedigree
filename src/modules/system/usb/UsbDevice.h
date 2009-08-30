/*
*
* Copyright (c) 2008 James Molloy, Jörg Pfähler, Matthew Iselin, Eduard Burtescu
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
#ifndef USB_DEVICE_H
#define USB_DEVICE_H

#include <utilities/String.h>
#include <utilities/Vector.h>
#include <processor/types.h>
#include <usb/UsbConstants.h>

/**
* The Pedigree usb stack - Usb Device
*/
class UsbDevice
{
    public:
        /** Constructors and destructors */
        UsbDevice() : m_Address(0) {};
        UsbDevice(uint8_t address) : m_Address(address) {};
        UsbDevice(UsbDevice *p) : m_Address(p->m_Address) {};
        virtual ~UsbDevice() {};

        /** Access to internal information */
        uint8_t getAddress()
        {
            return m_Address;
        }
        void setAddress(uint8_t addr)
        {
            m_Address = addr;
        }

        ssize_t setup(UsbSetup *setup);
        ssize_t in(void *buff, size_t size);
        ssize_t in(uint8_t endpoint, void *buff, size_t size);
        ssize_t out(void *buff, size_t size);

        ssize_t control(uint8_t req_type, uint8_t req, uint16_t val, uint16_t index, uint16_t len);
        int16_t status();
        bool ping();
        bool assign(uint8_t addr);
        void *getDescriptor(uint8_t des, uint8_t subdes, uint16_t size);
        UsbInfo *getUsbInfo();
        char *getString(uint8_t str);

    private:
        /** Our Address */
        uint8_t m_Address;

        UsbDevice(const UsbDevice &e);
        const UsbDevice& operator = (const UsbDevice& e);
};

#endif
