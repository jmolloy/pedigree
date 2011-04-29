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

static int threadStub(void *p)
{
    Ohci *pOhci = reinterpret_cast<Ohci*>(p);
    pOhci->doDequeue();
    return 0;
}

Ohci::Ohci(Device* pDev) : Device(pDev), m_pCurrentBulkQueueHead(0), m_pCurrentControlQueueHead(0),
                           m_DequeueListLock(), m_DequeueList(), m_DequeueCount(0), m_OhciMR("Ohci-MR")
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

    // Get an ED for the periodic list
    m_EDBitmap.set(0);
    ED *pPeriodicED = &m_pEDList[0];
    memset(pPeriodicED, 0, sizeof(ED));
    // HC will skip this ED
    pPeriodicED->bSkip = true;

    // Set all HCCA interrupt ED entries to our periodic ED
    dmemset(m_pHcca->pInterruptEDList, m_pEDListPhys, 3);

    // Every periodic ED will be added after this one
    m_pCurrentPeriodicQueueTail = pPeriodicED;

#ifdef X86_COMMON
    uint32_t nPciCmdSts = PciBus::instance().readConfigSpace(this, 1);
    PciBus::instance().writeConfigSpace(this, 1, nPciCmdSts | 0x6);
#endif

    // Grab the ports
    m_pBase = m_Addresses[0]->m_Io;
    m_Addresses[0]->map();
    
    uint8_t version = m_pBase->read32(OhciVersion) & 0xFF;
    DEBUG_LOG("USB: OHCI: starting up - controller is version " << Dec << ((version & 0xF0) >> 8) << "." << (version & 0xF) << Hex << ".");
    
    // Determine first of all if the HC is controlled by the BIOS.
    uint32_t control = m_pBase->read32(OhciControl);
    if(control & OhciControlInterruptRoute)
    {
        // SMM.
        DEBUG_LOG("USB: OHCI: currently in SMM!");
        uint32_t status = m_pBase->read32(OhciCommandStatus);
        m_pBase->write32(status | OhciCommandRequestOwnership, OhciCommandStatus);
        while((control = m_pBase->read32(OhciControl)) & OhciControlInterruptRoute)
            delay(1);
    }
    else if(control & OhciControlStateFunctionalMask)
    {
        // Chances are good that the BIOS has the thing running.
        DEBUG_LOG("USB: OHCI: BIOS is currently in charge.");
        
        // Throw the controller into operational mode if it isn't.
        if(!(control & OhciControlStateRunning))
            m_pBase->write32(OhciControlStateRunning, OhciControl);
    }
    
    // Perform a reset via the UHCI Control register.
    m_pBase->write32(control & ~OhciControlStateFunctionalMask, OhciControl);
    delay(200);
    
    // Grab the FM Interval register (5.1.1.4, OHCI spec).
    uint32_t interval = m_pBase->read32(OhciFmInterval);

    // Perform a full hardware reset.
    m_pBase->write32(OhciCommandHcReset, OhciCommandStatus);
    while(m_pBase->read32(OhciCommandStatus) & OhciCommandHcReset)
        delay(5);
    
    // We now have 2 ms to complete all operations before we start the controller. 5.1.1.4, OHCI spec.

    // Set up the HCCA block.
    m_pBase->write32(m_pHccaPhys, OhciHcca);
    
    /// \todo Two ED queues, for control and bulk EDs.
    
    // Set up the operational registers.
    m_pBase->write32(m_pEDListPhys, OhciControlHeadED);
    m_pBase->write32(m_pEDListPhys, OhciBulkHeadED);
    
    // Disable and then enable the interrupts we want.
    m_pBase->write32(0x4000007F, OhciInterruptDisable);
    m_pBase->write32(OhciInterruptMIE | OhciInterruptWbDoneHead | 0x1 | 0x10 | 0x20, OhciInterruptEnable);
    
    // Prepare the control register
    control = m_pBase->read32(OhciControl);
    control &= ~(0x3 | 0x3C | OhciControlStateFunctionalMask | OhciControlInterruptRoute); // Control bulk service, List enable, etc
    control |= OhciControlListsEnable | OhciControlStateRunning | 0x3; // 4:1 control/bulk ED ratio
    m_pBase->write32(control, OhciControl);
    
    // Controller is now running. Yay!
    
    // Restore the Frame Interval register (reset by a HC reset)
    m_pBase->write32(interval | (1 << 31), OhciFmInterval);
    
    DEBUG_LOG("USB: OHCI: maximum packet size is " << ((interval >> 16) & 0xEFFF));
    
    // Turn on all ports on the root hub.
    m_pBase->write32(0x10000, OhciRhStatus);
    
    // Dequeue main thread
    new Thread(Processor::information().getCurrentThread()->getParent(), threadStub, reinterpret_cast<void*>(this));

    // Install the IRQ handler
    Machine::instance().getIrqManager()->registerPciIrqHandler(this, this);
    Machine::instance().getIrqManager()->control(getInterruptNumber(), IrqManager::MitigationThreshold, (1500000 / 64)); // 12KB/ms (12Mbps) in bytes, divided by 64 bytes maximum per transfer/IRQ

    // Set up the RequestQueue
    initialise();

    // Get the number of ports and delay for power-up for this root hub. 
    uint32_t rhDescA = m_pBase->read32(OhciRhDescriptorA);
    uint8_t powerWait = ((rhDescA >> 24) & 0xFF) * 2;
    m_nPorts = rhDescA & 0xFF;

    DEBUG_LOG("USB: OHCI: Reset complete, " << Dec << m_nPorts << Hex << " ports available");

    for(size_t i = 0; i < m_nPorts; i++)
    {
        if(!(m_pBase->read32(OhciRhPortStatus + (i * 4)) & OhciRhPortStsPower))
        {
            DEBUG_LOG("USB: OHCI: applying power to port " << i);
            
            // Needs port power, do so
            m_pBase->write32(OhciRhPortStsPower, OhciRhPortStatus + (i * 4));

            // Wait as long as it needs
            delay(powerWait);
        }
        
        DEBUG_LOG("OHCI: Determining if there's a device on this port");

        // Check for a connected device
        if(m_pBase->read32(OhciRhPortStatus + (i * 4)) & OhciRhPortStsConnected)
            executeRequest(i);
    }

    // Enable RootHubStatusChange interrupt now that it's safe to
    m_pBase->write32(OhciInterruptRhStsChange, OhciInterruptStatus);
    m_pBase->write32(OhciInterruptMIE | OhciInterruptRhStsChange | OhciInterruptWbDoneHead, OhciInterruptEnable);
}

Ohci::~Ohci()
{
}

void Ohci::doDequeue()
{
    while(1)
    {
        m_DequeueCount.acquire();
        
        ED *pED = 0;
        {
            LockGuard<Spinlock> guard(m_DequeueListLock);
            pED = m_DequeueList.popFront();
        }
        
        if(!pED)
            continue;
        
        if(pED->pMetaData->completedTdList.count())
        {
            for(List<TD*>::Iterator it = pED->pMetaData->completedTdList.begin();
                it != pED->pMetaData->completedTdList.end();
                it++)
            {
                size_t idx = (*it)->id;
                memset(*it, 0, sizeof(TD));
                
                m_TDBitmap.clear(idx);
            }
        }
        
        // We might still have TDs that haven't been completed, if an earlier
        // transaction fails somewhere.
        if(pED->pMetaData->tdList.count())
        {
            for(List<TD*>::Iterator it = pED->pMetaData->completedTdList.begin();
                it != pED->pMetaData->completedTdList.end();
                it++)
            {
                size_t idx = (*it)->id;
                memset(*it, 0, sizeof(TD));
                
                m_TDBitmap.clear(idx);
            }
        }
        
        size_t edId = pED->pMetaData->id;
        
        delete pED->pMetaData;
        memset(pED, 0, sizeof(ED));
        
        m_EDBitmap.clear(edId);
        
#ifdef USB_VERBOSE_DEBUG
        DEBUG_LOG("Dequeue for ED #" << Dec << edId << Hex << " complete.");
#endif
    }
}

#ifdef X86_COMMON
bool Ohci::irq(irq_id_t number, InterruptState &state)
#else
void Ohci::interrupt(size_t number, InterruptState &state)
#endif
{
    if(!m_pHcca)
    {
        return
#ifdef X86_COMMON
        true
#endif
        ;
    }

    uint32_t nStatus = 0;
    
    // Have a look at the HCCA status.
    uint32_t done = m_pHcca->pDoneHead;
    if(done)
    {
        m_pHcca->pDoneHead = 0;
        if(done & ~0x1)
        {
            // Override other interrupt causes.
            nStatus = OhciInterruptWbDoneHead;
        }
        
        if(done & 0x1)
        {
            nStatus = m_pBase->read32(OhciInterruptStatus);
            done &= ~0x1;
        }
    }
    else
        nStatus = m_pBase->read32(OhciInterruptStatus);
    
    // Not for us?
    if(!nStatus)
    {
        return
#ifdef X86_COMMON
        true
#endif
        ;
    }
    
    // Clear the MIE bit from the interrupt status. We don't care for it.
    nStatus &= ~0x40000000;
    
#ifdef USB_VERBOSE_DEBUGGING
    DEBUG_LOG("OHCI: IRQ " << nStatus);
#endif

    // Check for newly connected / disconnected devices
    if(nStatus & OhciInterruptRhStsChange)
        for(size_t i = 0; i < m_nPorts; i++)
            if(m_pBase->read32(OhciRhPortStatus + (i * 4)) & OhciRhPortStsConnStsCh)
                addAsyncRequest(0, i);

    if(nStatus & OhciInterruptWbDoneHead)
    {
        bool bControlListChanged = false, bBulkListChanged = false;

        // Check all the EDs we have in the list
        /// \todo Move to the same style as UHCI, with lists of EDs to check.
        for(size_t i = 1; i < (0x1000 / sizeof(ED)); i++)
        {
            if(!m_EDBitmap.test(i))
                continue;

            ED *pED = &m_pEDList[i];
            
            // Verification.
            if(!pED->pMetaData)
                continue;
            if(pED->bSkip || pED->pMetaData->bIgnore)
                continue;
            if(!pED->pMetaData->bLinked)
                continue;

            bool bPeriodic = pED->pMetaData->bPeriodic;

            // Iterate the TD list
            TD *pTD = 0;
            while(pED->pMetaData->tdList.count())
            {
                pTD = pED->pMetaData->tdList.popFront();
                
                // TD not yet handled - return to the list and go to the next ED.
                if(pTD->nStatus == 0xF)
                {
                    pED->pMetaData->tdList.pushFront(pTD);
                    break;
                }
            
                ssize_t nResult;
                if(pTD->nStatus)
                {
#ifdef USB_VERBOSE_DEBUG
                    if(!bPeriodic)
                        ERROR_NOLOCK("TD Error " << Dec << pTD->nStatus << Hex);
#endif
                    nResult = - pTD->getError();
                }
                else
                {
                    if(pTD->pBufferStart)
                    {
                        // Only a part of the buffer has been transfered
                        size_t nBytesLeft = pTD->pBufferEnd - pTD->pBufferStart + 1;
                        nResult = pTD->nBufferSize - nBytesLeft;
                    }
                    else
                        nResult = pTD->nBufferSize;
                    pED->pMetaData->nTotalBytes += nResult;
                }
#ifdef USB_VERBOSE_DEBUG
                DEBUG_LOG_NOLOCK("TD #" << Dec << pTD->id << Hex << " [from ED #" << Dec << pED->pMetaData->id << Hex << "] DONE: " << Dec << pED->nAddress << ":" << pED->nEndpoint << " " << (pTD->nPid==1?"OUT":(pTD->nPid==2?"IN":(pTD->nPid==0?"SETUP":""))) << " " << nResult << Hex);
#endif

                /// \note It might be nice to document this.
                bool bEndOfTransfer = (!bPeriodic && ((nResult < 0) || (pTD == pED->pMetaData->pLastTD))) || (bPeriodic && (nResult >= 0));
                
                if(!bPeriodic)
                    pED->pMetaData->completedTdList.pushBack(pTD);

                // Last TD or error condition, if async, otherwise only when it gives no error
                if(bEndOfTransfer)
                {
                    // Valid callback?
                    if(pED->pMetaData->pCallback)
                    {
                        pED->pMetaData->pCallback(pED->pMetaData->pParam, nResult < 0 ? nResult : pED->pMetaData->nTotalBytes);
                    }

                    if(!bPeriodic)
                    {
                        // HC will skip with ED next time
                        pED->bSkip = pED->pMetaData->bIgnore = true;

                        // This ED is done, dequeue.
                        ED *pPrev = pED->pMetaData->pPrev;
                        ED *pNext = pED->pMetaData->pNext;
                        if(pNext)
                            pNext->pMetaData->pPrev = pPrev;
                        if(pPrev)
                        {
                            // Main non-hardware linked list update
                            pPrev->pMetaData->pNext = pNext;

                            // Hardware linked list update
                            pPrev->pNext = pED->pNext;
                        }

                        // Update the head pointers if we need to
                        m_QueueListChangeLock.acquire(); // Atomic operation
                        if(pED == m_pCurrentControlQueueHead)
                            m_pCurrentControlQueueHead = pNext;
                        if(pED == m_pCurrentBulkQueueHead)
                            m_pCurrentBulkQueueHead = pNext;
                        m_QueueListChangeLock.release();

                        // We changed one of the lists
                        if(!pED->pMetaData->endpointInfo.nEndpoint)
                            bControlListChanged = true;
                        else
                            bBulkListChanged = true;
                    
                        {
                            m_DequeueListLock.acquire();
                            m_DequeueList.pushBack(pED);
                            m_DequeueListLock.release();
                        }

                        break;
                    }
                    else
                    {
                        // Invert data toggle
                        pTD->bDataToggle = !pTD->bDataToggle;

                        // Clear the total bytes field so it won't grow with each completed transfer
                        pED->pMetaData->nTotalBytes = 0;
                    }
                }

                // Interrupt TDs need to be always active
                if(bPeriodic)
                {
                    pTD->nStatus = 0xf;
                    pTD->pBufferStart = pTD->pBufferEnd - pTD->nBufferSize + 1;
                    pED->pHeadTD = PHYS_TD(pTD->id) >> 4;
                    
                    pED->pMetaData->tdList.pushBack(pTD);
                    break; // Only one TD in a periodic transfer.
                }
            }
        }

        /// \todo Update to look more like UHCI and EHCI.
        if(bControlListChanged || bBulkListChanged)
        {
            // We changed one or both lists, fix the hardware list state
            if(bControlListChanged)
            {
                if(m_pCurrentControlQueueHead)
                {
                    size_t nHeadED = reinterpret_cast<uintptr_t>(m_pCurrentControlQueueHead) & 0xFFF;
                    m_pBase->write32(m_pEDListPhys + nHeadED, OhciControlHeadED);
                    m_pBase->write32(OhciCommandControlListFilled, OhciCommandStatus);
                }
                else
                    m_pBase->write32(0, OhciControlHeadED);
            }
            if(bBulkListChanged)
            {
                if(m_pCurrentBulkQueueHead)
                {
                    size_t nHeadED = reinterpret_cast<uintptr_t>(m_pCurrentBulkQueueHead) & 0xFFF;
                    m_pBase->write32(m_pEDListPhys + nHeadED, OhciBulkHeadED);
                    m_pBase->write32(OhciCommandBulkListFilled, OhciCommandStatus);
                }
                else
                    m_pBase->write32(0, OhciBulkHeadED);
            }
        }
    }

    // Clear the interrupt status now that we are done processing it.
    m_pBase->write32(nStatus, OhciInterruptStatus);

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
    pTD->id = nIndex;

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

    // Buffer for transfer
    if(nBytes)
    {
        VirtualAddressSpace &va = Processor::information().getVirtualAddressSpace();
        if(va.isMapped(reinterpret_cast<void*>(pBuffer)))
        {
            physical_uintptr_t phys = 0; size_t flags = 0;
            va.getMapping(reinterpret_cast<void*>(pBuffer), phys, flags);
            pTD->pBufferStart = phys + (pBuffer & 0xFFF);
            pTD->pBufferEnd = pTD->pBufferStart + nBytes - 1;
        }
        else
        {
            ERROR("OHCI: addTransferToTransaction: Buffer (page " << Dec << pBuffer << Hex << ") isn't mapped!");
            m_TDBitmap.clear(nIndex);
            return;
        }

        pTD->nBufferSize = nBytes;
    }

    // This is the last TD so far
    pTD->bLast = true;

    // Grab the transaction's ED and add our TD to it
    ED *pED = &m_pEDList[pTransaction];
    if(pED->pMetaData->pLastTD)
    {
        pED->pMetaData->pLastTD->pNext = PHYS_TD(nIndex) >> 4;
        pED->pMetaData->pLastTD->nNextTDIndex = nIndex;
        pED->pMetaData->pLastTD->bLast = false;
    }
    else
    {
        pED->pMetaData->pFirstTD = pTD;
        pED->pHeadTD = PHYS_TD(nIndex) >> 4;
    }
    pED->pMetaData->pLastTD = pTD;
    
    pED->pMetaData->tdList.pushBack(pTD);
}

uintptr_t Ohci::createTransaction(UsbEndpoint endpointInfo)
{
    // Atomic operation: find clear bit, set it
    size_t nIndex = 0;
    {
        LockGuard<Mutex> guard(m_Mutex);
        nIndex = m_EDBitmap.getFirstClear();
        if(nIndex >= (0x1000 / sizeof(ED)))
        {
            ERROR("USB: OHCI: ED space full");
            return static_cast<uintptr_t>(-1);
        }
        m_EDBitmap.set(nIndex);
    }

    ED *pED = &m_pEDList[nIndex];
    memset(pED, 0, sizeof(ED));

    // Device address, endpoint and speed
    pED->nAddress = endpointInfo.nAddress;
    pED->nEndpoint = endpointInfo.nEndpoint;
    pED->bLoSpeed = endpointInfo.speed == LowSpeed;

    // Maximum packet size
    pED->nMaxPacketSize = endpointInfo.nMaxPacketSize;

    // Set sKip, will be unset when the ED gets inserted into the list
    pED->bSkip = true;

    // Setup the metadata
    pED->pMetaData = new ED::MetaData;
    memset(pED->pMetaData, 0, sizeof(ED::MetaData));
    pED->pMetaData->endpointInfo = endpointInfo;
    pED->pMetaData->id = nIndex;
    pED->pMetaData->bIgnore = false;

    // Complete
    return nIndex;
}

void Ohci::doAsync(uintptr_t pTransaction, void (*pCallback)(uintptr_t, ssize_t), uintptr_t pParam)
{
    {
        LockGuard<Mutex> guard(m_Mutex);
        if((pTransaction == static_cast<uintptr_t>(-1)) || !m_EDBitmap.test(pTransaction))
        {
            ERROR("OHCI: doAsync: didn't get a valid transaction id [" << pTransaction << "].");
            return;
        }
    }
    
    /// \todo Stop the controller as we are modifying the queue.

    // Atomic operation - modifying both the housekeeping and the
    // hardware linked lists
    LockGuard<Spinlock> guard2(m_QueueListChangeLock);

    ED *pED = &m_pEDList[pTransaction];
    pED->pMetaData->pCallback = pCallback;
    pED->pMetaData->pParam = pParam;

    bool bControl = !pED->pMetaData->endpointInfo.nEndpoint;

    ED *pOldQueueHead = 0;
    if(bControl)
        pOldQueueHead = m_pCurrentControlQueueHead;
    else
        pOldQueueHead = m_pCurrentBulkQueueHead;

    // Link in to the asynchronous schedule
    if(pOldQueueHead)
    {
        // Current ED needs to point to the schedule's head
        size_t nHeadED = reinterpret_cast<uintptr_t>(pOldQueueHead) & 0xFFF;
        pED->pNext = (m_pEDListPhys + nHeadED) >> 5;

        // Enter the information for correct dequeue
        pED->pMetaData->pNext = pOldQueueHead;
        pOldQueueHead->pMetaData->pPrev = pED;
    }

    // Enable the ED by unsetting sKip
    pED->bSkip = false;
    // We are now linked
    pED->pMetaData->bLinked = true;

    // Update the head pointer and the one in the HC
    if(bControl)
    {
        m_pCurrentControlQueueHead = pED;
        m_pBase->write32(m_pEDListPhys + pTransaction * sizeof(ED), OhciControlHeadED);
        m_pBase->write32(OhciCommandControlListFilled, OhciCommandStatus);
    }
    else
    {
        m_pCurrentBulkQueueHead = pED;
        m_pBase->write32(m_pEDListPhys + pTransaction * sizeof(ED), OhciBulkHeadED);
        m_pBase->write32(OhciCommandBulkListFilled, OhciCommandStatus);
    }
}

void Ohci::addInterruptInHandler(UsbEndpoint endpointInfo, uintptr_t pBuffer, uint16_t nBytes, void (*pCallback)(uintptr_t, ssize_t), uintptr_t pParam)
{
    // Create a new transaction
    uintptr_t nTransaction = createTransaction(endpointInfo);

    // Get the ED and fix some stuff
    ED *pED = &m_pEDList[nTransaction];
    // Set the periodic & linked flags
    pED->pMetaData->bPeriodic = true;
    pED->pMetaData->bLinked = true;
    // Enable the ED by unsetting sKip
    pED->bSkip = false;
    // Set callback and param
    pED->pMetaData->pCallback = pCallback;
    pED->pMetaData->pParam = pParam;

    // Add a single transfer to the transaction
    addTransferToTransaction(nTransaction, false, UsbPidIn, pBuffer, nBytes);

    // Add the ED to the periodic queue
    LockGuard<Mutex> guard(m_Mutex);
    LockGuard<Spinlock> guard2(m_QueueListChangeLock); // Atomic operation
    m_pCurrentPeriodicQueueTail->pNext = (m_pEDListPhys + nTransaction * sizeof(ED)) >> 4;
    m_pCurrentPeriodicQueueTail = pED;
}

bool Ohci::portReset(uint8_t nPort, bool bErrorResponse)
{
    /// \todo Error handling? Device fails to reset? Not present after reset?

    // Perform a reset of the port
    m_pBase->write32(OhciRhPortStsReset | OhciRhPortStsConnStsCh, OhciRhPortStatus + (nPort * 4));
    while(!(m_pBase->read32(OhciRhPortStatus + (nPort * 4)) & OhciRhPortStsResCh))
        delay(5);
    m_pBase->write32(OhciRhPortStsResCh, OhciRhPortStatus + (nPort * 4));

    // Enable the port if not already enabled
    if(!(m_pBase->read32(OhciRhPortStatus + (nPort * 4)) & OhciRhPortStsEnable))
        m_pBase->write32(OhciRhPortStsEnable, OhciRhPortStatus + (nPort * 4));

    return true;
}

uint64_t Ohci::executeRequest(uint64_t p1, uint64_t p2, uint64_t p3, uint64_t p4, uint64_t p5,
                              uint64_t p6, uint64_t p7, uint64_t p8)
{
    // Check for a connected device
    if(m_pBase->read32(OhciRhPortStatus + (p1 * 4)) & OhciRhPortStsConnected)
    {
        if(!portReset(p1))
            return 0;

        // Determine the speed of the attached device
        if(m_pBase->read32(OhciRhPortStatus + (p1 * 4)) & OhciRhPortStsLoSpeed)
        {
            DEBUG_LOG("USB: OHCI: Port " << Dec << p1 << Hex << " has a low-speed device connected to it");
            deviceConnected(p1, LowSpeed);
        }
        else
        {
            DEBUG_LOG("USB: OHCI: Port " << Dec << p1 << Hex << " has a full-speed device connected to it");
            deviceConnected(p1, FullSpeed);
        }
    }
    else
        deviceDisconnected(p1);
    return 0;
}
