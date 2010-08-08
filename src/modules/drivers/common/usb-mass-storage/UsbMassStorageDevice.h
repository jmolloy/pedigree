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

#ifndef USBMASSSTORAGEDEVICE_H
#define USBMASSSTORAGEDEVICE_H

#include <processor/types.h>
#include <usb/UsbDevice.h>
#include <usb/UsbConstants.h>
#include <scsi/ScsiController.h>

class UsbMassStorageDevice : public UsbDevice, public ScsiController
{
    public:
        UsbMassStorageDevice(UsbDevice *dev);
        virtual ~UsbMassStorageDevice();

        virtual void initialiseDriver();

        virtual bool sendCommand(size_t nUnit, uintptr_t pCommand, uint8_t nCommandSize, uintptr_t pRespBuffer, uint16_t nRespBytes, bool bWrite);

        virtual void getName(String &str)
        {
            str = "USB Mass Storage Device";
        }

    private:

        bool massStorageReset();

        enum MassStorageRequests
        {
            MassStorageRequest  = UsbRequestType::Class | UsbRequestRecipient::Interface,

            MassStorageReset    = 0xFF,
            MassStorageGetMaxLUN= 0xFE
        };

        enum MassStorageSigs
        {
            CbwSig = HOST_TO_LITTLE32(0x43425355),  // USBC
            CswSig = HOST_TO_LITTLE32(0x53425355)   // USBS
        };

        struct Cbw {
            uint32_t nSig;
            uint32_t nTag;
            uint32_t nDataBytes;
            uint8_t nFlags;
            uint8_t nLUN;
            uint8_t nCommandSize;
            uint8_t pCommand[16];
        } PACKED;

        struct Csw {
            uint32_t nSig;
            uint32_t nTag;
            uint32_t nResidue;
            uint8_t nStatus;
        } PACKED;

        size_t m_nUnits;
        Endpoint *m_pInEndpoint;
        Endpoint *m_pOutEndpoint;

    protected:

        virtual size_t getNumUnits()
        {
            return m_nUnits;
        }
};

#endif
