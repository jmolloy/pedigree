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

#ifdef X86_COMMON

#ifndef UHCI_H
#define UHCI_H

#include <machine/Device.h>
#include <machine/IrqHandler.h>
#include <machine/Timer.h>
#include <processor/IoBase.h>
#include <processor/MemoryRegion.h>
#include <processor/PhysicalMemoryManager.h>
#include <processor/types.h>
#include <process/Semaphore.h>
#include <usb/UsbHub.h>

/** Device driver for the Uhci class */
class Uhci : public UsbHub, public IrqHandler, public RequestQueue, public TimerHandler
{
    public:
        Uhci(Device* pDev);
        virtual ~Uhci();

        struct TD
        {
            uint32_t bNextInvalid : 1;
            uint32_t bNextQH : 1;
            uint32_t bNextDepth : 1;
            uint32_t res0 : 1;
            uint32_t pNext : 28;
            uint32_t nActLen : 11;
            uint32_t res1 : 5;
            uint32_t nStatus : 8;
            uint32_t bIoc : 1;
            uint32_t bIsochronus : 1;
            uint32_t bLoSpeed : 1;
            uint32_t nErr : 2;
            uint32_t bSpd : 1;
            uint32_t res2 : 2;
            uint32_t nPid : 8;
            uint32_t nAddress : 7;
            uint32_t nEndpoint : 4;
            uint32_t bDataToggle : 1;
            uint32_t res3 : 1;
            uint32_t nMaxLen : 11;
            uint32_t pBuff;

            // Custom TD fields
            uint16_t nBufferSize;
            bool bShortTransferTD;
                
            size_t id;

            // Possible values for status
            enum StatusCodes
            {
                Timeout = 0x4,
                Nak = 0x8,
                Babble = 0x10,
                Stall = 0x40,
            };

            inline UsbError getError()
            {
                if(nStatus & Stall)
                    return ::Stall;
                else if(nStatus & Nak)
                    return NakNyet;
                else if(nStatus & Babble)
                    return ::Babble;
                else if(nStatus & Timeout)
                    return ::Timeout;
                else
                    return TransactionError;
            }
        } PACKED ALIGN(16);

        struct QH
        {
            uint32_t bNextInvalid : 1;
            uint32_t bNextQH : 1;
            uint32_t res0 : 2;
            uint32_t pNext : 28;
            uint32_t bElemInvalid : 1;
            uint32_t bElemQH : 1;
            uint32_t res1 : 2;
            uint32_t pElem : 28;

            struct MetaData
            {
                void (*pCallback)(uintptr_t, ssize_t);
                uintptr_t pParam;

                UsbEndpoint endpointInfo;

                bool bPeriodic;
                TD *pFirstTD;
                TD *pLastTD;
                size_t nTotalBytes;

                QH *pPrev;
                QH *pNext;
                
                List<TD*> tdList;
                List<TD*> completedTdList;

                bool bIgnore; /// Ignore this QH when iterating over the list - don't look at any of its TDs
                
                size_t id;
            } *pMetaData;
        } PACKED ALIGN(16);

        virtual void getName(String &str)
        {
            str = "UHCI";
        }

        virtual void addTransferToTransaction(uintptr_t pTransaction, bool bToggle, UsbPid pid, uintptr_t pBuffer, size_t nBytes);
        virtual uintptr_t createTransaction(UsbEndpoint endpointInfo);

        virtual void doAsync(uintptr_t pTransaction, void (*pCallback)(uintptr_t, ssize_t)=0, uintptr_t pParam=0);
        virtual void addInterruptInHandler(UsbEndpoint endpointInfo, uintptr_t pBuffer, uint16_t nBytes, void (*pCallback)(uintptr_t, ssize_t), uintptr_t pParam=0);

        /// IRQ handler
        virtual bool irq(irq_id_t number, InterruptState &state);

        void doDequeue();

        /// Timer callback to handle port status changes
        void timer(uint64_t delta, InterruptState &state);

        virtual bool portReset(uint8_t nPort, bool bErrorResponse = false);

    protected:

        virtual uint64_t executeRequest(uint64_t p1 = 0, uint64_t p2 = 0, uint64_t p3 = 0, uint64_t p4 = 0, uint64_t p5 = 0,
                                        uint64_t p6 = 0, uint64_t p7 = 0, uint64_t p8 = 0);

    private:
    
        /// Stops the UHCI controller
        void stop();
        
        /// Starts the UHCI controller
        void start();

        enum UhciConstants {
            UHCI_CMD = 0x00,            // Command register
            UHCI_STS = 0x02,            // Status register
            UHCI_INTR = 0x04,           // Intrerrupt Enable register
            UHCI_FRMN = 0x06,           // Frame Number register
            UHCI_FRLP = 0x08,           // Frame List Pointer register
            UHCI_PORTSC = 0x10,         // Port Status/Control registers

            UHCI_CMD_GRES = 0x04,       // Global Reset bit
            UHCI_CMD_HCRES = 0x02,      // Host Controller Reset bit
            UHCI_CMD_RUN = 0x01,        // Run bit

            UHCI_STS_HALT = 0x20,       // Controller is halted
            UHCI_STS_ERR = 0x02,        // UHCI Error
            UHCI_STS_INT = 0x01,        // On Completition Interrupt bit

            UHCI_PORTSC_PRES = 0x200,   // Port Reset bit
            UHCI_PORTSC_LOSPEED = 0x100,// Port has Low Speed Device attached bit
            UHCI_PORTSC_EDCH = 0x8,     // Port Enable/Disable Change bit
            UHCI_PORTSC_ENABLE = 0x4,   // Port Enable bit
            UHCI_PORTSC_CSCH = 0x2,     // Port Connect Status Change bit
            UHCI_PORTSC_CONN = 0x1,     // Port Connected bit
        };

        IoBase *m_pBase;

        uint8_t m_nPorts;

        Mutex m_Mutex;

        Spinlock m_AsyncQueueListChangeLock;

        uint32_t *m_pFrameList;
        uintptr_t m_pFrameListPhys;

        TD *m_pTDList;
        uintptr_t m_pTDListPhys;
        ExtensibleBitmap m_TDBitmap;

        QH *m_pAsyncQH;
        QH *m_pPeriodicQH;

        QH *m_pQHList;
        uintptr_t m_pQHListPhys;
        ExtensibleBitmap m_QHBitmap;

        MemoryRegion m_UhciMR;

        /// Pointer to the current queue tail, which allows insertion of new queue
        /// heads to the asynchronous schedule.
        QH *m_pCurrentAsyncQueueTail;

        /// Pointer to the current queue head. Used to fill pNext automatically
        /// for new queue heads inserted to the asynchronous schedule.
        QH *m_pCurrentAsyncQueueHead;
        
        /// List of QHs in the active asynchronous schedule
        List<QH*> m_AsyncSchedule;
        
        /// List of QHs ready for dequeue
        List<QH*> m_DequeueList;
        
        /// Semaphore for the dequeue list
        Semaphore m_DequeueCount;

        /// The time passed since last port check
        uint64_t m_nPortCheckTicks;

        Uhci(const Uhci&);
        void operator =(const Uhci&);
};

#endif

#endif
