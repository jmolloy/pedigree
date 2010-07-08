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
#ifndef USBULPI_H
#define USBULPI_H

#include <processor/types.h>
#include <processor/MemoryRegion.h>

class UsbUlpi
{
    public:
        UsbUlpi() : m_MemRegionUHH("USB-UHH_CONFIG"), m_MemRegionTLL("USB-TLL"), m_MemRegionPCtl("USB-PCTL")
        {
        }
        virtual ~UsbUlpi()
        {
        }

        static UsbUlpi &instance()
        {
            return m_Instance;
        }

        void initialise();

    private:

        static UsbUlpi m_Instance;

        enum UlpiRegisters
        {
            VendorIdLo          = 0,
            VendorIdHi          = 1,
            ProductIdLo         = 2,
            ProductIdHi         = 3,
            FunctionControl     = 4,
            InterfaceControl    = 7,
            OtgControl          = 0xA,
            UsbIntEnableRise    = 0xD,
            UsbIntEnableFall    = 0x10,
            UsbIntStatus        = 0x13,
            UsbIntLatch         = 0x14,
            Debug               = 0x15,
            Scratch             = 0x16,
            ExtendedSetAccess   = 0x2F,
            UtmiVControlEnable  = 0x30,
            UtmiVControlStatus  = 0x33,
            UtmiVControlLatch   = 0x34,
            UtmiVStatus         = 0x35,
            UtmiIntLatchNoClr   = 0x38,
            VendorIntEnable     = 0x3B,
            VendorIntStatus     = 0x3E,
            VendorIntLatch      = 0x3F
        };

        enum PowerManagement
        {
            VUSB1V5_DEV_GRP     = 0xCC,
            VUSB1V5_TYPE        = 0xCD,
            VUSB1V5_REMAP       = 0xCE,

            VUSB1V8_DEV_GRP     = 0xCF,
            VUSB1V8_TYPE        = 0xD0,
            VUSB1V8_REMAP       = 0xD1,

            VUSB3V1_DEV_GRP     = 0xD2,
            VUSB3V1_TYPE        = 0xD3,
            VUSB3V1_REMAP       = 0xD4,

            VUSBCP_DEV_GRP      = 0xD5,
            VUSBCP_TYPE         = 0xD6,
            VUSBCP_REMAP        = 0xD7,

            VUSB_DEDICATED1     = 0xD8,
            VUSB_DEDICATED2     = 0xD9,
        };

        MemoryRegion m_MemRegionUHH;
        MemoryRegion m_MemRegionTLL;
        MemoryRegion m_MemRegionPCtl;
};

#endif
