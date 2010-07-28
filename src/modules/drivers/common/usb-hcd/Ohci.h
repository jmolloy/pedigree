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
#ifndef OHCI_H
#define OHCI_H

#include <machine/Device.h>
#include <machine/IrqHandler.h>
#include <processor/IoBase.h>
#include <processor/MemoryRegion.h>
#include <processor/InterruptManager.h>
#include <processor/PhysicalMemoryManager.h>
#include <processor/types.h>
#include <usb/UsbHub.h>

/** Device driver for the Ohci class */
class Ohci : public UsbHub,
#ifdef X86_COMMON
    public IrqHandler
#else
    public InterruptHandler
#endif
{
    public:
        Ohci(Device* pDev);
        ~Ohci();

        typedef struct TD
        {
            uint32_t res0 : 18;
            uint32_t bBuffRounding : 1;
            uint32_t nPid : 2;
            uint32_t nIntDelay : 3;
            uint32_t bDataToggleSrc : 1;
            uint32_t bDataToggle : 1;
            uint32_t nErrorCount : 2;
            uint32_t nStatus : 4;
            uint32_t pBufferStart;
            uint32_t res1 : 4;
            uint32_t pNext : 28;
            uint32_t pBufferEnd;

            // Custom qTD fields
            uint16_t nBufferSize;
        } PACKED __attribute__((aligned(16))) TD;

        typedef struct ED
        {
            uint32_t nAddress : 7;
            uint32_t nEndpoint : 4;
            uint32_t bOut : 1;
            uint32_t bIn : 1;
            uint32_t bLoSpeed : 1;
            uint32_t bSkip : 1;
            uint32_t bIso : 1;
            uint32_t nMaxPacketSize : 11;
            uint32_t res0 : 9;
            uint32_t pTailTD : 28;
            uint32_t bHalted : 1;
            uint32_t bToggleCarry : 1;
            uint32_t res1 : 2;
            uint32_t pHeadTD : 28;
            uint32_t res2 : 4;
            uint32_t pNext : 28;

            struct MetaData
            {
                void (*pCallback)(uintptr_t, ssize_t);
                uintptr_t pParam;

                UsbEndpoint &endpointInfo;

                bool bPeriodic;
                TD *pFirstQTD;
                TD *pLastQTD;
                size_t nTotalBytes;

                ED *pPrev;
                ED *pNext;

                bool bIgnore; /// Ignore this QH when iterating over the list - don't look at any of its qTDs
            } *pMetaData;
        } PACKED __attribute__((aligned(16))) ED;

        typedef struct Hcca
        {
            uint32_t pInterruptEDList[32];
            uint16_t nFrameNumber;
            uint16_t res0;
            uint32_t pDoneHead;
        } PACKED Hcca;

        virtual void getName(String &str)
        {
            str = "OHCI";
        }

        virtual void addTransferToTransaction(uintptr_t pTransaction, bool bToggle, UsbPid pid, uintptr_t pBuffer, size_t nBytes);
        virtual uintptr_t createTransaction(UsbEndpoint endpointInfo);
        virtual void doAsync(uintptr_t pTransaction, void (*pCallback)(uintptr_t, ssize_t)=0, uintptr_t pParam=0);
        virtual void addInterruptInHandler(UsbEndpoint endpointInfo, uintptr_t pBuffer, uint16_t nBytes, void (*pCallback)(uintptr_t, ssize_t), uintptr_t pParam=0);

        //virtual void doAsync(UsbEndpoint endpointInfo, uint8_t nPid, uintptr_t pBuffer, uint16_t nBytes, void (*pCallback)(uintptr_t, ssize_t)=0, uintptr_t pParam=0);
        //virtual void addInterruptInHandler(uint8_t nAddress, uint8_t nEndpoint, uintptr_t pBuffer, uint16_t nBytes, void (*pCallback)(uintptr_t, ssize_t), uintptr_t pParam=0);

        // IRQ handler callback.
#ifdef X86_COMMON
        virtual bool irq(irq_id_t number, InterruptState &state);
#else
        virtual void interrupt(size_t number, InterruptState &state);
#endif

    private:

        enum OhciConstants {
            OhciControl = 0x04,             // HcControl register
            OhciCommandStatus = 0x08,       // HcCommandStatus register
            OhciInterruptStatus = 0x0c,     // HcInterruptStatus register
            OhciInterruptEnable = 0x10,     // HcIntrerruptEnable register
            OhciInterruptDisable = 0x14,    // HcIntrerruptDisable register
            OhciHcca = 0x18,                // HcHCCA register
            OhciControlHeadED = 0x20,       // HcControlHeadED register
            OhciBulkHeadED = 0x28,          // HcBulkHeadED register
            //OHCI_INTR = 0x04,             // Intrerrupt Enable register
            //OHCI_FRMN = 0x06,             // Frame Number register
            //OHCI_FRLP = 0x08,             // Frame List Pointer register
            OhciRhDescriptorA = 0x48,       // HcRhDescriptorA register
            OhciRhPortStatus = 0x54,        // HcRhPortStatus registers

            OhciControlStateRunning = 0x80, // HostControllerFunctionalState bits for USBOPERATIONAL
            OhciControlListsEnable = 0x30,  // ControlListEnable and BulkListEnable bits

            OhciCommandBlkListFilled = 0x04,// BulkListFilled bit
            OhciCommandCtlListFilled = 0x02,// ControlListFilled bit
            OhciCommandHcReset = 0x01,      // HostControllerReset bit

            OhciInterruptMIE = 0x80000000,  // MasterInterruptEnable bit
            OhciInterruptRhStsChange = 0x40,// RootHubStatusChange bit
            OhciInterruptWbDoneHead = 0x02, // WritebackDoneHead bit

            OhciRhPortStsResCh = 0x100000,  // PortResetStatusChange bit
            OhciRhPortStsReset = 0x10,      // SetPortReset bit
            //OHCI_PORTSC_EDCH = 0x08,      // Port Enable/Disable Change bit
            OhciRhPortStsEnable = 0x02,     // SetPortEnable bit
            //OHCI_PORTSC_CSCH = 0x02,      // Port Connect Status Change bit
            OhciRhPortStsConnected = 0x01,  // CurrentConnectStatus bit
        };

        IoBase *m_pBase;

        uint8_t m_nPorts;

        Mutex m_Mutex;

        Spinlock m_QueueListChangeLock;

        Hcca *m_pHcca;
        uintptr_t m_pHccaPhys;

        ED *m_pEDList;
        uintptr_t m_pEDListPhys;
        ExtensibleBitmap m_EDBitmap;

        TD *m_pTDList;
        uintptr_t m_pTDListPhys;
        ExtensibleBitmap m_TDBitmap;

        // Pointer to the current queue tail, which allows insertion of new queue
        // heads to the asynchronous schedule.
        ED *m_pCurrentBulkQueueTail;
        ED *m_pCurrentControlQueueTail;

        // Pointer to the current queue head. Used to fill pNext automatically
        // for new queue heads inserted to the asynchronous schedule.
        ED *m_pCurrentBulkQueueHead;
        ED *m_pCurrentControlQueueHead;

        MemoryRegion m_OhciMR;

        Ohci(const Ohci&);
        void operator =(const Ohci&);
};

#endif
