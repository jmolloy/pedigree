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
#include <machine/Pci.h>
#include <processor/Processor.h>
#include <usb/Usb.h>
#include <Log.h>
#include "Uhci.h"

#define delay(n) do{Semaphore semWAIT(0);semWAIT.acquire(1, 0, n*1000);}while(0)

#define INDEX_FROM_QTD(ptr) (((reinterpret_cast<uintptr_t>((ptr)) & 0xFFF) / sizeof(TD)))
#define PHYS_QTD(idx)        (m_pTDListPhys + ((idx) * sizeof(TD)))

#define GET_PAGE(param, buffer, qhIndex) \
do \
{ \
    if(va.isMapped(reinterpret_cast<void*>((buffer)))) \
    { \
        physical_uintptr_t phys = 0; size_t flags = 0; \
        va.getMapping(reinterpret_cast<void*>((buffer)), phys, flags); \
        (param) = phys + ((buffer) & 0xFFF); \
    } \
    else \
    { \
        ERROR("UHCI: addTransferToTransaction: Buffer (page " << Dec << (buffer) << Hex << ") isn't mapped!"); \
        m_QHBitmap.clear((qhIndex)); \
        return; \
    } \
} while(0)

Uhci::Uhci(Device* pDev) : Device(pDev), m_pBase(0), m_nPorts(0), m_nFrames(0), m_UhciMR("Uhci-MR"),
                           m_QueueListChangeLock(), m_pCurrentQueueTail(0), m_pCurrentQueueHead(0)
{
    setSpecificType(String("UHCI"));

    // Allocate the memory region
    if(!PhysicalMemoryManager::instance().allocateRegion(m_UhciMR, 3, PhysicalMemoryManager::continuous, VirtualAddressSpace::Write | VirtualAddressSpace::KernelMode))
    {
        ERROR("USB: UHCI: Couldn't allocate memory region!");
        return;
    }

    uintptr_t pVirtBase = reinterpret_cast<uintptr_t>(m_UhciMR.virtualAddress());
    physical_uintptr_t pPhysBase = m_UhciMR.physicalAddress();

    m_pFrameList        = reinterpret_cast<uint32_t*>(pVirtBase);
    m_pQHList           = reinterpret_cast<QH*>(pVirtBase + 0x1000);
    m_pTDList           = reinterpret_cast<TD*>(pVirtBase + 0x2000);

    m_pFrameListPhys    = pPhysBase;
    m_pQHListPhys       = pPhysBase + 0x1000;
    m_pTDListPhys       = pPhysBase + 0x2000;

    // Allocate room for the Periodic and Async QHs
    size_t nPeriodicIndex = m_QHBitmap.getFirstClear();
    // m_QHBitmap.set(nPeriodicIndex);
    size_t nAsyncIndex = m_QHBitmap.getFirstClear();
    m_QHBitmap.set(nAsyncIndex);

    m_pAsyncQH = &m_pQHList[nAsyncIndex];
    m_pPeriodicQH = &m_pQHList[nPeriodicIndex];

    dmemset(m_pFrameList, (m_pQHListPhys + (nAsyncIndex * sizeof(QH))) | 2, 0x400);

    memset(m_pAsyncQH, 0, sizeof(QH));
    memset(m_pPeriodicQH, 0, sizeof(QH));

    // m_pAsyncQH->pNext = (m_pQHListPhys + (nPeriodicIndex * sizeof(QH))) >> 4;
    m_pAsyncQH->pNext = (m_pQHListPhys + (nAsyncIndex * sizeof(QH))) >> 4;
    m_pAsyncQH->bNextQH = 1;
    m_pAsyncQH->bElemInvalid = 1;

    m_pPeriodicQH->pNext = (m_pQHListPhys + (nAsyncIndex * sizeof(QH))) >> 4;
    m_pPeriodicQH->bNextQH = 1;
    m_pPeriodicQH->bElemInvalid = 1;

    m_pCurrentQueueTail = m_pCurrentQueueHead = m_pAsyncQH;

    uint32_t nCommand = PciBus::instance().readConfigSpace(this, 1);
#ifdef USB_VERBOSE_DEBUG
    DEBUG_LOG("USB: UHCI: Pci command+status: " << nCommand);
#endif
    PciBus::instance().writeConfigSpace(this, 1, nCommand | 0x4);

    // Grab the ports
    m_pBase = m_Addresses[0]->m_Io;

    // Set reset bit
    m_pBase->write16(UHCI_CMD_GRES, UHCI_CMD);
    delay(50);
    m_pBase->write16(0, UHCI_CMD);

    // Write frame list pointer
    m_pBase->write32(m_pFrameListPhys, UHCI_FRLP);

    // Enable wanted interrupts
    m_pBase->write16(0xf, UHCI_INTR);

    // Start the controller
    m_pBase->write16(1, UHCI_CMD);

    // Install the IRQ handler
    Machine::instance().getIrqManager()->registerIsaIrqHandler(getInterruptNumber(), static_cast<IrqHandler*>(this));

    DEBUG_LOG("USB: UHCI: Reseted");

    for(size_t i = 0;i<7;i++)
    {
        uint16_t nPortStatus = m_pBase->read16(UHCI_PORTSC+i*2);
        if(nPortStatus == 0xffff || !(nPortStatus & 0x80))
            break;
        DEBUG_LOG("USB: UHCI: Port "<<Dec<<i<<Hex<<" got "<<m_pBase->read16(UHCI_PORTSC+i*2));
        if(nPortStatus & UHCI_PORTSC_CONN)
        {
            m_pBase->write16(UHCI_PORTSC_PRES | UHCI_PORTSC_CSCH, UHCI_PORTSC+i*2);
            delay(50);
            m_pBase->write16(0, UHCI_PORTSC+i*2);
            DEBUG_LOG("USB: UHCI: Port "<<Dec<<i<<Hex<<" got "<<m_pBase->read16(UHCI_PORTSC+i*2));
            delay(50);
            m_pBase->write16(UHCI_PORTSC_ENABLE/* | UHCI_PORTSC_EDCH*/, UHCI_PORTSC+i*2);
            DEBUG_LOG("USB: UHCI: Port "<<Dec<<i<<Hex<<" got "<<m_pBase->read16(UHCI_PORTSC+i*2));
            bool bLoSpeed = m_pBase->read16(UHCI_PORTSC+i*2) & UHCI_PORTSC_LOSPEED;
            DEBUG_LOG("USB: UHCI: Port "<<Dec<<i<<Hex<<" is connected at "<<(bLoSpeed?"Low Speed":"Full Speed"));
            deviceConnected(i, bLoSpeed?LowSpeed:FullSpeed);
        }
        m_nPorts++;
    }
}

Uhci::~Uhci()
{
}

static int threadStub(void *p)
{
    Uhci *pUhci = reinterpret_cast<Uhci*>(p);
    pUhci->doDequeue();
    return 0;
}

void Uhci::doDequeue()
{
    // Absolutely cannot have queue insetions during a dequeue
    LockGuard<Mutex> guard(m_Mutex);

    for(size_t i = 1; i < 128; i++)
    {
        if(!m_QHBitmap.test(i))
            continue;

        QH *pQH = &m_pQHList[i];

        // Is this QH valid?
        if(!pQH->pMetaData)
        {
#ifdef USB_VERBOSE_DEBUG
            DEBUG_LOG("Not performing dequeue on QH #" << Dec << i << Hex << " as it's not even initialised.");
#endif
            continue;
        }

        // Is this QH even linked!?
        if(!pQH->pMetaData->bIgnore)
        {
#ifdef USB_VERBOSE_DEBUG
            DEBUG_LOG("Not performing dequeue on QH #" << Dec << i << Hex << " as it's still active.");
#endif
            continue;
        }

        if(!pQH->pMetaData->qTDCount)
        {
            // Remove all qTDs
            size_t nQTDIndex = INDEX_FROM_QTD(pQH->pMetaData->pFirstQTD);
            while(true)
            {
                m_qTDBitmap.clear(nQTDIndex);

                TD *pqTD = &m_pTDList[nQTDIndex];
                bool shouldBreak = pqTD->bNextInvalid;
                if(!shouldBreak)
                    nQTDIndex = ((pqTD->pNext << 4) & 0xFFF) / sizeof(TD);

                memset(pqTD, 0, sizeof(TD));

                if(shouldBreak)
                    break;
            }

            // Completely invalidate the QH
            memset(pQH, 0, sizeof(QH));

#ifdef USB_VERBOSE_DEBUG
            DEBUG_LOG("Dequeue for QH #" << Dec << i << Hex << ".");
#endif

            // This QH is done
            m_QHBitmap.clear(i);
        }
    }
}

bool Uhci::irq(irq_id_t number, InterruptState &state)
{
    uint16_t nStatus = m_pBase->read16(UHCI_STS);
    m_pBase->write16(nStatus, UHCI_STS);

    DEBUG_LOG("UHCI IRQ " << nStatus);

    bool bDequeue = false;
    if(nStatus & UHCI_STS_INT)
    {
        // Check all the queue heads we have in the list
        for(size_t i = 0; i < (0x1000 / sizeof(QH)); i++)
        {
            if(!m_QHBitmap.test(i))
                continue;

            QH *pQH = &m_pQHList[i];
            if(!pQH->pMetaData) // This QH isn't actually ready to be handled yet.
                continue;
            if(!(pQH->pMetaData->pPrev && pQH->pMetaData->pNext)) // This QH isn't actually linked yet
                continue;
            if(pQH->pMetaData->bIgnore)
                continue;
            
            bool bPeriodic = pQH->pMetaData->bPeriodic;

            // Iterate the TD list
            size_t nQTDIndex = INDEX_FROM_QTD(pQH->pMetaData->pFirstQTD);
            while(true)
            {
                TD *pqTD = &m_pTDList[nQTDIndex];

                if(pqTD->nStatus != 0x80)
                {
                    ssize_t nResult;
                    if((pqTD->nStatus & 0x7c) || (nStatus & UHCI_STS_ERR))
                    {
#ifdef USB_VERBOSE_DEBUG
                        ERROR_NOLOCK(((nStatus & UHCI_STS_ERR) ? "USB" : "qTD") << " ERROR!");
                        ERROR_NOLOCK("qTD Status: " << pqTD->nStatus);
#endif
                        nResult = -(pqTD->nStatus & 0x7c);
                    }
                    else
                    {
                        nResult = (pqTD->nActLen + 1) > pqTD->nBufferSize ? pqTD->nBufferSize : (pqTD->nActLen + 1);
                        pQH->pMetaData->nTotalBytes += nResult;
                    }
#ifdef USB_VERBOSE_DEBUG
                    DEBUG_LOG_NOLOCK("qTD #" << Dec << nQTDIndex << Hex << " [from QH #" << Dec << i << Hex << "] DONE: " << Dec << pqTD->nAddress << ":" << pqTD->nEndpoint << " " << (pqTD->nPid==UsbPidOut?"OUT":(pqTD->nPid==UsbPidIn?"IN":(pqTD->nPid==UsbPidSetup?"SETUP":""))) << " " << nResult << Hex);
#endif

                    // Last qTD or error condition?
                    if((nResult < 0) || (pqTD == pQH->pMetaData->pLastQTD))
                    {
                        // Valid callback?
                        if(pQH->pMetaData->pCallback)
                        {
                            pQH->pMetaData->pCallback(pQH->pMetaData->pParam, nResult < 0 ? nResult : pQH->pMetaData->nTotalBytes);
                        }

                        // Caused by error?
                        if(nResult < 0)
                            pQH->pMetaData->qTDCount = 1; // Decrement will occur in following block
                    }
                    if(!bPeriodic)
                    {
                        // A handled qTD, hurrah!
                        pQH->pMetaData->qTDCount--;

                        /// \todo Errors will leave qTDs unfreed
                        /// \todo Errors will leave qTDs active!
                        if(!pQH->pMetaData->qTDCount) // This count starts from one, not zero
                        {
                            pQH->pMetaData->bIgnore = true;

                            // This queue head is done, dequeue.
                            QH *pPrev = pQH->pMetaData->pPrev;
                            QH *pNext = pQH->pMetaData->pNext;

                            // Main non-hardware linked list update
                            pPrev->pMetaData->pNext = pNext;
                            pNext->pMetaData->pPrev = pPrev;

                            // Hardware linked list update
                            pPrev->pNext = pQH->pNext;
                            pPrev->bElemQH = 1;
                            pPrev->bNextInvalid = 0;

                            // Update the tail pointer if we need to
                            if(pQH == m_pCurrentQueueTail)
                            {
                                m_QueueListChangeLock.acquire(); // Atomic operation
                                m_pCurrentQueueTail = pPrev;
                                m_QueueListChangeLock.release();
                            }

                            bDequeue = true;
                        }
                    }
                    // Interrupt qTDs need constant refresh
                    if(bPeriodic && !(pQH->pMetaData->bIgnore))
                    {
                        pqTD->nStatus = 0x80;
                        pqTD->nMaxLen = pqTD->nBufferSize ? pqTD->nBufferSize - 1 : 0x7ff;
                        pqTD->nErr = 0;
                    }
                }

                size_t oldIndex = nQTDIndex;

                if(pqTD->bNextInvalid)
                    break;
                else
                    nQTDIndex = ((pqTD->pNext << 4) & 0xFFF) / sizeof(TD);

                if(nQTDIndex == oldIndex)
                {
                    ERROR_NOLOCK("UHCI: QH #" << Dec << i << Hex << "'s qTD list is invalid - circular reference!");
                    break;
                }
                else if(pqTD->pNext == 0)
                {
                    ERROR_NOLOCK("UHCI: QH #" << Dec << i << Hex << "'s qTD list is invalid - null pNext pointer (and T bit not set)!");
                    break;
                }
            }
        }
    }

    if(bDequeue)
        new Thread(Processor::information().getCurrentThread()->getParent(), threadStub, reinterpret_cast<void*>(this));

    return true;
}

void Uhci::addTransferToTransaction(uintptr_t pTransaction, bool bToggle, UsbPid pid, uintptr_t pBuffer, size_t nBytes)
{
    // Atomic operation: find clear bit, set it
    size_t nIndex = 0;
    {
        LockGuard<Mutex> guard(m_Mutex);
        nIndex = m_qTDBitmap.getFirstClear();
        if(nIndex >= (0x1000 / sizeof(TD)))
        {
            ERROR("USB: UHCI: TD space full");
            return;
        }
        m_qTDBitmap.set(nIndex);
    }

    // Grab the TD
    TD *pTD = &m_pTDList[nIndex];
    memset(pTD, 0, sizeof(TD));
    pTD->bNextInvalid = 1; // Assume next is invalid, will be zeroed if another TD is linked

    // Grab the QH
    QH *pQH = &m_pQHList[pTransaction];

    // Active, interrupt on completion, and only allow one retry
    pTD->nStatus = 0x80;
    pTD->bIoc = 1;
    pTD->nErr = 1;

    // PID for this transfer
    pTD->nPid = pid;

    // Speed information
    pTD->bLoSpeed = pQH->pMetaData->endpointInfo.speed == LowSpeed;

    // Don't care about short packet detection
    pTD->bSpd = 0;

    // Endpoint information
    pTD->nAddress = pQH->pMetaData->endpointInfo.nAddress;
    pTD->nEndpoint = pQH->pMetaData->endpointInfo.nEndpoint;
    pTD->bDataToggle = bToggle;

    // Transfer information
    pTD->nMaxLen = nBytes ? nBytes - 1 : 0x7ff;
    pTD->nBufferSize = nBytes;
    if(nBytes)
    {
        VirtualAddressSpace &va = Processor::information().getVirtualAddressSpace();
        GET_PAGE(pTD->pBuff, pBuffer, pTransaction); /// \todo TD clear on error?
    }

    // Link into the existing TD list
    if(pQH->pMetaData->pLastQTD)
    {
        pQH->pMetaData->pLastQTD->pNext = PHYS_QTD(nIndex) >> 4;
        pQH->pMetaData->pLastQTD->bNextInvalid = 0;
        pQH->pMetaData->pLastQTD->bNextQH = 0;
        pQH->pMetaData->pLastQTD->bNextDepth = 1;
    }
    else
    {
        pQH->pMetaData->pFirstQTD = pTD;
        pQH->pElem = PHYS_QTD(nIndex) >> 4;
        pQH->bElemInvalid = 0;
        pQH->bElemQH = 0;
    }
    pQH->pMetaData->pLastQTD = pTD;

    pQH->pMetaData->qTDCount++;
}

uintptr_t Uhci::createTransaction(UsbEndpoint endpointInfo)
{
    // Atomic operation: find clear bit, set it
    size_t nIndex = 0;
    {
        LockGuard<Mutex> guard(m_Mutex);
        nIndex = m_QHBitmap.getFirstClear();
        if(nIndex >= (0x2000 / sizeof(QH)))
        {
            ERROR("USB: UHCI: QH space full");
            return static_cast<uintptr_t>(-1);
        }
        m_QHBitmap.set(nIndex);
    }

    // Grab the QH
    QH *pQH = &m_pQHList[nIndex];
    memset(pQH, 0, sizeof(QH));

    // Only need to configure metadata, everything else is set during linkage and TD creation
    pQH->pMetaData = new QH::MetaData;
    memset(pQH->pMetaData, 0, sizeof(QH::MetaData));
    pQH->pMetaData->endpointInfo = endpointInfo;

    return nIndex;
}

void Uhci::doAsync(uintptr_t pTransaction, void (*pCallback)(uintptr_t, ssize_t), uintptr_t pParam)
{
    LockGuard<Mutex> guard(m_Mutex);
    if((pTransaction == static_cast<uintptr_t>(-1)) || !m_QHBitmap.test(pTransaction))
    {
        ERROR("UHCI: doAsync: didn't get a valid transaction id [" << pTransaction << "].");
        return;
    }

    // Configure remaining metadata
    QH *pQH = &m_pQHList[pTransaction];
    pQH->pMetaData->pCallback = pCallback;
    pQH->pMetaData->pParam = pParam;

    // Do we need to configure the asynchronous schedule?
    if(m_pCurrentQueueTail)
    {
        // Current QH needs to point to the schedule's head
        size_t queueHeadIndex = (reinterpret_cast<uintptr_t>(m_pCurrentQueueHead) & 0xFFF) / sizeof(QH);
        NOTICE("queue head index is " << queueHeadIndex);
        pQH->pNext = (m_pQHListPhys + (queueHeadIndex * sizeof(QH))) >> 4;
        pQH->bNextInvalid = 0;
        pQH->bNextQH = 1;

        // Enter the information for correct dequeue
        pQH->pMetaData->pNext = m_pCurrentQueueHead;
        pQH->pMetaData->pPrev = m_pCurrentQueueTail;

        QH *pOldTail = m_pCurrentQueueTail;

        {
            // Atomic operation - modifying both the housekeeping and the
            // hardware linked lists
            LockGuard<Spinlock> guard(m_QueueListChangeLock);

            // Update the tail pointer
            m_pCurrentQueueTail = pQH;

            // The current tail needs to point to this QH
            pOldTail->pNext = (m_pQHListPhys + (pTransaction * sizeof(QH))) >> 4;
            pOldTail->bNextInvalid = 0;
            pOldTail->bNextQH = 1;

            // Finally, fix the linked list
            pOldTail->pMetaData->pNext = pQH;
            pOldTail->pMetaData->pPrev = pQH;

            DEBUG_LOG("queued, waiting for IRQ");
        }
    }
    else
    {
        ERROR("UHCI: Queue tail is null!");
    }
}

/*
void Uhci::doAsync(UsbEndpoint endpointInfo, uint8_t nPid, uintptr_t pBuffer, uint16_t nBytes, void (*pCallback)(uintptr_t, ssize_t), uintptr_t pParam)
{
    LockGuard<Mutex> guard(m_Mutex);
    DEBUG_LOG("START "<<Dec<<endpointInfo.nAddress<<":"<<endpointInfo.nEndpoint<<" "<<endpointInfo.dumpSpeed()<<" "<<(nPid==UsbPidOut?" OUT ":(nPid==UsbPidIn?" IN  ":(nPid==UsbPidSetup?"SETUP":"")))<<" "<<nBytes<<Hex);

    // Pause the controller
    m_pBase->write16(0, UHCI_CMD);

    // Copy the buffer, if needed
    if(nPid != UsbPidIn && pBuffer)
        memcpy(m_pTransferPages, reinterpret_cast<void*>(pBuffer), nBytes);

    // Fill the TD with data
    TD *pTD = &m_pTDList[0];
    memset(pTD, 0, sizeof(TD));
    pTD->pNext = m_pAsyncQH->pCurrent?m_pAsyncQH->pCurrent->pPhysAddr:0;
    pTD->bNextInvalid = m_pAsyncQH->pCurrent?0:1;
    pTD->nStatus = 0x80;
    pTD->bIoc = 1;
    pTD->bLoSpeed = endpointInfo.speed == LowSpeed;
    pTD->nErr = 1;
    pTD->bSpd = 0;
    pTD->nPid = nPid;
    pTD->nAddress = endpointInfo.nAddress;
    pTD->nEndpoint = endpointInfo.nEndpoint;
    pTD->bDataToggle = endpointInfo.bDataToggle;
    pTD->nMaxLen = nBytes?nBytes-1:0x7ff;
    pTD->pBuff = m_pTransferPagesPhys;
    pTD->pPhysAddr = (m_pTDListPhys+0*sizeof(TD))>>4;
    pTD->pCallback = reinterpret_cast<uintptr_t>(pCallback);
    pTD->pParam = pParam;
    pTD->pBuffer = pBuffer;

    // Change our async QH to include the new TD
    m_pAsyncQH->pElem = pTD->pPhysAddr;
    m_pAsyncQH->bElemInvalid = 0;
    m_pAsyncQH->pCurrent = pTD;

    // Resume the controller
    m_pBase->write16(UHCI_CMD_RUN, UHCI_CMD);
}


void Uhci::addInterruptInHandler(uint8_t nAddress, uint8_t nEndpoint, uintptr_t pBuffer, uint16_t nBytes, void (*pCallback)(uintptr_t, ssize_t), uintptr_t pParam)
{
    LockGuard<Mutex> guard(m_Mutex);

    // Pause the controller
    m_pBase->write16(0, UHCI_CMD);

    // Fill the TD with data
    TD *pTD = &m_pTDList[1];
    memset(pTD, 0, sizeof(TD));
    pTD->pPhysAddr = (m_pTDListPhys+1*sizeof(TD))>>4;
    pTD->pNext = m_pPeriodicQH->pCurrent?m_pPeriodicQH->pCurrent->pNext:pTD->pPhysAddr;
    pTD->nStatus = 0x80;
    pTD->bIoc = 1;
    pTD->bLoSpeed = 1;
    pTD->bSpd = 0;
    pTD->nPid = UsbPidIn;
    pTD->nAddress = nAddress;
    pTD->nEndpoint = nEndpoint;
    pTD->bDataToggle = 0;
    pTD->nMaxLen = nBytes?nBytes-1:0x7ff;
    pTD->pBuff = m_pTransferPagesPhys;
    pTD->pCallback = reinterpret_cast<uintptr_t>(pCallback);
    pTD->pParam = pParam;
    pTD->pBuffer = pBuffer;

    // Change our periodic QH to include the new TD
    if(m_pPeriodicQH->pCurrent)
        m_pPeriodicQH->pCurrent->pNext = pTD->pPhysAddr;
    else
    {
        m_pPeriodicQH->pElem = pTD->pPhysAddr;
        m_pPeriodicQH->bElemInvalid = 0;
    }
    m_pPeriodicQH->pCurrent = pTD;

    // Resume the controller
    m_pBase->write16(UHCI_CMD_RUN, UHCI_CMD);
}*/

