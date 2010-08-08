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

#ifndef FTDISERIALDEVICE_H
#define FTDISERIALDEVICE_H

#include <processor/types.h>
#include <usb/UsbDevice.h>
#include <machine/Serial.h>

class FtdiSerialDevice : public UsbDevice, public Serial
{
    public:
        FtdiSerialDevice(UsbDevice *dev);
        virtual ~FtdiSerialDevice();

        virtual void initialiseDriver();

        virtual void setBase(uintptr_t nBaseAddr){}
        virtual char read();
        virtual char readNonBlock(){return 0;}
        virtual void write(char c);

        virtual void getName(String &str)
        {
            str = "USB FTDI Serial Device";
        }

    private:

        Endpoint *m_pInEndpoint;
        Endpoint *m_pOutEndpoint;
};

#endif
