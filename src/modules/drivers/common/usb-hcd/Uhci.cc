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

#define INDEX_FROM_TD_VIRT(ptr) (((reinterpret_cast<uintptr_t>((ptr)) - reinterpret_cast<uintptr_t>(m_pTDList)) / sizeof(TD)))
#define INDEX_FROM_TD_PHYS(ptr) ((((ptr) - m_pTDListPhys) / sizeof(TD)))
#define PHYS_TD(idx)        (m_pTDListPhys + ((idx) * sizeof(TD)))

#define QH_REGION_SIZE      0x4000
#define TD_REGION_SIZE      0x8000

#define TOTAL_MEM_PAGES     ((QH_REGION_SIZE / 0x1000) + (TD_REGION_SIZE / 0x1000) + 1)

Uhci::Uhci(Device* pDev) :
    Device(pDev), m_pBase(0), m_nPorts(0), m_AsyncQueueListChangeLock(), m_UhciMR("Uhci-MR"),
    m_pCurrentAsyncQueueTail(0), m_pCurrentAsyncQueueHead(0), m_nPortCheckTicks(0)
{
    setSpecificType(String("UHCI"));

    // Allocate the memory region
    if(!PhysicalMemoryManager::instance().allocateRegion(m_UhciMR, TOTAL_MEM_PAGES, PhysicalMemoryManager::continuous, VirtualAddressSpace::Write | VirtualAddressSpace::KernelMode))
    {
        ERROR("USB: UHCI: Couldn't allocate memory region!");
        return;
    }

    uintptr_t pVirtBase = reinterpret_cast<uintptr_t>(m_UhciMR.virtualAddress());
    physical_uintptr_t pPhysBase = m_UhciMR.physicalAddress();

    m_pFrameList        = reinterpret_cast<uint32_t*>(pVirtBase);
    m_pQHList           = reinterpret_cast<QH*>(pVirtBase + 0x1000);
    m_pTDList           = reinterpret_cast<TD*>(pVirtBase + 0x1000 + QH_REGION_SIZE);

    m_pFrameListPhys    = pPhysBase;
    m_pQHListPhys       = pPhysBase + 0x1000;
    m_pTDListPhys       = m_pQHListPhys + QH_REGION_SIZE;

    // Allocate room for the Dummy QH
    m_QHBitmap.set(0);
    QH *pDummyQH = &m_pQHList[0];

    // Set all frame list entries to the dummy QH
    dmemset(m_pFrameList, m_pQHListPhys | 2, 0x400);

    memset(pDummyQH, 0, sizeof(QH));

    pDummyQH->pNext = m_pQHListPhys >> 4;
    pDummyQH->bNextQH = 1;
    pDummyQH->bElemInvalid = 1;

    pDummyQH->pMetaData = new QH::MetaData;
    pDummyQH->pMetaData->pPrev = pDummyQH->pMetaData->pNext = pDummyQH;

    m_pCurrentAsyncQueueTail = m_pCurrentAsyncQueueHead = pDummyQH;

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
    Machine::instance().getIrqManager()->registerPciIrqHandler(this, this);
    Machine::instance().getIrqManager()->control(getInterruptNumber(), IrqManager::MitigationThreshold, (12 * 1024 / 8 / 64)); // 12KB/ms (12Mbps) in bytes, divided by 64 bytes maximum per transfer/IRQ

    // Set up the RequestQueue
    initialise();

#ifdef USB_VERBOSE_DEBUG
    DEBUG_LOG("USB: UHCI: Reset complete");
#endif

    for(size_t i = 0; i < 7; i++)
    {
        uint16_t nPortStatus = m_pBase->read16(UHCI_PORTSC + i * 2);

        // Check if this is a valid port
        if(nPortStatus == 0xffff || !(nPortStatus & 0x80))
            break;

        // Check for a connected device
        if(nPortStatus & UHCI_PORTSC_CONN)
            executeRequest(i);

        m_nPorts++;
    }

    // Install the timer handler for the periodic port checks
    Machine::instance().getTimer()->registerHandler(this);
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

    // Stop the controller while we dequeue
    m_pBase->write16(m_pBase->read16(UHCI_CMD) & ~1, UHCI_CMD);
    while(!(m_pBase->read16(UHCI_STS) & 0x20)) delay(5);

    for(size_t i = 1; i < (QH_REGION_SIZE / sizeof(QH)); i++)
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
        if(!(pQH->pMetaData->pNext && pQH->pMetaData->pPrev))
        {
#ifdef USB_VERBOSE_DEBUG
            DEBUG_LOG("Not performing dequeue on QH #" << Dec << i << Hex << " as it's not yet linked.");
#endif
            continue;
        }
        if(!pQH->pMetaData->pFirstTD)
        {
#ifdef USB_VERBOSE_DEBUG
            DEBUG_LOG("Not performing dequeue on QH #" << Dec << i << Hex << " as it has no TDs yet.");
#endif
            continue;
        }

        // Remove all TDs
        size_t nTDIndex = INDEX_FROM_TD_VIRT(pQH->pMetaData->pFirstTD);
        while(true)
        {
            size_t thisIndex = nTDIndex;

            TD *pTD = &m_pTDList[nTDIndex];
            bool shouldBreak = pTD->bNextInvalid || !pTD->pNext;
            if(!shouldBreak)
            {
                size_t nOldTDIndex = nTDIndex;
                nTDIndex = INDEX_FROM_TD_PHYS(pTD->pNext << 4);
            }

            memset(pTD, 0, sizeof(TD));

            m_TDBitmap.clear(thisIndex);

            if(shouldBreak)
                break;
        }

        // This QH is done
        m_QHBitmap.clear(i);

        // Completely invalidate the QH
        delete pQH->pMetaData;
        memset(pQH, 0, sizeof(QH));

#ifdef USB_VERBOSE_DEBUG
        DEBUG_LOG("Dequeue for QH #" << Dec << i << Hex << ".");
#endif
    }

    // Resume the controller, dequeue done.
    m_pBase->write16(m_pBase->read16(UHCI_CMD) | 1, UHCI_CMD);
    while(m_pBase->read16(UHCI_STS) & 0x20) delay(5);
}

bool Uhci::irq(irq_id_t number, InterruptState &state)
{
    uint16_t nStatus = m_pBase->read16(UHCI_STS);
    m_pBase->write16(nStatus, UHCI_STS);

#ifdef USB_VERBOSE_DEBUG
    DEBUG_LOG("UHCI IRQ " << nStatus);
#endif

    bool bDequeue = false;
    if(nStatus & UHCI_STS_INT)
    {
        // Check all the queue heads we have in the list
        // QH *pQH = m_pCurrentAsyncQueueHead->pMetaData->pNext;
        // while(

        for(size_t i = 1; i < (QH_REGION_SIZE / sizeof(QH)); i++)
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
            if(!pQH->pMetaData->pFirstTD)
            {
                WARNING_NOLOCK("UHCI: QH #" << Dec << i << Hex << " hasn't had any transactions added to it yet.");
                continue;
            }

            bool bPeriodic = pQH->pMetaData->bPeriodic;

            // Iterate the TD list
            size_t nTDIndex = INDEX_FROM_TD_VIRT(pQH->pMetaData->pFirstTD);
            while(true)
            {
                TD *pTD = &m_pTDList[nTDIndex];

                // If we've already detected that this TD was a short transfer,
                // don't process any more TDs
                if(pTD->bShortTransferTD)
                    break;

                bool bEndOfTransfer = false;
                if(pTD->nStatus != 0x80)
                {
                    ssize_t nResult;
                    if((pTD->nStatus & 0x7e) || (nStatus & UHCI_STS_ERR))
                    {
#ifdef USB_VERBOSE_DEBUG
                        ERROR_NOLOCK(((nStatus & UHCI_STS_ERR) ? "USB" : "TD") << " ERROR!");
                        ERROR_NOLOCK("TD Status: " << pTD->nStatus);
#endif
                        nResult = - pTD->getError();
                    }
                    else
                    {
                        nResult = (pTD->nActLen + 1) % 0x800;
                        pQH->pMetaData->nTotalBytes += nResult;
                    }
#ifdef USB_VERBOSE_DEBUG
                    DEBUG_LOG_NOLOCK("TD #" << Dec << nTDIndex << Hex << " [from QH #" << Dec << i << Hex << "] DONE: " << Dec << pTD->nAddress << ":" << pTD->nEndpoint << " " << (pTD->nPid==UsbPidOut?"OUT":(pTD->nPid==UsbPidIn?"IN":(pTD->nPid==UsbPidSetup?"SETUP":""))) << " " << nResult << Hex);
#endif

                    // Handle the "end of transfer" cases
                    bEndOfTransfer = (!bPeriodic && ((nResult < 0) || (pTD == pQH->pMetaData->pLastTD))) || (bPeriodic && !nResult >= 0);

                    // Some extra cases we need to handle
                    if((pTD != pQH->pMetaData->pLastTD) && !bEndOfTransfer)
                    {
                        // Control endpoints are irrelevant here
                        if(pTD->nEndpoint)
                        {
                            if((nResult >= 0) && (pTD->nPid == UsbPidIn))
                            {
                                // Check for a short read. There is no point continuing
                                // to read from the device if it's sent us less than we
                                // asked for before the last TD.
                                // This stops the transfer hanging on IN transactions
                                // that return less data than expected.
                                if(nResult < pTD->nBufferSize)
                                {
                                    pTD->bShortTransferTD = true;
                                    bEndOfTransfer = true;
                                }
                            }
                        }
                    }

                    // Last TD or error condition, if async, otherwise only when it gives no error
                    if(bEndOfTransfer)
                    {
                        // Valid callback?
                        if(pQH->pMetaData->pCallback)
                            pQH->pMetaData->pCallback(pQH->pMetaData->pParam, nResult < 0 ? nResult : pQH->pMetaData->nTotalBytes);

                        if(!bPeriodic)
                        {
                            // This queue head is done, dequeue.
                            QH *pPrev = pQH->pMetaData->pPrev;
                            QH *pNext = pQH->pMetaData->pNext;

                            // Main non-hardware linked list update
                            m_AsyncQueueListChangeLock.acquire(); // Atomic operation
                            pPrev->pMetaData->pNext = pNext;
                            pNext->pMetaData->pPrev = pPrev;

                            // Hardware linked list update
                            pPrev->pNext = pQH->pNext;
                            pPrev->bNextQH = 1;
                            pPrev->bNextInvalid = 0;

                            // Update the tail pointer if we need to
                            if(pQH == m_pCurrentAsyncQueueTail)
                                m_pCurrentAsyncQueueTail = pPrev;

                            m_AsyncQueueListChangeLock.release();

                            bDequeue = true;

                            // Ignore after the linked list update to avoid a
                            // case where the dequeue call hits this QH while
                            // we unlink it. This is a problem as the dequeue
                            // call zeroes all TDs and the QH itself. Ouch!
                            pQH->pMetaData->bIgnore = true;
                        }
                        else
                        {
                            // Invert data toggle
                            pTD->bDataToggle = !pTD->bDataToggle;

                            // Clear the total bytes field so it won't grow with each completed transfer
                            pQH->pMetaData->nTotalBytes = 0;
                        }
                    }

                    // Interrupt TDs need to be always active
                    if(bPeriodic)
                    {
                        pQH->pMetaData->bIgnore = false;
                        pTD->nStatus = 0x80;
                        pTD->nActLen = 0;

                        // Modified by the host controller
                        pQH->pElem = PHYS_TD(nTDIndex) >> 4;
                        pQH->bElemInvalid = 0;
                        pQH->bElemQH = 0;
                    }
                }

                size_t oldIndex = nTDIndex;

                if(pTD->bNextInvalid || bEndOfTransfer)
                    break;
                else
                    nTDIndex = INDEX_FROM_TD_PHYS(pTD->pNext << 4);

                if(nTDIndex == oldIndex)
                {
                    ERROR_NOLOCK("UHCI: QH #" << Dec << i << Hex << "'s TD list is invalid - circular reference!");
                    break;
                }
                else if(pTD->pNext == 0)
                {
                    ERROR_NOLOCK("UHCI: QH #" << Dec << i << Hex << "'s TD list is invalid - null pNext pointer (and T bit not set)!");
                    break;
                }
                else if(nTDIndex > (TD_REGION_SIZE / sizeof(TD)))
                {
                    ERROR_NOLOCK("UHCI: QH #" << Dec << i << Hex << " has a TD pointing to a TD that is outside the TD region.");
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
        nIndex = m_TDBitmap.getFirstClear();
        if(nIndex >= (TD_REGION_SIZE / sizeof(TD)))
        {
            ERROR("USB: UHCI: TD space full");
            return;
        }
        m_TDBitmap.set(nIndex);
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
        if(va.isMapped(reinterpret_cast<void*>(pBuffer)))
        {
            physical_uintptr_t phys = 0; size_t flags = 0;
            va.getMapping(reinterpret_cast<void*>(pBuffer), phys, flags);
            pTD->pBuff = phys + (pBuffer & 0xFFF);
        }
        else
        {
            ERROR("UHCI: addTransferToTransaction: Buffer (page " << Dec << pBuffer << Hex << ") isn't mapped!");
            m_TDBitmap.clear(nIndex);
            return;
        }
    }

    // Link into the existing TD list
    if(pQH->pMetaData->pLastTD)
    {
        pQH->pMetaData->pLastTD->pNext = PHYS_TD(nIndex) >> 4;
        pQH->pMetaData->pLastTD->bNextInvalid = 0;
        pQH->pMetaData->pLastTD->bNextQH = 0;
        pQH->pMetaData->pLastTD->bNextDepth = 1;
    }
    else
    {
        pQH->pMetaData->pFirstTD = pTD;
        pQH->pElem = PHYS_TD(nIndex) >> 4;
        pQH->bElemInvalid = 0;
        pQH->bElemQH = 0;
    }
    pQH->pMetaData->pLastTD = pTD;
}

uintptr_t Uhci::createTransaction(UsbEndpoint endpointInfo)
{
    // Atomic operation: find clear bit, set it
    size_t nIndex = 0;
    {
        LockGuard<Mutex> guard(m_Mutex);
        nIndex = m_QHBitmap.getFirstClear();
        if(nIndex >= (QH_REGION_SIZE / sizeof(QH)))
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
    if(m_pCurrentAsyncQueueTail)
    {
        // Current QH needs to point to the schedule's head
        size_t queueHeadIndex = (reinterpret_cast<uintptr_t>(m_pCurrentAsyncQueueHead) - reinterpret_cast<uintptr_t>(m_pQHList)) / sizeof(QH);
        pQH->pNext = (m_pQHListPhys + (queueHeadIndex * sizeof(QH))) >> 4;
        pQH->bNextInvalid = 0;
        pQH->bNextQH = 1;

        {
            // Atomic operation - modifying both the housekeeping and the
            // hardware linked lists
            LockGuard<Spinlock> guard(m_AsyncQueueListChangeLock);

            QH *pOldTail = m_pCurrentAsyncQueueTail;

            // Update the tail pointer
            m_pCurrentAsyncQueueTail = pQH;

            // The current tail needs to point to this QH
            pOldTail->pNext = (m_pQHListPhys + (pTransaction * sizeof(QH))) >> 4;
            pOldTail->bNextInvalid = 0;
            pOldTail->bNextQH = 1;

            // Finally, fix the linked list
            pOldTail->pMetaData->pNext = pQH;

            // Enter the information for correct dequeue
            pQH->pMetaData->pNext = m_pCurrentAsyncQueueHead;
            pQH->pMetaData->pPrev = pOldTail;
            m_pCurrentAsyncQueueHead->pMetaData->pPrev = pQH;
        }
    }
    else
    {
        ERROR("UHCI: Queue tail is null!");
    }
}

void Uhci::addInterruptInHandler(UsbEndpoint endpointInfo, uintptr_t pBuffer, uint16_t nBytes, void (*pCallback)(uintptr_t, ssize_t), uintptr_t pParam)
{
    // Create a new transaction
    uintptr_t nTransaction = createTransaction(endpointInfo);

    // Get the QH and set the periodic flag
    QH *pQH = &m_pQHList[nTransaction];
    pQH->pMetaData->bPeriodic = true;

    // Add a single transfer to the transaction
    addTransferToTransaction(nTransaction, false, UsbPidIn, pBuffer, nBytes);

    // Get the TD and set the error counter to "unlimited retries"
    TD *pTD = pQH->pMetaData->pLastTD;
    pTD->nErr = 0;

    // Let doAsync do the rest
    doAsync(nTransaction, pCallback, pParam);
}

uint64_t Uhci::executeRequest(uint64_t p1, uint64_t p2, uint64_t p3, uint64_t p4, uint64_t p5,
                              uint64_t p6, uint64_t p7, uint64_t p8)
{
    // Check for a connected device
    if(m_pBase->read16(UHCI_PORTSC + p1 * 2) & UHCI_PORTSC_CONN)
    {
        // Perform a reset of the port
        m_pBase->write16(UHCI_PORTSC_PRES | UHCI_PORTSC_CSCH, UHCI_PORTSC + p1 * 2);
        delay(50);
        m_pBase->write16(0, UHCI_PORTSC + p1 * 2);
        delay(50);

        // Enable the port
        m_pBase->write16(UHCI_PORTSC_ENABLE, UHCI_PORTSC + p1 * 2);
        delay(50);
        m_pBase->write16(UHCI_PORTSC_ENABLE, UHCI_PORTSC + p1 * 2);

        // Determine the speed of the attached device
        if(m_pBase->read16(UHCI_PORTSC + p1 * 2) & UHCI_PORTSC_LOSPEED)
        {
            DEBUG_LOG("USB: UHCI: Port " << Dec << p1 << Hex << " has a low-speed device connected to it");
            deviceConnected(p1, LowSpeed);
        }
        else
        {
            DEBUG_LOG("USB: UHCI: Port " << Dec << p1 << Hex << " has a full-speed device connected to it");
            deviceConnected(p1, FullSpeed);
        }
    }
    else
        deviceDisconnected(p1);
    return 0;
}

void Uhci::timer(uint64_t delta, InterruptState &state)
{
    m_nPortCheckTicks += delta;

    // We check the ports once in a millisecond
    if(m_nPortCheckTicks >= 1000000)
    {
        // Reset the counter
        m_nPortCheckTicks = 0;

        // Check every port for a change
        for(size_t i = 0; i < m_nPorts; i++)
        {
            if(m_pBase->read16(UHCI_PORTSC + i * 2) & UHCI_PORTSC_CSCH)
            {
                // Clear the port status change
                m_pBase->write16(UHCI_PORTSC_CSCH, UHCI_PORTSC + i * 2);

                // Now we can safely add the request
                addAsyncRequest(0, i);
            }
        }
    }
}

