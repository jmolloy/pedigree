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

#include <hid/HidReport.h>
#include <processor/types.h>
#include <usb/UsbDevice.h>

class UsbHumanInterfaceDevice : public UsbDevice
{
    public:
        UsbHumanInterfaceDevice(UsbDevice *dev);
        virtual ~UsbHumanInterfaceDevice();

        virtual void initialiseDriver();

        virtual void getName(String &str)
        {
            str = "USB Human Interface Device";
        }

    private:

        struct HidDescriptor
        {
            inline HidDescriptor(UnknownDescriptor *pDes)
            {
                Descriptor *pDescriptor = static_cast<Descriptor*>(pDes->pDescriptor);
                nDescriptorLength = pDescriptor->nDescriptorLength;
                delete pDescriptor;
            }

            struct Descriptor
            {
                uint8_t nLength;
                uint8_t nType;
                uint16_t nBcdHidRelease;
                uint8_t nCountryCode;
                uint8_t nDescriptors;
                uint8_t nDescriptorType;
                uint16_t nDescriptorLength;
            } PACKED;

            uint16_t nDescriptorLength;
        };

        static void callback(uintptr_t pParam, ssize_t ret);
        void inputHandler();

        /// The endpoint used to receive input reports from the device
        Endpoint *m_pInEndpoint;

        /// The report instance used for input parsing
        HidReport *m_pReport;

        /// Input report buffer
        uint8_t *m_pInReportBuffer;
        /// Old input report buffer
        uint8_t *m_pOldInReportBuffer;
};

#endif
