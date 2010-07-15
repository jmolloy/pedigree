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
#include <scsi/ScsiController.h>

class UsbMassStorageDevice : public UsbDevice, public ScsiController
{
    public:
        UsbMassStorageDevice (UsbDevice *dev);
        virtual ~UsbMassStorageDevice();

        virtual bool sendCommand(size_t nUnit, uintptr_t pCommand, uint8_t nCommandSize, uintptr_t pRespBuffer, uint16_t nRespBytes, bool bWrite);

        virtual void getName(String &str)
        {
            str = "USB Mass Storage Device";
        }

    private:

        typedef struct Cbw {
            uint32_t sig;
            uint32_t tag;
            uint32_t data_len;
            uint8_t flags;
            uint8_t lun;
            uint8_t cmd_len;
            uint8_t cmd[16];
        } PACKED Cbw;

        typedef struct Csw {
            uint32_t sig;
            uint32_t tag;
            uint32_t residue;
            uint8_t status;
        } PACKED Csw;

        size_t m_nUnits;
        uint8_t m_nInEndpoint;
        uint8_t m_nOutEndpoint;

    protected:

        virtual size_t getNumUnits()
        {
            return m_nUnits;
        }
};

#endif
