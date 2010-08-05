/*
 * Copyright (c) 2010 Matthew Iselin
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
#ifndef DM9601_H
#define DM9601_H

#include <processor/types.h>
#include <machine/Device.h>
#include <machine/Network.h>
#include <processor/IoBase.h>
#include <processor/IoPort.h>
#include <processor/MemoryRegion.h>
#include <processor/PhysicalMemoryManager.h>
#include <machine/IrqHandler.h>
#include <process/Thread.h>
#include <process/Semaphore.h>
#include <utilities/List.h>
#include <usb/UsbDevice.h>

class Dm9601 : public UsbDevice, public Network
{
    public:

        Dm9601(UsbDevice *pDev) {}

        virtual ~Dm9601() {}

        virtual void getName(String &str)
        {
            str = "DM9601";
        }

        virtual bool send(size_t nBytes, uintptr_t buffer);

        virtual bool setStationInfo(StationInfo info);

        virtual StationInfo getStationInfo();

    private:

        enum VendorRequests
        {
            ReadRegister    = 0,
            WriteRegister   = 1,
            ReadMemory      = 2,
            WriteRegister1  = 3,
            WriteMemory     = 5,
            WriteMemory1    = 7,
        };

        /// Reads data from a register into a buffer
        ssize_t readRegister(uint8_t reg, uintptr_t buffer, size_t nBytes);

        /// Writes data from a buffer to a register
        ssize_t writeRegister(uint8_t reg, uintptr_t buffer, size_t nBytes);

        /// Writes a single 8-bit value to a register
        ssize_t writeRegister(uint8_t reg, uint8_t data);

        /// Reads data from device memory into a buffer
        ssize_t readMemory(uint16_t offset, uintptr_t buffer, size_t nBytes);

        /// Writes data from a buffer into device memory
        ssize_t writeMemory(uint16_t offset, uintptr_t buffer, size_t nBytes);

        /// Writes a single 8-bit value into device memory
        ssize_t writeMemory(uint16_t offset, uint8_t data);

        Dm9601(const Dm9601&);
        void operator =(const Dm9601&);
};

#endif

