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
    public IrqHandler,
#else
    public InterruptHandler,
#endif
    public RequestQueue
{
    public:
        Ohci(Device* pDev);
        ~Ohci();

        struct TD
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

            // Custom TD fields
            uint16_t nBufferSize;
            uint16_t nNextTDIndex;
            bool bLast;

            // Possible values for status
            enum StatusCodes
            {
                CrcError = 1,
                Stall = 4
            };

            UsbError getError()
            {
                switch(nStatus)
                {
                    case CrcError:
                        return ::CrcError;
                    case Stall:
                        return ::Stall;
                    default:
                        return TransactionError;
                }
            }

        } PACKED ALIGN(16);

        struct ED
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
                TD *pFirstTD;
                TD *pLastTD;
                size_t nTotalBytes;

                ED *pPrev;
                ED *pNext;

                bool bLinked;
            } *pMetaData;
        } PACKED ALIGN(16);

        struct Hcca
        {
            uint32_t pInterruptEDList[32];
            uint16_t nFrameNumber;
            uint16_t res0;
            uint32_t pDoneHead;
        } PACKED;

        virtual void getName(String &str)
        {
            str = "OHCI";
        }

        virtual void addTransferToTransaction(uintptr_t pTransaction, bool bToggle, UsbPid pid, uintptr_t pBuffer, size_t nBytes);
        virtual uintptr_t createTransaction(UsbEndpoint endpointInfo);

        virtual void doAsync(uintptr_t pTransaction, void (*pCallback)(uintptr_t, ssize_t)=0, uintptr_t pParam=0);
        virtual void addInterruptInHandler(UsbEndpoint endpointInfo, uintptr_t pBuffer, uint16_t nBytes, void (*pCallback)(uintptr_t, ssize_t), uintptr_t pParam=0);

        /// IRQ handler
#ifdef X86_COMMON
        virtual bool irq(irq_id_t number, InterruptState &state);
#else
        virtual void interrupt(size_t number, InterruptState &state);
#endif

        void doDequeue();

    protected:

        virtual uint64_t executeRequest(uint64_t p1 = 0, uint64_t p2 = 0, uint64_t p3 = 0, uint64_t p4 = 0, uint64_t p5 = 0,
                                        uint64_t p6 = 0, uint64_t p7 = 0, uint64_t p8 = 0);

    private:

        enum OhciConstants {
            OhciControl         = 0x04,     // HcControl register
            OhciCommandStatus   = 0x08,     // HcCommandStatus register
            OhciInterruptStatus = 0x0c,     // HcInterruptStatus register
            OhciInterruptEnable = 0x10,     // HcIntrerruptEnable register
            OhciInterruptDisable= 0x14,     // HcIntrerruptDisable register
            OhciHcca            = 0x18,     // HcHCCA register
            OhciControlHeadED   = 0x20,     // HcControlHeadED register
            OhciControlCurrentED= 0x24,     // HcControlCurrentED register
            OhciBulkHeadED      = 0x28,     // HcBulkHeadED register
            OhciBulkCurrentED   = 0x2c,     // HcBulkCurrentED register
            OhciRhDescriptorA   = 0x48,     // HcRhDescriptorA register
            OhciRhPortStatus    = 0x54,     // HcRhPortStatus registers

            OhciControlStateRunning = 0x80,     // HostControllerFunctionalState bits for USBOPERATIONAL
            OhciControlListsEnable  = 0x34,     // PeriodicListEnable, ControlListEnable and BulkListEnable bits

            OhciCommandBulkListFilled   = 0x04, // BulkListFilled bit
            OhciCommandControlListFilled= 0x02, // ControlListFilled bit
            OhciCommandHcReset          = 0x01, // HostControllerReset bit

            OhciInterruptMIE        = 0x80000000,   // MasterInterruptEnable bit
            OhciInterruptRhStsChange= 0x40,         // RootHubStatusChange bit
            OhciInterruptWbDoneHead = 0x02,         // WritebackDoneHead bit

            OhciRhPortStsResCh      = 0x100000, // PortResetStatusChange bit
            OhciRhPortStsConnStsCh  = 0x10000,  // ConnectStatusChange bit
            OhciRhPortStsLoSpeed    = 0x200,    // LowSpeedDeviceAttached bit
            OhciRhPortStsPower      = 0x100,    // PortPowerStatus / SetPortPower bit
            OhciRhPortStsReset      = 0x10,     // SetPortReset bit
            OhciRhPortStsEnable     = 0x02,     // SetPortEnable bit
            OhciRhPortStsConnected  = 0x01,     // CurrentConnectStatus bit
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

        // Pointer to the current bulk and control queue heads (can be null)
        ED *m_pCurrentBulkQueueHead;
        ED *m_pCurrentControlQueueHead;

        // Pointer to the current periodic queue tail
        ED *m_pCurrentPeriodicQueueTail;

        MemoryRegion m_OhciMR;

        Ohci(const Ohci&);
        void operator =(const Ohci&);
};

#endif
