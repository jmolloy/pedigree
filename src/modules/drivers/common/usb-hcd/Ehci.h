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
#ifndef EHCI_H
#define EHCI_H

#include <machine/IrqHandler.h>
#include <processor/InterruptManager.h>
#include <processor/MemoryRegion.h>
#include <processor/PhysicalMemoryManager.h>
#include <processor/types.h>
#include <usb/UsbHub.h>
#include <utilities/ExtensibleBitmap.h>
#include <utilities/MemoryAllocator.h>

/** Device driver for the Ehci class */
class Ehci : public UsbHub,
#ifdef X86_COMMON
    public IrqHandler,
#else
    public InterruptHandler,
#endif
    public RequestQueue
{
    public:
        Ehci(Device* pDev);
        virtual ~Ehci();

        struct qTD
        {
            uint32_t bNextInvalid : 1;
            uint32_t res0 : 4;
            uint32_t pNext : 27;
            uint32_t bAltNextInvalid : 1;
            uint32_t res1 : 4;
            uint32_t pAltNext : 27;
            uint32_t nStatus : 8;
            uint32_t nPid : 2;
            uint32_t nErr : 2;
            uint32_t nPage : 3;
            uint32_t bIoc : 1;
            uint32_t nBytes : 15;
            uint32_t bDataToggle : 1;
            uint32_t nOffset : 12;
            uint32_t pPage0 : 20;
            uint32_t res2 : 12;
            uint32_t pPage1 : 20;
            uint32_t res3 : 12;
            uint32_t pPage2 : 20;
            uint32_t res4 : 12;
            uint32_t pPage3 : 20;
            uint32_t res5 : 12;
            uint32_t pPage4 : 20;

            // 64-bit qTD fields
            uint32_t extend0;
            uint32_t extend1;
            uint32_t extend2;
            uint32_t extend3;
            uint32_t extend4;

            // Custom qTD fields
            uint16_t nBufferSize;

            // Possible values for status
            enum StatusCodes
            {
                Halted = 0x40,
            };

            UsbError getError()
            {
                if(nStatus & Halted)
                    return Stall;
                return TransactionError;
            }
        } PACKED ALIGN(32);

        struct QH
        {
            uint32_t bNextInvalid : 1;
            uint32_t nNextType : 2;
            uint32_t res0 : 2;
            uint32_t pNext : 27;
            uint32_t nAddress : 7;
            uint32_t bInactiveNext : 1;
            uint32_t nEndpoint : 4;
            uint32_t nSpeed : 2;
            uint32_t bDataToggleSrc : 1;
            uint32_t hrcl : 1;
            uint32_t nMaxPacketSize : 11;
            uint32_t bControlEndpoint : 1;
            uint32_t nNakReload : 4;
            uint32_t ism : 8;
            uint32_t scm : 8;
            uint32_t nHubAddress : 7;
            uint32_t nHubPort : 7;
            uint32_t mult : 2;
            uint32_t res1 : 5;
            uint32_t pQTD : 27;

            qTD overlay;

            struct MetaData
            {
                void (*pCallback)(uintptr_t, ssize_t);
                uintptr_t pParam;

                bool bPeriodic;
                qTD *pFirstQTD;
                qTD *pLastQTD;
                size_t nTotalBytes;

                QH *pPrev;
                QH *pNext;

                bool bIgnore; /// Ignore this QH when iterating over the list - don't look at any of its qTDs
            } *pMetaData;
        } PACKED ALIGN(32);

        virtual void getName(String &str)
        {
            str = "EHCI";
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

        virtual bool portReset(uint8_t nPort, bool bErrorResponse = false);

    protected:

        virtual uint64_t executeRequest(uint64_t p1 = 0, uint64_t p2 = 0, uint64_t p3 = 0, uint64_t p4 = 0, uint64_t p5 = 0,
                                        uint64_t p6 = 0, uint64_t p7 = 0, uint64_t p8 = 0);

    private:

        enum EhciConstants {
            EHCI_CAPLENGTH = 0x00,      // Capability Registers Length
            EHCI_HCIVERSION = 0x02,     // Host Controller Interface Version
            EHCI_HCSPARAMS = 0x04,      // Host Controller Structural Parameters
            EHCI_HCCPARAMS = 0x08,      // Host Controller Structural Parameters

            EHCI_CMD = 0x00,            // Command register
            EHCI_STS = 0x04,            // Status register
            EHCI_INTR = 0x08,           // Intrerrupt Enable register
            EHCI_CTRLDSEG = 0x10,       // Control Data Structure Segment Register
            EHCI_FRINDEX = 0x0c,        // Periodic Frame Index register
            EHCI_PERIODICLP = 0x14,     // Periodic List Pointer register
            EHCI_ASYNCLP = 0x18,        // Async List Pointer register
            EHCI_CFGFLAG = 0x40,        // Config Flag register
            EHCI_PORTSC = 0x44,         // Port Status/Control registers

            EHCI_CMD_ASYNCLE = 0x20,    // Async List Enable bit
            EHCI_CMD_PERIODICLE = 0x10, // Periodic List Enable bit
            EHCI_CMD_HCRES = 0x02,      // Host Controller Reset bit
            EHCI_CMD_RUN = 0x01,        // Run bit

            EHCI_STS_HALTED = 0x1000,   // Host Controller Halted bit
            EHCI_STS_ASYNCADVANCE = 0x20, // Async Advance
            EHCI_STS_PORTCH = 0x4,      // Port Change Detect bit
            EHCI_STS_ERR = 0x2,         // Error bit
            EHCI_STS_INT = 0x1,         // On Completition Interrupt bit

            EHCI_PORTSC_PPOW = 0x1000,  // Port Power bit
            EHCI_PORTSC_PRES = 0x100,   // Port Reset bit
            EHCI_PORTSC_ENCH = 0x8,     // Port Enable/Disable Change bit
            EHCI_PORTSC_EN = 0x4,       // Port Enabled bit
            EHCI_PORTSC_CSCH = 0x2,     // Port Connect Status Change bit
            EHCI_PORTSC_CONN = 0x1,     // Port Connected bit
        };

        IoBase *m_pBase;

        uint8_t m_nOpRegsOffset;
        uint8_t m_nPorts;

        Mutex m_Mutex;

        Spinlock m_QueueListChangeLock;

        QH *m_pQHList;
        uintptr_t m_pQHListPhys;
        ExtensibleBitmap m_QHBitmap;

        uint32_t *m_pFrameList;
        uintptr_t m_pFrameListPhys;
        ExtensibleBitmap m_FrameBitmap;

        qTD *m_pqTDList;
        uintptr_t m_pqTDListPhys;
        ExtensibleBitmap m_qTDBitmap;

        // Pointer to the current queue tail, which allows insertion of new queue
        // heads to the asynchronous schedule.
        QH *m_pCurrentQueueTail;

        // Pointer to the current queue head. Used to fill pNext automatically
        // for new queue heads inserted to the asynchronous schedule.
        QH *m_pCurrentQueueHead;

        MemoryRegion m_EhciMR;

        Ehci(const Ehci&);
        void operator =(const Ehci&);
};

#endif
