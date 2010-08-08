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
#ifdef X86_COMMON
#include <machine/Pci.h>
#endif
#include <processor/Processor.h>
#include <processor/InterruptManager.h>
#include <usb/Usb.h>
#include <utilities/assert.h>
#include <Log.h>
#include "Ehci.h"

#define delay(n) do{Semaphore semWAIT(0);semWAIT.acquire(1, 0, n * 1000);}while(0)

#define INDEX_FROM_QTD(ptr) (((reinterpret_cast<uintptr_t>((ptr)) & 0xFFF) / sizeof(qTD)))
#define PHYS_QTD(idx)        (m_pqTDListPhys + ((idx) * sizeof(qTD)))

#define GET_PAGE(param, page, qhIndex) \
do \
{ \
    if((nBufferPageOffset + nBytes) > ((page) * 0x1000)) \
    { \
        if(va.isMapped(reinterpret_cast<void*>(pBufferPageStart + (page) * 0x1000))) \
        { \
            physical_uintptr_t phys = 0; size_t flags = 0; \
            va.getMapping(reinterpret_cast<void*>(pBufferPageStart + (page) * 0x1000), phys, flags); \
            (param) = phys >> 12; \
        } \
        else \
        { \
            ERROR("EHCI: addTransferToTransaction: Buffer (page " << Dec << (page) << Hex << ") isn't mapped!"); \
            m_QHBitmap.clear((qhIndex)); \
            return; \
        } \
    } \
} while(0)

Ehci::Ehci(Device* pDev) : Device(pDev), m_pCurrentQueueTail(0), m_pCurrentQueueHead(0), m_EhciMR("Ehci-MR")
{
    setSpecificType(String("EHCI"));

    // Allocate the pages we need
    if(!PhysicalMemoryManager::instance().allocateRegion(m_EhciMR, 4, PhysicalMemoryManager::continuous, VirtualAddressSpace::KernelMode | VirtualAddressSpace::Write))
    {
        ERROR("USB: EHCI: Couldn't allocate Memory Region!");
        return;
    }

    uintptr_t virtualBase   = reinterpret_cast<uintptr_t>(m_EhciMR.virtualAddress());
    uintptr_t physicalBase  = m_EhciMR.physicalAddress();
    m_pQHList               = reinterpret_cast<QH*>(virtualBase);
    m_pFrameList            = reinterpret_cast<uint32_t*>(virtualBase + 0x2000);
    m_pqTDList              = reinterpret_cast<qTD*>(virtualBase + 0x3000);
    m_pQHListPhys           = physicalBase;
    m_pFrameListPhys        = physicalBase + 0x2000;
    m_pqTDListPhys          = physicalBase + 0x3000;

    dmemset(m_pFrameList, 1, 0x400);

#ifdef X86_COMMON
    uint32_t nPciCmdSts = PciBus::instance().readConfigSpace(this, 1);
#ifdef USB_VERBOSE_DEBUG
    DEBUG_LOG("USB: EHCI: Pci command: " << (nPciCmdSts & 0xffff));
#endif
    PciBus::instance().writeConfigSpace(this, 1, nPciCmdSts | 0x4);
#endif

    // Grab the ports
    m_pBase = m_Addresses[0]->m_Io;
    m_nOpRegsOffset = m_pBase->read8(EHCI_CAPLENGTH);

    // Get structural capabilities to determine the number of physical ports
    // we have available to us.
    m_nPorts = m_pBase->read8(EHCI_HCSPARAMS) & 0xF;

    // Don't reset a running controller, make sure it's paused
    m_pBase->write32(m_pBase->read32(m_nOpRegsOffset + EHCI_CMD) & ~EHCI_CMD_RUN, m_nOpRegsOffset + EHCI_CMD);
    while(!(m_pBase->read32(m_nOpRegsOffset + EHCI_STS) & EHCI_STS_HALTED))
        delay(5);

    // Write reset command and wait for it to complete
    m_pBase->write32(EHCI_CMD_HCRES, m_nOpRegsOffset + EHCI_CMD);
    while(m_pBase->read32(m_nOpRegsOffset + EHCI_CMD) & EHCI_CMD_HCRES)
        delay(5);
#ifdef USB_VERBOSE_DEBUG
    DEBUG_LOG("USB: EHCI: Reset complete, status: " << m_pBase->read32(m_nOpRegsOffset + EHCI_STS) << ".");
#endif

    // Install the IRQ
#ifdef X86_COMMON
    Machine::instance().getIrqManager()->registerPciIrqHandler(this, this);
#else
    InterruptManager::instance().registerInterruptHandler(getInterruptNumber(), this);
#endif

    // Zero the top 64 bits for addresses of EHCI data structures
    m_pBase->write32(0, m_nOpRegsOffset + EHCI_CTRLDSEG);

    // Enable interrupts
    m_pBase->write32(0x3b, m_nOpRegsOffset + EHCI_INTR);

    // Write the base address of the periodic frame list - all T-bits are set to one
    m_pBase->write32(m_pFrameListPhys, m_nOpRegsOffset + EHCI_PERIODICLP);

    delay(5);

    // Create a dummy QH and qTD
    m_QHBitmap.set(0); m_qTDBitmap.set(0);
    QH *pDummyQH = &m_pQHList[0];
    qTD *pDummyTD = &m_pqTDList[0];
    memset(pDummyQH, 0, sizeof(QH));
    memset(pDummyTD, 0, sizeof(qTD));

    // Configure the dummy TD
    pDummyTD->bNextInvalid = pDummyTD->bAltNextInvalid = 1;

    // Configure the dummy QH
    pDummyQH->pNext = m_pQHListPhys >> 5;
    pDummyQH->nNextType = 1;

    pDummyQH->pQTD = m_pqTDListPhys >> 5;
    pDummyQH->mult = 1;
    pDummyQH->hrcl = 1;

    pDummyQH->pMetaData = new QH::MetaData;
    memset(pDummyQH->pMetaData, 0, sizeof(QH::MetaData));
    pDummyQH->pMetaData->pFirstQTD = 0;
    pDummyQH->pMetaData->pLastQTD = pDummyTD;
    pDummyQH->pMetaData->pNext = pDummyQH;
    pDummyQH->pMetaData->pPrev = pDummyQH;

    memcpy(&pDummyQH->overlay, pDummyTD, sizeof(qTD));

    m_pCurrentQueueHead = m_pCurrentQueueTail = pDummyQH;

    // Disable the asynchronous schedule, and wait for it to become disabled
    m_pBase->write32(m_pBase->read32(m_nOpRegsOffset + EHCI_CMD) & ~EHCI_CMD_ASYNCLE, m_nOpRegsOffset + EHCI_CMD);
    while(m_pBase->read32(m_nOpRegsOffset + EHCI_STS) & 0x8000);

    // Write the async list head pointer
    m_pBase->write32(m_pQHListPhys, m_nOpRegsOffset + EHCI_ASYNCLP);

    // Turn on the controller
    m_pBase->write32(m_pBase->read32(m_nOpRegsOffset + EHCI_CMD) | EHCI_CMD_RUN, m_nOpRegsOffset + EHCI_CMD);
    while(m_pBase->read32(m_nOpRegsOffset + EHCI_STS) & EHCI_STS_HALTED)
        delay(5);

    // Enable the asynchronous schedule, and wait for it to become enabled
    m_pBase->write32(m_pBase->read32(m_nOpRegsOffset + EHCI_CMD) | EHCI_CMD_ASYNCLE, m_nOpRegsOffset + EHCI_CMD);
    while(!(m_pBase->read32(m_nOpRegsOffset + EHCI_STS) & 0x8000));

    // Set the desired interrupt threshold (frame list size = 4096 bytes)
    m_pBase->write32((m_pBase->read32(m_nOpRegsOffset + EHCI_CMD) & ~0xFF0000) | 0x80000, m_nOpRegsOffset + EHCI_CMD);

    // Take over the ports
    m_pBase->write32(1, m_nOpRegsOffset + EHCI_CFGFLAG);

    // Set up the RequestQueue
    initialise();

    // Search for ports with devices and initialise them
    for(size_t i = 0; i < m_nPorts; i++)
    {
#ifdef USB_VERBOSE_DEBUG
        DEBUG_LOG("USB: EHCI: Port " << Dec << i << Hex << " - status initially: " << m_pBase->read32(m_nOpRegsOffset+EHCI_PORTSC+i*4));
#endif
        if(!(m_pBase->read32(m_nOpRegsOffset + EHCI_PORTSC + i * 4) & EHCI_PORTSC_PPOW))
        {
            m_pBase->write32(EHCI_PORTSC_PPOW, m_nOpRegsOffset + EHCI_PORTSC + i * 4);
            delay(20);
#ifdef USB_VERBOSE_DEBUG
            DEBUG_LOG("USB: EHCI: Port " << Dec << i << Hex << " - status after power-up: " << m_pBase->read32(m_nOpRegsOffset + EHCI_PORTSC + i * 4));
#endif
        }

        // If connected, send it to the RequestQueue
        if(m_pBase->read32(m_nOpRegsOffset+EHCI_PORTSC+i*4) & EHCI_PORTSC_CONN)
            executeRequest(i);
        else
            m_pBase->write32(m_pBase->read32(m_nOpRegsOffset + EHCI_PORTSC + i * 4), m_nOpRegsOffset + EHCI_PORTSC + i * 4);
    }

    // Enable port status change interrupt and clear it from status
    m_pBase->write32(EHCI_STS_PORTCH, m_nOpRegsOffset + EHCI_STS);
    m_pBase->write32(0x3f, m_nOpRegsOffset + EHCI_INTR);
}

Ehci::~Ehci()
{
}

static int threadStub(void *p)
{
    Ehci *pEhci = reinterpret_cast<Ehci*>(p);
    pEhci->doDequeue();
    return 0;
}

void Ehci::doDequeue()
{
    // Absolutely cannot have queue insetions during a dequeue
    LockGuard<Mutex> guard(m_Mutex);

    for(size_t i = 1; i < 0x2000 / sizeof(QH); i++)
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

        // Remove all qTDs
        size_t nQTDIndex = INDEX_FROM_QTD(pQH->pMetaData->pFirstQTD);
        while(true)
        {
            m_qTDBitmap.clear(nQTDIndex);

            qTD *pqTD = &m_pqTDList[nQTDIndex];
            bool shouldBreak = pqTD->bNextInvalid;
            if(!shouldBreak)
                nQTDIndex = ((pqTD->pNext << 5) & 0xFFF) / sizeof(qTD);

            memset(pqTD, 0, sizeof(qTD));

            if(shouldBreak)
                break;
        }

        // Completely invalidate the QH
        delete pQH->pMetaData;
        memset(pQH, 0, sizeof(QH));

#ifdef USB_VERBOSE_DEBUG
        DEBUG_LOG("Dequeue for QH #" << Dec << i << Hex << ".");
#endif

        // This QH is done
        m_QHBitmap.clear(i);
    }
}

#ifdef X86_COMMON
bool Ehci::irq(irq_id_t number, InterruptState &state)
#else
void Ehci::interrupt(size_t number, InterruptState &state)
#endif
{
    uint32_t nStatus = m_pBase->read32(m_nOpRegsOffset + EHCI_STS) & m_pBase->read32(m_nOpRegsOffset + EHCI_INTR);
#ifdef USB_VERBOSE_DEBUG
    DEBUG_LOG_NOLOCK("EHCI IRQ " << nStatus);
#endif
    if(nStatus & EHCI_STS_PORTCH)
        for(size_t i = 0; i < m_nPorts; i++)
            if(m_pBase->read32(m_nOpRegsOffset + EHCI_PORTSC + i * 4) & EHCI_PORTSC_CSCH)
                addAsyncRequest(0, i);

    if(nStatus & EHCI_STS_INT)
    {
        for(size_t i = 1; i < 128; i++)
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

            size_t nQTDIndex = INDEX_FROM_QTD(pQH->pMetaData->pFirstQTD);
            while(true)
            {
                qTD *pqTD = &m_pqTDList[nQTDIndex];

                if(pqTD->nStatus != 0x80)
                {
                    ssize_t nResult;
                    if((pqTD->nStatus & 0x7c) || (nStatus & EHCI_STS_ERR))
                    {
#ifdef USB_VERBOSE_DEBUG
                        ERROR_NOLOCK(((nStatus & EHCI_STS_ERR) ? "USB" : "qTD") << " ERROR!");
                        ERROR_NOLOCK("qTD Status: " << pqTD->nStatus << " [overlay status=" << pQH->overlay.nStatus << "]");
                        ERROR_NOLOCK("qTD Error Counter: " << pqTD->nErr << " [overlay counter=" << pQH->overlay.nErr << "]");
                        ERROR_NOLOCK("QH NAK counter: " << pqTD->res1 << " [overlay count=" << pQH->overlay.res1 << "]");
                        ERROR_NOLOCK("qTD PID: " << pqTD->nPid << ".");
#endif
                        nResult = - pqTD->getError();
                    }
                    else
                    {
                        nResult = pqTD->nBufferSize - pqTD->nBytes;
                        pQH->pMetaData->nTotalBytes += nResult;
                    }
#ifdef USB_VERBOSE_DEBUG
                    DEBUG_LOG_NOLOCK("qTD #" << Dec << nQTDIndex << Hex << " [from QH #" << Dec << i << Hex << "] DONE: " << Dec << pQH->nAddress << ":" << pQH->nEndpoint << " " << (pqTD->nPid==0?"OUT":(pqTD->nPid==1?"IN":(pqTD->nPid==2?"SETUP":""))) << " " << nResult << Hex);
#endif

                    // Last qTD or error condition?
                    if((nResult < 0) || (pqTD == pQH->pMetaData->pLastQTD))
                    {
                        // Valid callback?
                        if(pQH->pMetaData->pCallback)
                            pQH->pMetaData->pCallback(pQH->pMetaData->pParam, nResult < 0 ? nResult : pQH->pMetaData->nTotalBytes);

                        if(!bPeriodic)
                        {
                            pQH->pMetaData->bIgnore = true;

                            // Was the reclaim head bit set?
                            if(pQH->hrcl)
                                pQH->pMetaData->pNext->hrcl = 1; // Make sure there's always a reclaim head

                            // This queue head is done, dequeue.
                            QH *pPrev = pQH->pMetaData->pPrev;
                            QH *pNext = pQH->pMetaData->pNext;

                            // Main non-hardware linked list update
                            pPrev->pMetaData->pNext = pNext;
                            pNext->pMetaData->pPrev = pPrev;

                            // Hardware linked list update
                            pPrev->pNext = pQH->pNext;

                            // Update the tail pointer if we need to
                            if(pQH == m_pCurrentQueueTail)
                            {
                                m_QueueListChangeLock.acquire(); // Atomic operation
                                m_pCurrentQueueTail = pPrev;
                                m_QueueListChangeLock.release();
                            }

                            // Interrupt on Async Advance Doorbell - will run the dequeue thread to
                            // clear bits in the QH and qTD bitmaps
                            m_pBase->write32(m_pBase->read32(m_nOpRegsOffset + EHCI_CMD) | (1 << 6), m_nOpRegsOffset + EHCI_CMD);
                        }
                    }
                    // Interrupt qTDs need constant refresh
                    if(bPeriodic)
                    {
                        pqTD->nStatus = 0x80;
                        pqTD->nBytes = pqTD->nBufferSize;
                        pqTD->nPage = 0;
                        //pqTD->nOffset = pQH->pMetaData->nBufferOffset%0x1000;
                        pqTD->nErr = 0;
                        //pqTD->pPage0 = m_pTransferPagesPhys>>12;
                        //pqTD->pPage1 = (m_pTransferPagesPhys + 0x1000)>>12;
                        //pqTD->pPage2 = (m_pTransferPagesPhys + 0x2000)>>12;
                        //pqTD->pPage3 = (m_pTransferPagesPhys + 0x3000)>>12;
                        //pqTD->pPage4 = (m_pTransferPagesPhys + 0x4000)>>12;
                        memcpy(&pQH->overlay, pqTD, sizeof(qTD));
                    }
                }

                size_t oldIndex = nQTDIndex;

                if(pqTD->bNextInvalid)
                    break;
                else
                    nQTDIndex = ((pqTD->pNext << 5) & 0xFFF) / sizeof(qTD);

                if(nQTDIndex == oldIndex)
                {
                    ERROR_NOLOCK("EHCI: QH #" << Dec << i << Hex << "'s qTD list is invalid - circular reference!");
                    break;
                }
                else if(pqTD->pNext == 0)
                {
                    ERROR_NOLOCK("EHCI: QH #" << Dec << i << Hex << "'s qTD list is invalid - null pNext pointer (and T bit not set)!");
                    break;
                }
            }
        }
    }

    if(nStatus & EHCI_STS_ASYNCADVANCE)
        new Thread(Processor::information().getCurrentThread()->getParent(), threadStub, reinterpret_cast<void*>(this));

    m_pBase->write32(nStatus, m_nOpRegsOffset + EHCI_STS);

#ifdef X86_COMMON
    return true;
#endif
}

void Ehci::addTransferToTransaction(uintptr_t nTransaction, bool bToggle, UsbPid pid, uintptr_t pBuffer, size_t nBytes)
{
    // Atomic operation: find clear bit, set it
    size_t nIndex = 0;
    {
        LockGuard<Mutex> guard(m_Mutex);
        nIndex = m_qTDBitmap.getFirstClear();
        if(nIndex >= (0x1000 / sizeof(qTD)))
        {
            ERROR("USB: EHCI: qTD space full");
            return;
        }
        m_qTDBitmap.set(nIndex);
    }

    // Grab the qTD pointer we're going to set up now
    qTD *pqTD = &m_pqTDList[nIndex];
    memset(pqTD, 0, sizeof(qTD));

    // There's nothing after us for now
    pqTD->bNextInvalid = 1;
    pqTD->bAltNextInvalid = 1;

    // PID for the transfer
    switch(pid)
    {
        case UsbPidOut:
            pqTD->nPid = 0;
            break;
        case UsbPidIn:
            pqTD->nPid = 1;
            break;
        case UsbPidSetup:
            pqTD->nPid = 2;
            break;
        default:
            pqTD->nPid = 3;
    };

    // Active, we want an interrupt on completion, and reset the error counter
    pqTD->nStatus = 0x80;
    pqTD->bIoc = 1;
    pqTD->nErr = 3; // Up to 3 retries of this transaction

    // Set up the transfer
    pqTD->nBytes = nBytes;
    pqTD->nBufferSize = nBytes;
    pqTD->bDataToggle = bToggle;

    if(nBytes)
    {
        // Configure transfer pages
        uintptr_t nBufferPageOffset = pBuffer & 0xFFF, pBufferPageStart = pBuffer & ~0xFFF;
        pqTD->nOffset = nBufferPageOffset;

        if(nBufferPageOffset + nBytes >= 0x5000)
        {
            ERROR("EHCI: addTransferToTransaction: Too many bytes for a single transaction!");
            return;
        }

        VirtualAddressSpace &va = Processor::information().getVirtualAddressSpace();
        GET_PAGE(pqTD->pPage0, 0, nIndex);
        GET_PAGE(pqTD->pPage1, 1, nIndex);
        GET_PAGE(pqTD->pPage2, 2, nIndex);
        GET_PAGE(pqTD->pPage3, 3, nIndex);
        GET_PAGE(pqTD->pPage4, 4, nIndex);
    }

    // Grab transaction's QH and add our qTD to it
    QH *pQH = &m_pQHList[nTransaction];
    if(pQH->pMetaData->pLastQTD)
    {
        pQH->pMetaData->pLastQTD->pNext = PHYS_QTD(nIndex) >> 5;
        pQH->pMetaData->pLastQTD->bNextInvalid = 0;

        if(pQH->pMetaData->pLastQTD == pQH->pMetaData->pFirstQTD)
        {
            pQH->overlay.pNext = pQH->pMetaData->pLastQTD->pNext;
            pQH->overlay.bNextInvalid = pQH->pMetaData->pLastQTD->bNextInvalid;
        }
    }
    else
    {
        pQH->pMetaData->pFirstQTD = pqTD;
        pQH->pQTD = PHYS_QTD(nIndex) >> 5;
        memcpy(&pQH->overlay, pqTD, sizeof(qTD));
    }
    pQH->pMetaData->pLastQTD = pqTD;
}

uintptr_t Ehci::createTransaction(UsbEndpoint endpointInfo)
{
    // Atomic operation: find clear bit, set it
    size_t nIndex = 0;
    {
        LockGuard<Mutex> guard(m_Mutex);
        nIndex = m_QHBitmap.getFirstClear();
        if(nIndex >= (0x2000 / sizeof(QH)))
        {
            ERROR("USB: EHCI: QH space full");
            return static_cast<uintptr_t>(-1);
        }
        m_QHBitmap.set(nIndex);
    }

    QH *pQH = &m_pQHList[nIndex];
    memset(pQH, 0, sizeof(QH));

    // The pointer to the next QH gets set in doAsync
    pQH->nNextType = 1;

    // NAK counter reload = 15
    pQH->nNakReload = 15;

    // Head of the reclaim list
    pQH->hrcl = true;

    // LS/FS handling
    pQH->nHubAddress = endpointInfo.speed != HighSpeed ? endpointInfo.nHubAddress : 0;
    pQH->nHubPort = endpointInfo.speed != HighSpeed ? endpointInfo.nHubPort : 0;
    pQH->bControlEndpoint = (endpointInfo.speed != HighSpeed) && !endpointInfo.nEndpoint;

    // Data toggle controlled by qTD
    pQH->bDataToggleSrc = 1;

    // Device address and speed
    pQH->nAddress = endpointInfo.nAddress;
    pQH->nSpeed = endpointInfo.speed;

    // Endpoint number and maximum packet size
    pQH->nEndpoint = endpointInfo.nEndpoint;
    pQH->nMaxPacketSize = endpointInfo.nMaxPacketSize;

    // Bandwidth multiplier - number of transactions that can be performed in a microframe
    pQH->mult = 1;

    // Setup the metadata
    QH::MetaData *pMetaData = new QH::MetaData();
    pMetaData->bPeriodic = false;
    pMetaData->pFirstQTD = 0;
    pMetaData->pLastQTD = 0;
    pMetaData->pNext = 0;
    pMetaData->pPrev = 0;
    pMetaData->bIgnore = false;

    pQH->pMetaData = pMetaData;

    // Complete
    return nIndex;
}

void Ehci::doAsync(uintptr_t nTransaction, void (*pCallback)(uintptr_t, ssize_t), uintptr_t pParam)
{
    LockGuard<Mutex> guard(m_Mutex);
    if((nTransaction == static_cast<uintptr_t>(-1)) || !m_QHBitmap.test(nTransaction))
    {
        ERROR("EHCI: doAsync: didn't get a valid transaction id [" << nTransaction << "].");
        return;
    }

    QH *pQH = &m_pQHList[nTransaction];
    pQH->pMetaData->pCallback = pCallback;
    pQH->pMetaData->pParam = pParam;
#ifdef USB_VERBOSE_DEBUG
    DEBUG_LOG("START #" << Dec << nTransaction << Hex << " " << Dec << pQH->nAddress << ":" << pQH->nEndpoint << Hex);
#endif

    // Link in to the asynchronous schedule
    if(m_pCurrentQueueTail)
    {
        // This QH is NOT the queue head. If we leave this set to one, and the
        // reclaim bit is set, the controller will think it's executed a full
        // circle, when in fact it's only partway there.
        pQH->hrcl = 0;

        // Current QH needs to point to the schedule's head
        size_t queueHeadIndex = (reinterpret_cast<uintptr_t>(m_pCurrentQueueHead) & 0xFFF) / sizeof(QH);
        pQH->pNext = (m_pQHListPhys + (queueHeadIndex * sizeof(QH))) >> 5;

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
            pOldTail->pNext = (m_pQHListPhys + (nTransaction * sizeof(QH))) >> 5;
            pOldTail->nNextType = 1; // QH

            // Finally, fix the linked list
            pOldTail->pMetaData->pNext = pQH;
            pOldTail->pMetaData->pPrev = pQH;
        }

        // No longer reclaiming
        m_pCurrentQueueHead->hrcl = 1;
    }
    else
    {
        ERROR("EHCI: Queue tail is null!");
    }
}

void Ehci::addInterruptInHandler(UsbEndpoint endpointInfo, uintptr_t pBuffer, uint16_t nBytes, void (*pCallback)(uintptr_t, ssize_t), uintptr_t pParam)
{
    LockGuard<Mutex> guard(m_Mutex);

    // Find an empty frame entry
    size_t nFrameIndex = m_FrameBitmap.getFirstClear();
    if(nFrameIndex >= 1024)
    {
        ERROR("USB: EHCI: Frame list full");
        return;
    }
    m_FrameBitmap.set(nFrameIndex);

    // Create a new transaction
    uintptr_t nTransaction = createTransaction(endpointInfo);

    // Get the QH and set the periodic flag
    QH *pQH = &m_pQHList[nTransaction];
    pQH->pMetaData->bPeriodic = true;

    // Add a single transfer to the transaction
    addTransferToTransaction(nTransaction, false, UsbPidIn, pBuffer, nBytes);

    // Get the qTD and set the error counter to "unlimited retries"
    qTD *pqTD = pQH->pMetaData->pLastQTD;
    pqTD->nErr = 0;

    // Add the QH to the frame list
    m_pFrameList[nFrameIndex] = (m_pQHListPhys + nTransaction * sizeof(QH)) | 2;
}

uint64_t Ehci::executeRequest(uint64_t p1, uint64_t p2, uint64_t p3, uint64_t p4, uint64_t p5,
                              uint64_t p6, uint64_t p7, uint64_t p8)
{
    // See if there's any device attached on the port
    if(m_pBase->read32(m_nOpRegsOffset + EHCI_PORTSC + p1 * 4) & EHCI_PORTSC_CONN)
    {
        int retry;
        for(retry = 0; retry < 3; retry++)
        {
            // Set the reset bit
#ifdef USB_VERBOSE_DEBUG
            DEBUG_LOG("USB: EHCI: Port "<<Dec<<p1<<Hex<<" - status before reset: "<<m_pBase->read32(m_nOpRegsOffset+EHCI_PORTSC+p1*4));
#endif
            m_pBase->write32(m_pBase->read32(m_nOpRegsOffset + EHCI_PORTSC + p1 * 4) | EHCI_PORTSC_PRES, m_nOpRegsOffset + EHCI_PORTSC + p1 * 4);
            delay(50);
            // Unset the reset bit
            m_pBase->write32(m_pBase->read32(m_nOpRegsOffset + EHCI_PORTSC + p1 * 4) & ~EHCI_PORTSC_PRES, m_nOpRegsOffset + EHCI_PORTSC + p1 * 4);
            // Wait for the reset to complete
            while(m_pBase->read32(m_nOpRegsOffset + EHCI_PORTSC + p1 * 4) & EHCI_PORTSC_PRES)
                delay(5);
#ifdef USB_VERBOSE_DEBUG
            DEBUG_LOG("USB: EHCI: Port " << Dec << p1 << Hex << " - status after reset: " << m_pBase->read32(m_nOpRegsOffset + EHCI_PORTSC + p1 * 4));
#endif
            if(m_pBase->read32(m_nOpRegsOffset + EHCI_PORTSC + p1 * 4) & EHCI_PORTSC_EN)
            {
                DEBUG_LOG("USB: EHCI: Port " << Dec << p1 << Hex << " is now connected");

                if(deviceConnected(p1, HighSpeed))
                    break;

                // We have to try again
                delay(500);
            }
            else
            {
                DEBUG_LOG("USB: EHCI: Port " << Dec << p1 << Hex << " seems to be not HighSpeed. Oh, well, let's send it to companions");
                m_pBase->write32(m_pBase->read32(m_nOpRegsOffset + EHCI_PORTSC + p1 * 4) | 0x2000, m_nOpRegsOffset + EHCI_PORTSC + p1 * 4);
                break;
            }
        }

        if(retry == 3)
            WARNING("EHCI: Port " << Dec << p1 << Hex << " could not be connected");
    }
    else
    {
#ifdef USB_VERBOSE_DEBUG
        DEBUG_LOG("USB: EHCI: Port " << Dec << p1 << Hex << " is now disconnected");
#endif
        deviceDisconnected(p1);
        // Clean any bits that would remain
        m_pBase->write32(m_pBase->read32(m_nOpRegsOffset + EHCI_PORTSC + p1 * 4), m_nOpRegsOffset + EHCI_PORTSC + p1 * 4);
    }
    return 0;
}
