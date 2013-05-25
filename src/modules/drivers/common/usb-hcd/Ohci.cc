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

Ohci::Ohci(Device* pDev) : UsbHub(pDev), m_Mutex(false), m_ScheduleChangeLock(),
                           m_PeriodicListChangeLock(), m_ControlListChangeLock(),
                           m_BulkListChangeLock(), m_PeriodicEDBitmap(),
                           m_ControlEDBitmap(), m_BulkEDBitmap(),
                           m_pBulkQueueHead(0), m_pControlQueueHead(0),
                           m_pBulkQueueTail(0), m_pControlQueueTail(0), m_pPeriodicQueueTail(0),
                           m_DequeueListLock(), m_DequeueList(), m_DequeueCount(0), m_OhciMR("Ohci-MR")
{
    setSpecificType(String("OHCI"));
    
    // Allocate the memory region
    if(!PhysicalMemoryManager::instance().allocateRegion(m_OhciMR, 5, PhysicalMemoryManager::continuous, VirtualAddressSpace::Write | VirtualAddressSpace::KernelMode))
    {
        ERROR("USB: OHCI: Couldn't allocate memory region!");
        return;
    }

    uintptr_t virtualBase   = reinterpret_cast<uintptr_t>(m_OhciMR.virtualAddress());
    uintptr_t physicalBase  = m_OhciMR.physicalAddress();
    
    m_pHcca                 = reinterpret_cast<Hcca*>(virtualBase);
    m_pBulkEDList           = reinterpret_cast<ED*>(virtualBase + 0x1000);
    m_pControlEDList        = reinterpret_cast<ED*>(virtualBase + 0x2000);
    m_pPeriodicEDList       = reinterpret_cast<ED*>(virtualBase + 0x3000);
    m_pTDList               = reinterpret_cast<TD*>(virtualBase + 0x4000);

    m_pHccaPhys             = physicalBase;
    m_pBulkEDListPhys       = physicalBase + 0x1000;
    m_pControlEDListPhys    = physicalBase + 0x2000;
    m_pPeriodicEDListPhys   = physicalBase + 0x3000;
    m_pTDListPhys           = physicalBase + 0x4000;

    // Clear out the HCCA block.
    memset(m_pHcca, 0, 0x800);

    // Get an ED for the periodic list
    m_PeriodicEDBitmap.set(0);
    ED *pPeriodicED = m_pPeriodicEDList;
    memset(pPeriodicED, 0, sizeof(ED));
    pPeriodicED->bSkip = true;
    pPeriodicED->pMetaData = new ED::MetaData;
    pPeriodicED->pMetaData->id = 0x2000;
    pPeriodicED->pMetaData->pPrev = pPeriodicED->pMetaData->pNext = pPeriodicED;

    // Set all HCCA interrupt ED entries to our periodic ED
    dmemset(m_pHcca->pInterruptEDList, m_pPeriodicEDListPhys, 3);

    // Every periodic ED will be added after this one
    m_pPeriodicQueueTail = pPeriodicED;
    
    m_pBulkQueueTail = m_pBulkQueueHead = 0;
    m_pControlQueueTail = m_pControlQueueHead = 0;

#ifdef X86_COMMON
    // Make sure bus mastering and MMIO are enabled.
    uint32_t nPciCmdSts = PciBus::instance().readConfigSpace(this, 1);
    PciBus::instance().writeConfigSpace(this, 1, nPciCmdSts | 0x6);
#endif

    // Grab the ports
    m_pBase = m_Addresses[0]->m_Io;
    m_Addresses[0]->map();
    
    // Dump the version of the controller and a nice little banner.
    uint8_t version = m_pBase->read32(OhciVersion) & 0xFF;
    DEBUG_LOG("USB: OHCI: starting up - controller is version " << Dec << ((version & 0xF0) >> 4) << "." << (version & 0xF) << Hex << ".");
    
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
    else
    {
        // Chances are good that the BIOS has the thing running.
        if(control & OhciControlStateFunctionalMask)
            DEBUG_LOG("USB: OHCI: BIOS is currently in charge.");
        else
            DEBUG_LOG("USB: OHCI: not yet operational.");
        
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
    
    // Set up the operational registers.
    m_pBase->write32(m_pControlEDListPhys, OhciControlHeadED);
    m_pBase->write32(m_pBulkEDListPhys, OhciBulkHeadED);
    
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

    // Set up the RequestQueue
    initialise();
    
    // Dequeue main thread
    // new Thread(Processor::information().getCurrentThread()->getParent(), threadStub, reinterpret_cast<void*>(this));

    // Install the IRQ handler
    Machine::instance().getIrqManager()->registerPciIrqHandler(this, this);
    Machine::instance().getIrqManager()->control(getInterruptNumber(), IrqManager::MitigationThreshold, (1500000 / 64)); // 12KB/ms (12Mbps) in bytes, divided by 64 bytes maximum per transfer/IRQ

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

void Ohci::removeED(ED *pED)
{
    /// \note Refer to page 56 in the OHCI spec for this function.
    
    if(!pED || !pED->pMetaData)
        return;

#ifdef USB_VERBOSE_DEBUG
    DEBUG_LOG("OHCI: removing ED #" << pED->pMetaData->id << " from the schedule to prepare for reclamation");
#endif
    
    // Make sure the ED is skipped by the host controller until it is properly
    // dequeued.
    pED->bSkip = true;
    
    ED *pPrev = pED->pMetaData->pPrev;
    ED *pNext = pED->pMetaData->pNext;
    
    ED **pQueueHead = 0;
    ED **pQueueTail = 0;
    
    if(pED->pMetaData->edType == ControlList)
    {
        pQueueHead = &m_pControlQueueHead;
        pQueueTail = &m_pControlQueueTail;
    }
    else if(pED->pMetaData->edType == BulkList)
    {
        pQueueHead = &m_pBulkQueueHead;
        pQueueTail = &m_pBulkQueueTail;
    }
    
    bool bControl = pED->pMetaData->edType == ControlList;
    
    // Unlink from the hardware linked list.
    if(pED == *pQueueHead)
    {
#ifdef USB_VERBOSE_DEBUG
        DEBUG_LOG("OHCI: ED was a queue head, adjusting controller state accordingly");
#endif

        *pQueueHead = pNext;
        
        if(bControl)
            m_pBase->write32(vtp_ed(pNext), OhciControlHeadED);
        else /// \todo Isochronous and Periodic.
            m_pBase->write32(vtp_ed(pNext), OhciBulkHeadED);
    }
    else if(pPrev)
    {
        pPrev->pNext = pED->pNext;
    }
    
    // Simply for tracking purposes, make sure the tail is valid.
    if(pED == *pQueueTail)
    {
        *pQueueTail = pPrev;
    }
    
    // Unlink from the software linked list.
    if(pPrev)
        pPrev->pMetaData->pNext = pNext;
    if(pNext)
        pNext->pMetaData->pPrev = pPrev;
    
    // Clear all TDs related to the ED.
    if(pED->pMetaData->completedTdList.count())
    {
        for(List<TD*>::Iterator it = pED->pMetaData->completedTdList.begin();
            it != pED->pMetaData->completedTdList.end();
            it++)
        {
            size_t idx = (*it)->id;
            memset((*it), 0, sizeof(TD));
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
            memset((*it), 0, sizeof(TD));
            m_TDBitmap.clear(idx);
        }
    }
    
    // Disable list processing for this ED
    stop(pED->pMetaData->edType);
    
    // Prepare the ED for reclamation in the next USB frame.
    {
        LockGuard<Spinlock> guard(m_DequeueListLock);
        m_DequeueList.pushBack(pED);
    }
    
    // Clear any pending SOF interrupt and then enable the SOF IRQ.
    // The IRQ handler will pick up a SOF status and clean up the ED.
    m_pBase->write32(OhciInterruptStartOfFrame, OhciInterruptStatus);
    m_pBase->write32(OhciInterruptStartOfFrame, OhciInterruptEnable);
}

#ifdef X86_COMMON
bool Ohci::irq(irq_id_t number, InterruptState &state)
#else
void Ohci::interrupt(size_t number, InterruptState &state)
#endif
{
    if(!m_pHcca)
    {
        // Assume not for us - no HCCA yet!
        return
#ifdef X86_COMMON
        false
#endif
        ;
    }

    uint32_t nStatus = 0;
    
    // Find out if this came from either a done ED or a useful status.
    if(m_pHcca->pDoneHead)
    {
        // We must process this interrupt.
        nStatus = OhciInterruptWbDoneHead;
        if(m_pHcca->pDoneHead & 0x1) // ... ???
            nStatus |= m_pBase->read32(OhciInterruptStatus) & m_pBase->read32(OhciInterruptEnable);
    }
    else
    {
        nStatus = m_pBase->read32(OhciInterruptStatus) & m_pBase->read32(OhciInterruptEnable);
    }
    
    // Not for us?
    if(!nStatus)
    {
        DEBUG_LOG("USB: OHCI: irq is not for us");
        return
#ifdef X86_COMMON
        false
#endif
        ;
    }
    
    // However, make sure we do not get interrupted during handling.
    m_pBase->write32(OhciInterruptMIE, OhciInterruptDisable);
    
    // Clear the MIE bit from the interrupt status. We don't care for it.
    nStatus &= ~OhciInterruptMIE;
    
#ifdef USB_VERBOSE_DEBUG
    DEBUG_LOG("OHCI: IRQ " << nStatus);
#endif

    if(nStatus & OhciInterruptUnrecoverableError)
    {
        /// \todo Handle.
        
        // Don't enable interrupts again, controller is not in a safe state.
        ERROR("OHCI: controller is hung!");
        return
#ifdef X86_COMMON
        true
#endif
        ;
    }
    
    if(nStatus & OhciInterruptStartOfFrame)
    {
        LockGuard<Spinlock> guard(m_DequeueListLock);
        
#ifdef USB_VERBOSE_DEBUG
        DEBUG_LOG("OHCI: SOF, preparing to reclaim EDs...");
#endif

        // Firstly disable the SOF interrupt now that we've gotten it.
        m_pBase->write32(OhciInterruptStartOfFrame, OhciInterruptDisable);
        
        // Process the reclaim list.
        ED *pED = 0;
        while(1)
        {
            if(!m_DequeueList.count())
                break;
            
            pED = m_DequeueList.popFront();
            if(pED)
            {
                size_t id = pED->pMetaData->id & 0xFFF;
                Lists type = pED->pMetaData->edType;

#ifdef USB_VERBOSE_DEBUG
                DEBUG_LOG("OHCI: freeing ED #" << pED->pMetaData->id << ".");
#endif
                
                // Destroy the ED and free it's memory space.
                delete pED->pMetaData;
                memset(pED, 0, sizeof(ED));
                
                switch(type)
                {
                    case ControlList:
                        m_ControlEDBitmap.clear(id);
                        break;
                    case BulkList:
                        m_BulkEDBitmap.clear(id);
                        break;
                    case PeriodicList:
                        WARNING("periodic: not actually clearing bit");
                        // m_PeriodicEDBitmap.clear(edId);
                        break;
                    case IsochronousList:
                        DEBUG_LOG("USB: OHCI: dequeue on an isochronous ED, but we don't support them yet.");
                        break;
                }
                
                // Safe to restore this list to the running state.
                /// \note List processing won't start until the NEXT SOF.
                start(type);
            }
        }
    }

    // Check for newly connected / disconnected devices
    if(nStatus & OhciInterruptRhStsChange)
        for(size_t i = 0; i < m_nPorts; i++)
            if(m_pBase->read32(OhciRhPortStatus + (i * 4)) & OhciRhPortStsConnStsCh)
                addAsyncRequest(0, i);
    
    // A list of EDs that persist in the schedule. Used to repopulate the schedule list.
    List<ED*> persistList;

    if(nStatus & OhciInterruptWbDoneHead)
    {
        ED *pED = 0;
        do
        {
            {
                LockGuard<Spinlock> guard(m_ScheduleChangeLock);
                if(m_FullSchedule.count())
                    pED = m_FullSchedule.popFront();
                else
                    break;
            }
            
            // Assume not yet linked properly
            if(pED->pMetaData->bIgnore)
            {
                persistList.pushBack(pED);
                continue;
            }
            
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
                        removeED(pED);
                        continue;
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
            
            // If this ED is not queued for deletion, make sure we can use it
            // in the next IRQ.
            if(!pED->pMetaData->bIgnore)
                persistList.pushBack(pED);
        } while(pED);
    }
    
    // Restore EDs into the schedule if they were removed and need to persist.
    if(persistList.count())
    {
        LockGuard<Spinlock> guard(m_ScheduleChangeLock);
        for(List<ED*>::Iterator it = persistList.begin();
            it != persistList.end();
            it++)
        {
            m_FullSchedule.pushBack(*it);
            it = persistList.erase(it);
        }
    }
    
    // Clear the interrupt status now.
    m_pBase->write32(nStatus, OhciInterruptStatus);
    
    // Re-enable all interrupts.
    m_pBase->write32(OhciInterruptMIE, OhciInterruptEnable);

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
    
    // pTransaction will be 0x0xxx for CONTROL, 0x1xxx for BULK, 0x2xxx for PERIODIC.
    size_t transactionType = (pTransaction & 0x3000) >> 12;
    uintptr_t edOffset = pTransaction & 0xFFF;
    
    ED *pED = 0;
    switch(transactionType)
    {
        case 0:
            pED = &m_pControlEDList[edOffset];
            break;
        case 1:
            pED = &m_pBulkEDList[edOffset];
            break;
        case 2:
            pED = &m_pPeriodicEDList[edOffset];
            break;
        default:
            break;
    }
    
    if(!pED)
    {
        /// \todo Clean up!
        ERROR("USB: OHCI: transaction " << pTransaction << " is invalid.");
        return;
    }

    // Add our TD to the ED's queue.
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
    // Determine what kind of transaction this is.
    bool bIsBulk = endpointInfo.nEndpoint > 0;

    // Atomic operation: find clear bit, set it
    ED *pED = 0;
    size_t nIndex = 0;
    {
        LockGuard<Mutex> guard(m_Mutex);
        
        if(bIsBulk)
            nIndex = m_BulkEDBitmap.getFirstClear();
        else
            nIndex = m_ControlEDBitmap.getFirstClear();
        
        if(nIndex >= (0x1000 / sizeof(ED)))
        {
            ERROR("USB: OHCI: ED space full");
            return static_cast<uintptr_t>(-1);
        }
        
        if(bIsBulk)
        {
            m_BulkEDBitmap.set(nIndex);
            pED = &m_pBulkEDList[nIndex];
            nIndex += 0x1000;
        }
        else
        {
            m_ControlEDBitmap.set(nIndex);
            pED = &m_pControlEDList[nIndex];
        }
    }

    memset(pED, 0, sizeof(ED));

    // Device address, endpoint and speed
    pED->nAddress = endpointInfo.nAddress;
    pED->nEndpoint = endpointInfo.nEndpoint;
    pED->bLoSpeed = endpointInfo.speed == LowSpeed;

    // Maximum packet size
    pED->nMaxPacketSize = endpointInfo.nMaxPacketSize;

    // Make sure this ED is ignored until it's properly queued.
    pED->bSkip = true;

    // Setup the metadata
    pED->pMetaData = new ED::MetaData;
    pED->pMetaData->endpointInfo = endpointInfo;
    pED->pMetaData->id = nIndex;
    pED->pMetaData->bIgnore = true; // Don't handle this ED until we're ready.
    pED->pMetaData->edType = bIsBulk ? BulkList : ControlList;
    pED->pMetaData->bPeriodic = false;
    pED->pMetaData->pFirstTD = pED->pMetaData->pLastTD = 0;
    pED->pMetaData->nTotalBytes = 0;
    pED->pMetaData->pPrev = pED->pMetaData->pNext = 0;
    pED->pMetaData->bLinked = false;

    // Complete
    return nIndex;
}

void Ohci::doAsync(uintptr_t pTransaction, void (*pCallback)(uintptr_t, ssize_t), uintptr_t pParam)
{
    // pTransaction will be 0x0xxx for CONTROL, 0x1xxx for BULK, 0x2xxx for PERIODIC.
    size_t transactionType = (pTransaction & 0x3000) >> 12;
    uintptr_t edOffset = pTransaction & 0xFFF;
    
    Spinlock *pLock = 0;
    
    ED *pED = 0;
    {
        LockGuard<Mutex> guard(m_Mutex);
        
        bool bValid = false;
        if(transactionType == 0)
            bValid = m_ControlEDBitmap.test(edOffset);
        else if(transactionType == 1)
            bValid = m_BulkEDBitmap.test(edOffset);
        
        if((pTransaction == static_cast<uintptr_t>(-1)) || !bValid)
        {
            ERROR("OHCI: doAsync: didn't get a valid transaction id [" << pTransaction << ", " << edOffset << "].");
            return;
        }
        
        if(transactionType == 0)
        {
            pED = &m_pControlEDList[edOffset];
            pLock = &m_ControlListChangeLock;
        }
        else if(transactionType == 1)
        {
            pED = &m_pBulkEDList[edOffset];
            pLock = &m_BulkListChangeLock;
        }
        else
        {
            ERROR("OHCI: doAsync: only control and bulk transactions supported");
            return;
        }
    }

    // Set up all the metadata we can at the moment.
    pED->pMetaData->pCallback = pCallback;
    pED->pMetaData->pParam = pParam;

    bool bControl = !pED->pMetaData->endpointInfo.nEndpoint;
    
    // Stop the controller as we are modifying the queue.
    // stop(pED->pMetaData->edType);
    
    // Lock while we modify the linked lists.
    pLock->acquire();
    
    // Always at the end of the ED queue. Zero means "no next ED" to OHCI.
    pED->pNext = 0;
    
    // Handle the case where there is not yet a queue head.
    if(bControl)
    {
        if(!m_pControlQueueHead)
        {
#ifdef USB_VERBOSE_DEBUG
            DEBUG_LOG("OHCI: ED is now the control queue head.");
#endif
            m_pControlQueueHead = pED;
        }
    }
    else
    {
        if(!m_pBulkQueueHead)
        {
#ifdef USB_VERBOSE_DEBUG
            DEBUG_LOG("OHCI: ED is now the control queue head.");
#endif
            m_pBulkQueueHead = pED;
        }
    }
    
    // Grab the queue head.
    ED *pQueueHead = 0;
    physical_uintptr_t queueHeadPhys = 0;
    if(bControl)
    {
        pQueueHead = m_pControlQueueHead;
        queueHeadPhys = vtp_ed(pQueueHead);
    }
    else
    {
        pQueueHead = m_pBulkQueueHead;
        queueHeadPhys = vtp_ed(pQueueHead);
    }
    
    // Update the head of the relevant list.
    if(queueHeadPhys == vtp_ed(pED))
    {
        if(bControl)
        {
#ifdef USB_VERBOSE_DEBUG
            DEBUG_LOG("OHCI: new control queue head is " << queueHeadPhys << " compared to " << m_pBase->read32(OhciControlHeadED));
            DEBUG_LOG("OHCI: current control queue ED is " << m_pBase->read32(OhciControlCurrentED));
#endif
            m_pBase->write32(queueHeadPhys, OhciControlHeadED);
        }
        else
        {
#ifdef USB_VERBOSE_DEBUG
            DEBUG_LOG("OHCI: new bulk queue head is " << queueHeadPhys);
#endif
            m_pBase->write32(queueHeadPhys, OhciBulkHeadED);
        }
    }
    
    // Grab the current tail of the list and update it to point to us.
    ED *pTail = 0;
    if(bControl)
    {
        pTail = m_pControlQueueTail;
        m_pControlQueueTail = pED;
    }
    else
    {
        pTail = m_pBulkQueueTail;
        m_pBulkQueueTail = pED;
    }
    
    // Point the old tail to this ED.
    if(pTail)
    {
        pTail->pNext = vtp_ed(pED) >> 4;
        pTail->pMetaData->pNext = pED;
    }
    
    // Fix up the software linked list.
    pED->pMetaData->pNext = 0;
    pED->pMetaData->pPrev = pTail;
    pQueueHead->pMetaData->pPrev = 0;
    
    // Enable handling of this ED now.
    pED->bSkip = pED->pMetaData->bIgnore = false;
    pED->pMetaData->bLinked = true;
    
    // Can now unlock.
    pLock->release();
    
    // Add to the housekeeping schedule before we link in proper.
    m_ScheduleChangeLock.acquire();
    m_FullSchedule.pushBack(pED);
    m_ScheduleChangeLock.release();
    
    // Restart the controller if it was stopped for some reason.
    start(pED->pMetaData->edType);
    
    // Tell the controller that the list has valid TD in it now.
    // The OHCI will automatically stop processing the ED list if it determines
    // no more transfers are pending.
    uint32_t status = m_pBase->read32(OhciCommandStatus);
    status |= bControl ? OhciCommandControlListFilled : OhciCommandBulkListFilled;
    m_pBase->write32(status, OhciCommandStatus);
}

void Ohci::addInterruptInHandler(UsbEndpoint endpointInfo, uintptr_t pBuffer, uint16_t nBytes, void (*pCallback)(uintptr_t, ssize_t), uintptr_t pParam)
{
    // Atomic operation: find clear bit, set it
    ED *pED = 0;
    size_t nIndex = 0;
    {
        LockGuard<Mutex> guard(m_Mutex);
        
        nIndex = m_PeriodicEDBitmap.getFirstClear();
        if(nIndex >= (0x1000 / sizeof(ED)))
        {
            ERROR("USB: OHCI: ED space full");
            return;
        }
        
        m_PeriodicEDBitmap.set(nIndex);
        pED = &m_pPeriodicEDList[nIndex];
        
        // Periodic identifier
        nIndex += 0x2000;
    }

    memset(pED, 0, sizeof(ED));

    // Device address, endpoint and speed
    pED->nAddress = endpointInfo.nAddress;
    pED->nEndpoint = endpointInfo.nEndpoint;
    pED->bLoSpeed = endpointInfo.speed == LowSpeed;

    // Maximum packet size
    pED->nMaxPacketSize = endpointInfo.nMaxPacketSize;

    // Make sure this ED is ignored until it's properly queued.
    pED->bSkip = true;

    // Setup the metadata
    pED->pMetaData = new ED::MetaData;
    pED->pMetaData->endpointInfo = endpointInfo;
    pED->pMetaData->id = nIndex;
    pED->pMetaData->bIgnore = true; // Don't handle this ED until we're ready.
    pED->pMetaData->edType = PeriodicList;
    pED->pMetaData->pFirstTD = pED->pMetaData->pLastTD = 0;
    pED->pMetaData->nTotalBytes = 0;
    pED->pMetaData->pPrev = pED->pMetaData->pNext = 0;
    pED->pMetaData->bLinked = false;
    
    pED->pMetaData->bPeriodic = true;
    
    // Set up the callback and pointer.
    pED->pMetaData->pCallback = pCallback;
    pED->pMetaData->pParam = pParam;
    
    // Add to the housekeeping schedule before we link in proper.
    m_ScheduleChangeLock.acquire();
    m_FullSchedule.pushBack(pED);
    m_ScheduleChangeLock.release();
    
    // Lock before linking.
    LockGuard<Spinlock> guard(m_PeriodicListChangeLock);
    
    pED->pMetaData->pPrev = m_pPeriodicQueueTail;
    
    m_pPeriodicQueueTail->pNext = vtp_ed(pED) >> 4;
    m_pPeriodicQueueTail = pED;
    
    // All linked in and ready to go!
    pED->bSkip = pED->pMetaData->bIgnore = false;
    pED->pMetaData->bLinked = true;
    
    // Start processing of the list if it isn't already active.
    start(pED->pMetaData->edType);
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

void Ohci::stop(Lists list)
{
    if(!m_pHcca)
        return;

    uint32_t control = m_pBase->read32(OhciControl);
    control &= ~static_cast<int>(list);
    m_pBase->write32(control, OhciControl);
}

void Ohci::start(Lists list)
{
    if(!m_pHcca)
        return;

    uint32_t control = m_pBase->read32(OhciControl);
    control |= static_cast<int>(list);
    m_pBase->write32(control, OhciControl);
}

