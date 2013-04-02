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
    private:
    
        /// Enumeration of lists that can be stopped or started.
        enum Lists
        {
            PeriodicList    = 0x4,
            IsochronousList = 0x8,
            ControlList     = 0x10,
            BulkList        = 0x20
        };
    
    public:
        Ohci(Device* pDev);
        virtual ~Ohci();

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
            
            size_t id;

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

                UsbEndpoint endpointInfo;

                bool bPeriodic;
                TD *pFirstTD;
                TD *pLastTD;
                size_t nTotalBytes;

                ED *pPrev;
                ED *pNext;
                
                List<TD*> tdList;
                List<TD*> completedTdList;
                
                bool bIgnore;

                bool bLinked;
                
                Lists edType;
                
                size_t id;
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

        virtual bool portReset(uint8_t nPort, bool bErrorResponse = false);

    protected:

        virtual uint64_t executeRequest(uint64_t p1 = 0, uint64_t p2 = 0, uint64_t p3 = 0, uint64_t p4 = 0, uint64_t p5 = 0,
                                        uint64_t p6 = 0, uint64_t p7 = 0, uint64_t p8 = 0);

    private:
        /// Stops the controller from processing the given list.
        void stop(Lists list);
        
        /// Starts processing of the given list.
        void start(Lists list);
        
        /// Prepares an ED to be reclaimed.
        void removeED(ED *pED);
        
        /// Converts a software ED pointer to a physical address.
        inline physical_uintptr_t vtp_ed(ED *pED)
        {
            if(!pED || !pED->pMetaData)
                return 0;
            
            size_t id = pED->pMetaData->id & 0xFFF;
            Lists type = pED->pMetaData->edType;
            switch(type)
            {
                case ControlList:
                    return m_pControlEDListPhys + (id * sizeof(ED));
                case BulkList:
                    return m_pBulkEDListPhys + (id * sizeof(ED));
                default:
                    return 0;
            }
        }
        
        /// Converts a physical address to an ED pointer. Maybe.
        inline ED *ptv_ed(physical_uintptr_t phys)
        {
            if(!phys)
                return 0;
            
            // Figure out which list the ED was in.
            /// \todo defines for the list sizes so changing one doesn't involve rewriting heaps of code
            if((m_pControlEDListPhys <= phys) && (phys < (m_pControlEDListPhys + 0x1000)))
            {
                return &m_pControlEDList[phys & 0xFFF];
            }
            else if((m_pBulkEDListPhys <= phys) && (phys < (m_pBulkEDListPhys + 0x1000)))
            {
                return &m_pBulkEDList[phys & 0xFFF];
            }
            else
                return 0;
        }

        enum OhciConstants {
            OhciVersion         = 0x00,
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
            OhciFmInterval      = 0x34,
            OhciRhDescriptorA   = 0x48,     // HcRhDescriptorA register
            OhciRhStatus        = 0x50,
            OhciRhPortStatus    = 0x54,     // HcRhPortStatus registers

            OhciControlStateFunctionalMask = 0xC0,
            
            OhciControlInterruptRoute = 0x100,
            OhciControlStateRunning = 0x80,     // HostControllerFunctionalState bits for USBOPERATIONAL
            OhciControlListsEnable  = 0x30, // 0x34     // PeriodicListEnable, ControlListEnable and BulkListEnable bits

            OhciCommandRequestOwnership = 0x08, // Requests ownership change
            OhciCommandBulkListFilled   = 0x04, // BulkListFilled bit
            OhciCommandControlListFilled= 0x02, // ControlListFilled bit
            OhciCommandHcReset          = 0x01, // HostControllerReset bit

            OhciInterruptMIE        = 0x80000000,   // MasterInterruptEnable bit
            OhciInterruptRhStsChange= 0x40,         // RootHubStatusChange bit
            OhciInterruptUnrecoverableError = 0x10,
            OhciInterruptWbDoneHead = 0x02,         // WritebackDoneHead bit
            OhciInterruptStartOfFrame = 0x04,         // StartOfFrame interrupt

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

        /// Global lock.
        Mutex m_Mutex;

        Hcca *m_pHcca;
        uintptr_t m_pHccaPhys;
        
        /// Lock for modifying the schedule list itself (m_FullSchedule)
        Spinlock m_ScheduleChangeLock;
        
        /// Lock for changing the periodic list.
        Spinlock m_PeriodicListChangeLock;
        
        /// Lock for changing the control list.
        Spinlock m_ControlListChangeLock;
        
        /// Lock for changing the bulk list.
        Spinlock m_BulkListChangeLock;

        ED *m_pPeriodicEDList;
        uintptr_t m_pPeriodicEDListPhys;
        ExtensibleBitmap m_PeriodicEDBitmap;

        ED *m_pControlEDList;
        uintptr_t m_pControlEDListPhys;
        ExtensibleBitmap m_ControlEDBitmap;
        
        ED *m_pBulkEDList;
        uintptr_t m_pBulkEDListPhys;
        ExtensibleBitmap m_BulkEDBitmap;

        TD *m_pTDList;
        uintptr_t m_pTDListPhys;
        ExtensibleBitmap m_TDBitmap;

        // Pointers to the current bulk and control queue heads (can be null)
        ED *m_pBulkQueueHead;
        ED *m_pControlQueueHead;
        
        // Pointers to the current bulk and control queue tails
        ED *m_pBulkQueueTail;
        ED *m_pControlQueueTail;

        // Pointer to the current periodic queue tail
        ED *m_pPeriodicQueueTail;
        
        /// Dequeue list lock.
        Spinlock m_DequeueListLock;
        
        /// List of ED pointers in both the control and bulk queues. Used for
        /// IRQ handling.
        List<ED*> m_FullSchedule;
        
        /// List of EDs ready for dequeue (reclaiming)
        List<ED*> m_DequeueList;
        
        /// Semaphore for the dequeue list
        Semaphore m_DequeueCount;

        MemoryRegion m_OhciMR;

        Ohci(const Ohci&);
        void operator =(const Ohci&);
};

#endif
