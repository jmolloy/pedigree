/*
 * Copyright (c) 2008 James Molloy, Jörg Pfähler, Matthew Iselin
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
#ifndef NIC_3C90X_H
#define NIC_3C90X_H

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

/** Device driver for the Nic3C90x class of network device */
class Nic3C90x : public Network, public IrqHandler
{
    public:
        Nic3C90x(Network* pDev);
        ~Nic3C90x();

        virtual void getName(String &str)
        {
            str = "3C90x";
        }

        virtual bool send(size_t nBytes, uintptr_t buffer);

        virtual bool setStationInfo(StationInfo info);

        virtual StationInfo getStationInfo();

        // IRQ handler callback.
        virtual bool irq(irq_id_t number, InterruptState &state);

        IoBase *m_pBase;

    private:

        int issueCommand(int cmd, int param);

        int setWindow(int window);

        uint16_t readEeprom(int address);

        int writeEepromWord(int address, uint16_t value);
        int writeEeprom(int address, uint16_t value);

        static int trampoline(void* p);

        void receiveThread();

        void reset();

        /** Local NIC information */
        uint8_t m_isBrev;
        uint8_t m_CurrentWindow;

        uint8_t *m_pRxBuffVirt;
        uint8_t *m_pTxBuffVirt;
        uintptr_t m_pRxBuffPhys;
        uintptr_t m_pTxBuffPhys;
        MemoryRegion m_RxBuffMR;
        MemoryRegion m_TxBuffMR;

        uintptr_t m_pDPD;
        MemoryRegion m_DPDMR;

        uintptr_t m_pUPD;
        MemoryRegion m_UPDMR;

        /** TX Descriptor */
        struct TXD
        {
          uint32_t DnNextPtr;
          uint32_t FrameStartHeader;
          // uint32_t HdrAddr;
          // uint32_t HdrLength;
          uint32_t DataAddr;
          uint32_t DataLength;
        } __attribute__((aligned(8)));

        /** RX Descriptor */
        struct RXD
        {
          uint32_t UpNextPtr;
          uint32_t UpPktStatus;
          uint32_t DataAddr;
          uint32_t DataLength;
        } __attribute__((aligned(8)));

        TXD *m_TransmitDPD;
        RXD *m_ReceiveUPD;

        Nic3C90x(const Nic3C90x&);
        void operator =(const Nic3C90x&);

        Semaphore m_RxMutex;
        Semaphore m_TxMutex;

        List<void*> m_PendingPackets;
};

#endif
