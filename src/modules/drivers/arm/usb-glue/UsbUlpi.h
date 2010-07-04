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
        UsbUlpi() : m_MemRegionUHH("USB-UHH_CONFIG"), m_MemRegionPCtl("USB-PCTL")
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

        MemoryRegion m_MemRegionUHH;
        MemoryRegion m_MemRegionPCtl;
};

#endif
