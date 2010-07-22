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

#ifndef USBHUBDEVICE_H
#define USBHUBDEVICE_H

#include <usb/UsbDevice.h>
#include <usb/UsbHub.h>

class UsbHubDevice : public UsbDevice, public UsbHub
{
    public:
        UsbHubDevice (UsbDevice *dev);
        virtual ~UsbHubDevice();

        virtual void getName(String &str)
        {
            str = "USB Hub Device";
        }

        virtual void addTransferToTransaction(uintptr_t pTransaction, bool bToggle, UsbPid pid, uintptr_t pBuffer, size_t nBytes);
        virtual uintptr_t createTransaction(UsbEndpoint endpointInfo);
        virtual void doAsync(uintptr_t pTransaction, void (*pCallback)(uintptr_t, ssize_t)=0, uintptr_t pParam=0);
        virtual void addInterruptInHandler(UsbEndpoint endpointInfo, uintptr_t pBuffer, uint16_t nBytes, void (*pCallback)(uintptr_t, ssize_t), uintptr_t pParam=0);

    private:

        typedef struct HubDescriptor
        {
            inline HubDescriptor(void *pBuffer) : pDescriptor(static_cast<struct Descriptor*>(pBuffer)),
                nPorts(pDescriptor->nPorts), hubCharacteristics(pDescriptor->hubCharacteristics) {}

            struct Descriptor
            {
                uint8_t nLength;
                uint8_t nType;
                uint8_t nPorts;
                uint16_t hubCharacteristics;
            } PACKED *pDescriptor;

            uint8_t nPorts;
            uint16_t hubCharacteristics;
        } HubDescriptor;

        size_t m_nPorts;
};

#endif
