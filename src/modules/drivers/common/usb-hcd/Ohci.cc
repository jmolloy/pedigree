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
#include "Ohci.h"

#define delay(n) do{Semaphore semWAIT(0);semWAIT.acquire(1, 0, n*1000);}while(0)

Ohci::Ohci(Device* pDev) : Device(pDev), m_TransferPagesAllocator(0, 0x1000), m_OhciMR("Ohci-MR")
{
    setSpecificType(String("OHCI"));
    // Allocate the memory region
    if(!PhysicalMemoryManager::instance().allocateRegion(m_OhciMR, 3, PhysicalMemoryManager::continuous, 0))
    {
        ERROR("USB: OHCI: Couldn't allocate memory region!");
        return;
    }
    uint8_t *pVirtBase = static_cast<uint8_t*>(m_OhciMR.virtualAddress());
    m_pHcca = reinterpret_cast<Hcca*>(pVirtBase);
    m_pEDList = reinterpret_cast<ED*>(pVirtBase+0x800);
    m_pTDList = reinterpret_cast<TD*>(pVirtBase+0x1000);
    m_pTransferPages = pVirtBase+0x2000;

    m_pHccaPhys = m_OhciMR.physicalAddress();
    m_pEDListPhys = m_OhciMR.physicalAddress()+0x800;
    m_pTDListPhys = m_OhciMR.physicalAddress()+0x1000;
    m_pTransferPagesPhys = m_OhciMR.physicalAddress()+0x2000;

    memset(m_pHcca, 0, 0x800);

#ifdef X86_COMMON
    uint32_t nPciCmdSts = PciBus::instance().readConfigSpace(this, 1);
    NOTICE("USB: OHCI: Pci command: "<<(nPciCmdSts&0xffff));
    PciBus::instance().writeConfigSpace(this, 1, (nPciCmdSts & ~0x4) | 0x4);
#endif

    // Grab the ports
    m_pBase = m_Addresses[0]->m_Io;
    // Set reset bit
    m_pBase->write32(OhciCommandHcReset, OhciCommandStatus);
    while(m_pBase->read32(OhciCommandStatus) & OhciCommandHcReset);
    //delay(50);
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
    // Enable control and bulk lists
    m_pBase->write32(OhciControlListsEnable | OhciControlStateRunning, OhciControl);

    NOTICE("USB: OHCI: Reseted");
    // Get the number of ports
    m_nPorts = m_pBase->read32(OhciRhDescriptorA) & 0xf;
    for(size_t i = 0;i<m_nPorts;i++)
    {
        uint16_t nPortStatus = m_pBase->read32(OhciRhPortStatus+i*4);
        if(nPortStatus & OhciRhPortStsConnected)
        {
            m_pBase->write32(OhciRhPortStsReset/* | 0x10000*/, OhciRhPortStatus+i*4);
            while(!(m_pBase->read32(OhciRhPortStatus+i*4) & OhciRhPortStsResCh));
            m_pBase->write32(OhciRhPortStsResCh | OhciRhPortStsEnable, OhciRhPortStatus+i*4);
            //m_pBase->write32(OhciRhPortStsEnable | OHCI_PORTSC_CSCH | OHCI_PORTSC_EDCH, OhciRhPortStatus+i*4);
            //delay(50);
            //m_pBase->write32(OhciRhPortStsEnable | OHCI_PORTSC_CSCH | OHCI_PORTSC_EDCH, OhciRhPortStatus+i*4);
            NOTICE("USB: OHCI: Port "<<Dec<<i<<Hex<<" is connected");
            deviceConnected(i, FullSpeed);
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
        NOTICE("IRQ "<<nStatus);
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
            NOTICE("STOP "<<Dec<<pED->nAddress<<":"<<pED->nEndpoint<<" "<<(pTD->nPid==1?" OUT ":(pTD->nPid==2?" IN  ":(pTD->nPid==0?"SETUP":"")))<<" "<<nReturn<<Hex);
    }
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

void Ohci::doAsync(UsbEndpoint endpointInfo, uint8_t nPid, uintptr_t pBuffer, uint16_t nBytes, void (*pCallback)(uintptr_t, ssize_t), uintptr_t pParam)
{
    LockGuard<Mutex> guard(m_Mutex);
    NOTICE("START "<<Dec<<endpointInfo.nAddress<<":"<<endpointInfo.nEndpoint<<" "<<(nPid==UsbPidOut?" OUT ":(nPid==UsbPidIn?" IN  ":(nPid==UsbPidSetup?"SETUP":"")))<<" "<<nBytes<<Hex);

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
    /*LockGuard<Mutex> guard(m_Mutex);

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
    m_pBase->write32(OHCI_CMD_RUN, OHCI_CMD);*/
}