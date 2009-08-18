/*
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
#ifndef UHCI_H
#define UHCI_H

#include <machine/Device.h>
#include <machine/IrqHandler.h>
#include <process/Thread.h>
#include <processor/IoBase.h>
#include <processor/IoPort.h>
#include <processor/MemoryRegion.h>
#include <processor/PhysicalMemoryManager.h>
#include <processor/types.h>
#include <process/Semaphore.h>
#include <usb/UsbConstants.h>
#include <usb/UsbController.h>

/** Device driver for the UHCI class */
class UHCI : public UsbController, public Device, public IrqHandler
{
    public:
        UHCI(Device* pDev);
        ~UHCI();

        typedef struct TD
        {
            uint32_t link;
            uint32_t ctrl;
            uint32_t token;
            uint32_t buff;
            uint32_t reserved[4];
        } PACKED TD;

        virtual void getName(String &str)
        {
            str = "UHCI";
        }
        ssize_t setup(uint8_t addr, UsbSetup *setup);
        ssize_t in(uint8_t addr, uint8_t endpoint, void *buff, size_t size);
        ssize_t out(uint8_t addr, void *buff, size_t size);
        // IRQ handler callback.
        virtual bool irq(irq_id_t number, InterruptState &state);
        IoBase *m_pBase;

    private:

        volatile bool m_inTransaction;

        uint32_t *m_pTDList;
        uint8_t *m_pTDListVirt;
        uintptr_t m_pTDListPhys;
        uint32_t *m_pFrameList;
        uint8_t *m_pFrmListVirt;
        uintptr_t m_pFrmListPhys;
        MemoryRegion m_FrmListMR;

        UHCI(const UHCI&);
        void operator =(const UHCI&);
};

#endif
