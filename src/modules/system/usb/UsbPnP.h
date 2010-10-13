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
#ifndef USBPNP_H
#define USBPNP_H

#include <usb/UsbDevice.h>

enum UsbPnPConstants
{
    VendorIdNone    = 0xFFFF,
    ProductIdNone   = 0xFFFF,
    ClassNone       = 0xFF,
    SubclassNone    = 0xFF,
    ProtocolNone    = 0xFF,
};

class UsbPnP
{
    private:
        /// Callback function type
        typedef UsbDevice *(*callback_t)(UsbDevice*);

    public:

        UsbPnP() : m_Callbacks() {}
        inline virtual ~UsbPnP() {}

        /// Singleton design
        static UsbPnP& instance()
        {
            return m_Instance;
        }

        /// Register a callback for the given vendor and product IDs
        void registerCallback(uint16_t nVendorId, uint16_t nProductId, callback_t callback);

        /// Register a callback for the given class, subclass and protocol numbers
        void registerCallback(uint8_t nClass, uint8_t nSubclass, uint8_t nProtocol, callback_t callback);

        /// Tries to find a suitable driver for the given USB device
        bool probeDevice(UsbDevice *pDevice);

    private:
        /// Static instance
        static UsbPnP m_Instance;

        /// Goes down the device tree, reprobing every USB device
        void reprobeDevices(Device *pParent);

        /// Item in the callback list. This stores information that's needed
        /// to choose a specific callback for a device.
        struct CallbackItem
        {
            /// The callback function
            callback_t callback;

            /// Vendor and product IDs
            uint16_t nVendorId;
            uint16_t nProductId;

            /// Class, subclass and protocol numbers
            uint8_t nClass;
            uint8_t nSubclass;
            uint8_t nProtocol;
        };

        /// Callback list
        List<CallbackItem*> m_Callbacks;
};

#endif
