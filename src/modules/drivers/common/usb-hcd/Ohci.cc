/*
 * Copyright (c) 2010 Eduard Burtescu
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITRTLSS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, RTLGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONRTLCTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <machine/Machine.h>
#include <processor/Processor.h>
#include <usb/Usb.h>
#include <Log.h>
#ifdef X86_COMMON
#include <machine/Pci.h>
#endif
#include "Ohci.h"

#define delay(n) do{Semaphore semWAIT(0);semWAIT.acquire(1, 0, n*1000);}while(0)

#define INDEX_FROM_TD(ptr) (((reinterpret_cast<uintptr_t>((ptr)) & 0xFFF) / sizeof(TD)))
#define PHYS_TD(idx)        (m_pTDListPhys + ((idx) * sizeof(TD)))

Ohci::Ohci(Device* pDev) : Device(pDev), m_OhciMR("Ohci-MR")
{
    setSpecificType(String("OHCI"));
    // Allocate the memory region
    if(!PhysicalMemoryManager::instance().allocateRegion(m_OhciMR, 3, PhysicalMemoryManager::continuous, 0))
    {
        ERROR("USB: OHCI: Couldn't allocate memory region!");
        return;
    }
    
    uintptr_t virtualBase   = reinterpret_cast<uintptr_t>(m_OhciMR.virtualAddress());
    uintptr_t physicalBase  = m_OhciMR.physicalAddress();
    m_pHcca                 = reinterpret_cast<Hcca*>(virtualBase);
    m_pEDList               = reinterpret_cast<ED*>(virtualBase + 0x1000);
    m_pTDList               = reinterpret_cast<TD*>(virtualBase + 0x2000);

    m_pHccaPhys             = physicalBase;
    m_pEDListPhys           = physicalBase + 0x1000;
    m_pTDListPhys           = physicalBase + 0x2000;

    memset(m_pHcca, 0, 0x800);

    m_EDBitmap.set(0);
    ED *pBulkED = &m_pEDList[0];
    memset(pBulkED, 0, sizeof(ED));

    m_EDBitmap.set(1);
    ED *pControlED = &m_pEDList[1];
    memset(pControlED, 0, sizeof(ED));

    m_pCurrentBulkQueueHead = m_pCurrentBulkQueueTail = pBulkED;
    m_pCurrentControlQueueHead = m_pCurrentControlQueueTail = pControlED;

#ifdef X86_COMMON
    uint32_t nPciCmdSts = PciBus::instance().readConfigSpace(this, 1);
    DEBUG_LOG("USB: OHCI: Pci command: " << (nPciCmdSts&0xffff));
    PciBus::instance().writeConfigSpace(this, 1, nPciCmdSts | 0x4);
#endif

    // Grab the ports
    m_pBase = m_Addresses[0]->m_Io;
    
    // Set reset bit
    m_pBase->write32(OhciCommandHcReset, OhciCommandStatus);
    while(m_pBase->read32(OhciCommandStatus) & OhciCommandHcReset)
        delay(5);
    
    // Set Hcca pointer
    m_pBase->write32(m_pHccaPhys, OhciHcca);
    
    // Install the IRQ handler
#ifdef X86_COMMON
    Machine::instance().getIrqManager()->registerIsaIrqHandler(getInterruptNumber(), static_cast<IrqHandler*>(this));
#else
    InterruptManager::instance().registerInterruptHandler(pDev->getInterruptNumber(), this);
#endif

    // Enable wanted interrupts and clean the interrupt status
    m_pBase->write32(0x4000007f, OhciInterruptStatus);
    m_pBase->write32(OhciInterruptMIE | OhciInterruptWbDoneHead, OhciInterruptEnable);

    // Set up the control and bulk head EDs
    m_pBase->write32(m_pEDListPhys, OhciBulkHeadED);
    m_pBase->write32(m_pEDListPhys + sizeof(ED), OhciControlHeadED);
    
    // Enable control and bulk lists
    m_pBase->write32(OhciControlListsEnable | OhciControlStateRunning, OhciControl);
    m_pBase->write32(OhciCommandCtlListFilled | OhciCommandBlkListFilled, OhciCommandStatus);

    // Get the number of ports
    uint32_t ohciDescA = m_pBase->read32(OhciRhDescriptorA);
    m_nPorts = ohciDescA & 0xFF;
    uint8_t powerWait = ((ohciDescA >> 24) & 0xFF) * 2;
    DEBUG_LOG("USB: OHCI: Reset complete, " << Dec << m_nPorts << Hex << " ports available");
    for(size_t i = 0;i<m_nPorts;i++)
    {
        uint16_t nPortStatus = m_pBase->read32(OhciRhPortStatus + (i * 4));
        if(!(nPortStatus & (1 << 8)))
        {
            // Needs port power, do so
            nPortStatus |= (1 << 8);
            m_pBase->write32(nPortStatus, OhciRhPortStatus + (i * 4));

            // Wait as long as it needs
            delay(powerWait);

            // Read updated status
            nPortStatus = m_pBase->read32(OhciRhPortStatus + (i * 4));
        }

        // Perform a reset of the port
        m_pBase->write32(OhciRhPortStsReset, OhciRhPortStatus + (i * 4));
        while(!(m_pBase->read32(OhciRhPortStatus + (i * 4)) & OhciRhPortStsResCh))
            delay(5);
        m_pBase->write32(OhciRhPortStsResCh, OhciRhPortStatus + (i * 4));
    
        // Read updated status
        nPortStatus = m_pBase->read32(OhciRhPortStatus + (i * 4));

        // Check for a connected device
        if(nPortStatus & OhciRhPortStsConnected)
        {
            // Enable the port if not already enabled
            if(!(nPortStatus & OhciRhPortStsEnable))
                m_pBase->write32(nPortStatus | OhciRhPortStsEnable, OhciRhPortStatus + (i * 4));

            // Determine the speed of the attached device
            if(nPortStatus & (1 << 9))
            {
                DEBUG_LOG("USB: OHCI: Port " << Dec << i << Hex << " has a low-speed device connected to it");
                deviceConnected(i, LowSpeed);
            }
            else
            {
                DEBUG_LOG("USB: OHCI: Port " << Dec << i << Hex << " has a full-speed device connected to it");
                deviceConnected(i, FullSpeed);
            }
        }
    }
    // Enable RootHubStatusChange interrupt now that it's safe to
    m_pBase->write32(OhciInterruptRhStsChange, OhciInterruptStatus);
    m_pBase->write32(OhciInterruptMIE | OhciInterruptRhStsChange | OhciInterruptWbDoneHead, OhciInterruptEnable);
}

Ohci::~Ohci()
{
}

#ifdef X86_COMMON
bool Ohci::irq(irq_id_t number, InterruptState &state)
#else
void Ohci::interrupt(size_t number, InterruptState &state)
#endif
{
    uint32_t nStatus = m_pBase->read32(OhciInterruptStatus) & m_pBase->read32(OhciInterruptEnable);
#ifdef USB_VERBOSE_DEBUG
    DEBUG_LOG("IRQ "<<nStatus);
#endif
    /*
    if(nStatus & OhciInterruptWbDoneHead)
    {
        uint8_t nEDIndex = m_pHcca->pDoneHead >> 5 & 0x7f;
        ED *pED = &m_pEDList[nEDIndex];
        TD *pTD = &m_pTDList[nEDIndex];
        // Call the callback, this TD is done
        ssize_t nReturn = pTD->nStatus?-pTD->nStatus:pTD->nBufferSize;//pTD->pBufferEnd-pTD->pBufferStart+1;
        if(pTD->nPid == 2 && pTD->pBuffer && nReturn >= 0)
            memcpy(reinterpret_cast<void*>(pTD->pBuffer), &m_pTransferPages[pTD->nBufferOffset], nReturn);
        if(pTD->pCallback)
        {
            void (*pCallback)(uintptr_t, ssize_t) = reinterpret_cast<void(*)(uintptr_t, ssize_t)>(pTD->pCallback);
            pCallback(pTD->pParam, nReturn);
        }
        m_TransferPagesAllocator.free(pTD->nBufferOffset, pTD->nBufferSize);
        m_EDBitmap.clear(nEDIndex);
        //if(nReturn < 0)
            DEBUG_LOG("STOP "<<Dec<<pED->nAddress<<":"<<pED->nEndpoint<<" "<<(pTD->nPid==1?" OUT ":(pTD->nPid==2?" IN  ":(pTD->nPid==0?"SETUP":"")))<<" "<<nReturn<<Hex);
    }
    */
    //else
    m_pBase->write32(nStatus, OhciInterruptStatus);
    /*if(nStatus & OHCI_STS_INT)
    {
        // Pause the controller
        m_pBase->write32(0, OHCI_CMD);
        // Check every async TD
        TD *pTD = m_pAsyncED->pCurrent;
        while(pTD)
        {
            // Stop if we've reached a TD that is not finished
            if(pTD->status == 0x80)
                break;
            // Call the callback, this TD is done
            ssize_t nReturn = pTD->status & 0x7e?-(pTD->status & 0x7e):(pTD->actlen + 1) % 0x800;
            if(pTD->nPid == UsbPidIn && pTD->buffer && nReturn >= 0)
                memcpy(reinterpret_cast<void*>(pTD->buffer), &m_pTransferPages[pTD->buff-m_pTransferPagesPhys], nReturn);
            if(pTD->pCallback)
            {
                void (*pCallback)(uintptr_t, ssize_t) = reinterpret_cast<void(*)(uintptr_t, ssize_t)>(pTD->pCallback);
                pCallback(pTD->param, nReturn);
            }
            NOTICE("STOP "<<Dec<<pTD->nAddress<<":"<<pTD->nEndpoint<<" "<<(pTD->nPid==UsbPidOut?" OUT ":(pTD->nPid==UsbPidIn?" IN  ":(pTD->nPid==UsbPidSetup?"SETUP":"")))<<" "<<nReturn<<Hex);
            pTD = pTD->next_invalid?0:&m_pTDList[pTD->next & 0xfff];
        }
        m_pAsyncED->pCurrent = pTD;
        m_pAsyncED->elem = pTD?pTD->phys:0;
        m_pAsyncED->elem_invalid = pTD?0:1;

        // Check every periodic TD
        pTD = m_pPeriodicED->pCurrent;
        while(pTD)
        {
            // Stop if we've reached a TD that is not finished
            if(pTD->status == 0x80)
            {
                m_pPeriodicED->pCurrent = pTD;
                break;
            }
            // Call the callback, this TD is done
            if(!pTD->status)
            {
                ssize_t nReturn = (pTD->actlen + 1) % 0x800;
                if(pTD->nPid == UsbPidIn && pTD->buffer)
                    memcpy(reinterpret_cast<void*>(pTD->buffer), &m_pTransferPages[pTD->buff-m_pTransferPagesPhys], nReturn);
                if(pTD->pCallback)
                {
                    void (*pCallback)(uintptr_t, ssize_t) = reinterpret_cast<void(*)(uintptr_t, ssize_t)>(pTD->pCallback);
                    pCallback(pTD->param, nReturn);
                }
            }
            pTD->status = 0x80;
            // Stop if we reached where we started
            if(pTD->next == m_pPeriodicED->pCurrent->phys)
                break;
            pTD = &m_pTDList[pTD->next & 0xfff];
        }
        // Resume the controller
        m_pBase->write32(OHCI_CMD_RUN, OHCI_CMD);
    }*/
#ifdef X86_COMMON
    return true;
#endif
}

void Ohci::addTransferToTransaction(uintptr_t pTransaction, bool bToggle, UsbPid pid, uintptr_t pBuffer, size_t nBytes)
{
    // Atomic operation: find clear bit, set it
    size_t nIndex = 0;
    {
        LockGuard<Mutex> guard(m_Mutex);
        nIndex = m_TDBitmap.getFirstClear();
        if(nIndex >= (0x1000 / sizeof(TD)))
        {
            ERROR("USB: OHCI: TD space full");
            return;
        }
        m_TDBitmap.set(nIndex);
    }

    // Grab the TD pointer we're going to set up now
    TD *pTD = &m_pTDList[nIndex];
    memset(pTD, 0, sizeof(TD));

    // Buffer rounding - allow packets smaller than the buffer we specify
    pTD->bBuffRounding = 1;

    // PID for the transfer
    switch(pid)
    {
        case UsbPidSetup:
            pTD->nPid = 0;
            break;
        case UsbPidOut:
            pTD->nPid = 1;
            break;
        case UsbPidIn:
            pTD->nPid = 2;
            break;
        default:
            pTD->nPid = 3;
    };

    // Active
    pTD->nStatus = 0xf;

    // Buffer for transer
    if(nBytes)
    {
        VirtualAddressSpace &va = Processor::information().getVirtualAddressSpace();
        if(va.isMapped(reinterpret_cast<void*>(pBuffer)))
        {
            physical_uintptr_t phys = 0; size_t flags = 0;
            va.getMapping(reinterpret_cast<void*>(pBuffer), phys, flags);
            pTD->pBufferStart = phys + (pBuffer & 0xFFF);
        }
        else
        {
            ERROR("UHCI: addTransferToTransaction: Buffer (page " << Dec << pBuffer << Hex << ") isn't mapped!");
            m_TDBitmap.clear(nIndex);
            return;
        }

        pTD->nBufferSize = nBytes;
    }
    
    // Grab the transaction's ED and add our qTD to it
    ED *pED = &m_pEDList[pTransaction];
    if(pED->pMetaData->pLastQTD)
    {
        pED->pMetaData->pLastQTD->pNext = PHYS_TD(nIndex) >> 4;
        pTD->pNext = PHYS_TD(INDEX_FROM_TD(pED->pMetaData->pFirstQTD)) >> 4; // Loop around to the front (stopping this will be handled by the IRQ handler)

        // Update the tail pointer in the ED
        pED->pTailTD = PHYS_TD(nIndex) >> 4;
    }
    else
    {
        pED->pMetaData->pFirstQTD = pTD;
        pED->pHeadTD = PHYS_TD(nIndex) >> 4;
    }
    pED->pMetaData->pLastQTD = pTD;
}

uintptr_t Ohci::createTransaction(UsbEndpoint endpointInfo)
{
    // Atomic operation: find clear bit, set it
    size_t nIndex = 0;
    {
        LockGuard<Mutex> guard(m_Mutex);
        nIndex = m_EDBitmap.getFirstClear();
        if(nIndex >= (0x2000 / sizeof(ED)))
        {
            ERROR("USB: EHCI: ED space full");
            return static_cast<uintptr_t>(-1);
        }
        m_EDBitmap.set(nIndex);
    }

    ED *pED = &m_pEDList[nIndex];
    memset(pED, 0, sizeof(ED));

    // Loop back on this ED for now
    pED->pNext = (m_pEDListPhys + nIndex * sizeof(ED)) >> 4;

    // Device address, endpoint and speed
    pED->nAddress = endpointInfo.nAddress;
    pED->nEndpoint = endpointInfo.nEndpoint;
    pED->bLoSpeed = endpointInfo.speed == LowSpeed;

    // Don't skip
    pED->bSkip = 0;

    // Maximum packet size
    pED->nMaxPacketSize = endpointInfo.nMaxPacketSize;

    // Set up initial data toggle
    pED->bToggleCarry = 0;

    // Setup the metadata
    ED::MetaData *pMetaData = new ED::MetaData;
    pMetaData->bPeriodic = false;
    pMetaData->pFirstQTD = 0;
    pMetaData->pLastQTD = 0;
    pMetaData->pNext = 0;
    pMetaData->pPrev = 0;
    pMetaData->nTotalBytes = 0;
    pMetaData->bIgnore = false;
    pMetaData->endpointInfo = endpointInfo;

    pED->pMetaData = pMetaData;

    // Complete
    return nIndex;
}

void Ohci::doAsync(uintptr_t pTransaction, void (*pCallback)(uintptr_t, ssize_t), uintptr_t pParam)
{
    LockGuard<Mutex> guard(m_Mutex);
    if((pTransaction == static_cast<uintptr_t>(-1)) || !m_EDBitmap.test(pTransaction))
    {
        ERROR("OHCI: doAsync: didn't get a valid transaction id [" << pTransaction << "].");
        return;
    }

    ED *pED = &m_pEDList[pTransaction];
    pED->pMetaData->pCallback = pCallback;
    pED->pMetaData->pParam = pParam;

    ED *pCurrentQueueHead = 0;
    ED *pCurrentQueueTail = 0;
    if(pED->pMetaData->endpointInfo.nEndpoint)
    {
        pCurrentQueueHead = m_pCurrentBulkQueueHead;
        pCurrentQueueTail = m_pCurrentBulkQueueTail;
    }
    else
    {
        pCurrentQueueHead = m_pCurrentControlQueueHead;
        pCurrentQueueTail = m_pCurrentControlQueueTail;
    }

    // Link in to the asynchronous schedule
    if(pCurrentQueueTail)
    {
        // Current ED needs to point to the schedule's head
        size_t queueHeadIndex = (reinterpret_cast<uintptr_t>(pCurrentQueueHead) & 0xFFF) / sizeof(ED);
        pED->pNext = (m_pEDListPhys + (queueHeadIndex * sizeof(ED))) >> 5;

        // Enter the information for correct dequeue
        pED->pMetaData->pNext = pCurrentQueueHead;
        pED->pMetaData->pPrev = pCurrentQueueTail;

        ED *pOldTail = pCurrentQueueTail;

        {
            // Atomic operation - modifying both the housekeeping and the
            // hardware linked lists
            LockGuard<Spinlock> guard(m_QueueListChangeLock);

            // Update the tail pointer
            if(pED->pMetaData->endpointInfo.nEndpoint)
                m_pCurrentBulkQueueTail = pED;
            else
                m_pCurrentControlQueueTail = pED;

            // The current tail needs to point to this ED
            pOldTail->pNext = (m_pEDListPhys + (pTransaction * sizeof(ED))) >> 5;

            // Finally, fix the linked list
            pOldTail->pMetaData->pNext = pED;
            pOldTail->pMetaData->pPrev = pED;
        }

        // Set up the control and bulk head EDs in the queue
        if(pED->pMetaData->endpointInfo.nEndpoint)
        {
            m_pBase->write32(m_pEDListPhys, OhciBulkHeadED);
            m_pBase->write32(OhciCommandBlkListFilled, OhciCommandStatus);
        }
        else
        {
            m_pBase->write32(m_pEDListPhys + sizeof(ED), OhciControlHeadED);
            m_pBase->write32(OhciCommandCtlListFilled, OhciCommandStatus);
        }
    }
    else
    {
        ERROR("OHCI: Queue tail is null!");
    }
}

void Ohci::addInterruptInHandler(UsbEndpoint endpointInfo, uintptr_t pBuffer, uint16_t nBytes, void (*pCallback)(uintptr_t, ssize_t), uintptr_t pParam)
{
    /// \todo Implement - HCCA and all that
}

/*
void Ohci::doAsync(UsbEndpoint endpointInfo, uint8_t nPid, uintptr_t pBuffer, uint16_t nBytes, void (*pCallback)(uintptr_t, ssize_t), uintptr_t pParam)
{
    LockGuard<Mutex> guard(m_Mutex);
    DEBUG_LOG("START "<<Dec<<endpointInfo.nAddress<<":"<<endpointInfo.nEndpoint<<" "<<(nPid==UsbPidOut?" OUT ":(nPid==UsbPidIn?" IN  ":(nPid==UsbPidSetup?"SETUP":"")))<<" "<<nBytes<<Hex);

    // Get a buffer somewhere in our transfer pages
    uintptr_t nBufferOffset = 0;
    if(nBytes)
    {
        if(!m_TransferPagesAllocator.allocate(nBytes, nBufferOffset))
            FATAL("USB: OHCI: Buffers full :(");
        if(nPid != UsbPidIn && pBuffer)
            memcpy(&m_pTransferPages[nBufferOffset], reinterpret_cast<void*>(pBuffer), nBytes);
    }

    // Get an unused ED
    size_t nEDIndex = m_EDBitmap.getFirstClear();
    if(nEDIndex > 127)
        FATAL("USB: OHCI: ED/TD space full :(");
    m_EDBitmap.set(nEDIndex);

    ED *pED = &m_pEDList[nEDIndex];
    memset(pED, 0, sizeof(ED));
    pED->nAddress = endpointInfo.nAddress;
    pED->nEndpoint = endpointInfo.nEndpoint;
    pED->bLoSpeed = endpointInfo.speed == LowSpeed;
    pED->nMaxPacketSize = 8;
    pED->pHeadTD = (m_pTDListPhys+nEDIndex*sizeof(TD))>>4;

    TD *pTD = &m_pTDList[nEDIndex];
    memset(pTD, 0, sizeof(TD));
    pTD->bBuffRounding = 1;
    pTD->nPid = nPid==UsbPidSetup?0:(nPid==UsbPidOut?1:(nPid==UsbPidIn?2:3));
    pTD->bDataToggleSrc = 1;
    pTD->bDataToggle = endpointInfo.bDataToggle;
    pTD->nStatus = 0xf;
    if(nBytes)
    {
        pTD->pBufferStart = m_pTransferPagesPhys+nBufferOffset;
        pTD->pBufferEnd = m_pTransferPagesPhys+nBufferOffset+nBytes-1;
        pTD->nBufferSize = nBytes;
        pTD->nBufferOffset = nBufferOffset;
    }
    pTD->pCallback = reinterpret_cast<uintptr_t>(pCallback);
    pTD->pParam = pParam;
    pTD->pBuffer = pBuffer;

    // Change our async ED to include the new TD
    //m_pAsyncED->elem = pTD->phys;
    //m_pAsyncED->elem_invalid = 0;
    //m_pAsyncED->pCurrent = pTD;

    bool bControl = !endpointInfo.nEndpoint;
    m_pBase->write32(m_pEDListPhys+nEDIndex*sizeof(ED), bControl?OhciControlHeadED:OhciBulkHeadED);
    m_pBase->write32(bControl?OhciCommandCtlListFilled:OhciCommandBlkListFilled, OhciCommandStatus);
}


void Ohci::addInterruptInHandler(uint8_t nAddress, uint8_t nEndpoint, uintptr_t pBuffer, uint16_t nBytes, void (*pCallback)(uintptr_t, ssize_t), uintptr_t pParam)
{
    /// \note not implemented
    LockGuard<Mutex> guard(m_Mutex);

    // Pause the controller
    m_pBase->write32(0, OHCI_CMD);

    // Fill the TD with data
    TD *pTD = &m_pTDList[1];
    memset(pTD, 0, sizeof(TD));
    pTD->phys = (m_pTDListPhys+1*sizeof(TD))>>4;
    pTD->next = m_pPeriodicED->pCurrent?m_pPeriodicED->pCurrent->next:pTD->phys;
    pTD->status = 0x80;
    pTD->ioc = 1;
    pTD->speed = 1;
    pTD->spd = 0;
    pTD->nPid = UsbPidIn;
    pTD->nAddress = nAddress;
    pTD->nEndpoint = nEndpoint;
    pTD->data_toggle = 0;
    pTD->maxlen = nBytes?nBytes-1:0x7ff;
    pTD->buff = m_pTransferPagesPhys;
    pTD->pCallback = reinterpret_cast<uintptr_t>(pCallback);
    pTD->param = pParam;
    pTD->buffer = pBuffer;

    // Change our periodic ED to include the new TD
    if(m_pPeriodicED->pCurrent)
        m_pPeriodicED->pCurrent->next = pTD->phys;
    else
    {
        m_pPeriodicED->elem = pTD->phys;
        m_pPeriodicED->elem_invalid = 0;
    }
    m_pPeriodicED->pCurrent = pTD;

    // Resume the controller
    m_pBase->write32(OHCI_CMD_RUN, OHCI_CMD);
}*/
