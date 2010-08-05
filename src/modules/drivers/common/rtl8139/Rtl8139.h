/*
 * Copyright (c) 2009 Eduard Burtescu
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
#ifndef RTL8139_H
#define RTL8139_H

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

#define RTL8139_VENDOR_ID 0x10ec
#define RTL8139_DEVICE_ID 0x8139

/** Device driver for the RTL8139 class of network device */
class Rtl8139 : public Network, public IrqHandler
{
    public:
        Rtl8139(Network* pDev);
        ~Rtl8139();

        virtual void getName(String &str)
        {
            str = "rtl8139";
        }

        virtual bool send(size_t nBytes, uintptr_t buffer);

        virtual bool setStationInfo(StationInfo info);

        virtual StationInfo getStationInfo();

        virtual bool isConnected();

        // IRQ handler callback.
        virtual bool irq(irq_id_t number, InterruptState &state);

        IoBase *m_pBase;

    private:

        void recv();

        void reset();

        struct packet
        {
            uintptr_t ptr;
            size_t len;
        };

        StationInfo m_StationInfo;

        uint32_t m_RxCurr;
        uint8_t m_TxCurr;

        volatile bool m_RxLock;
        Spinlock m_TxLock;

        uint8_t *m_pRxBuffVirt;
        uint8_t *m_pTxBuffVirt;

        uintptr_t m_pRxBuffPhys;
        uintptr_t m_pTxBuffPhys;

        MemoryRegion m_RxBuffMR;
        MemoryRegion m_TxBuffMR;

        Rtl8139(const Rtl8139&);
        void operator =(const Rtl8139&);
};

#endif
