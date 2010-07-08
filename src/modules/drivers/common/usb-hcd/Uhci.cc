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

Uhci::Uhci(Device* pDev) : Device(pDev), m_pBase(0), m_nPorts(0), m_nFrames(0), m_UhciMR("Uhci-MR")
{
    setSpecificType(String("UHCI"));
    // Allocate the memory region
    if(!PhysicalMemoryManager::instance().allocateRegion(m_UhciMR, 3, PhysicalMemoryManager::continuous, 0))
    {
        ERROR("USB: UHCI: Couldn't allocate memory region!");
        return;
    }
    uint8_t *pVirtBase = static_cast<uint8_t*>(m_UhciMR.virtualAddress());
    m_pFrameList = reinterpret_cast<uint32_t*>(pVirtBase);
    m_pAsyncQH = reinterpret_cast<QH*>(pVirtBase+0x1000);
    m_pPeriodicQH = reinterpret_cast<QH*>(pVirtBase+0x1000+sizeof(QH));
    m_pTDList = reinterpret_cast<TD*>(pVirtBase+0x1000+sizeof(QH)*2);
    m_pTransferPages = pVirtBase+0x2000;

    m_pFrameListPhys = m_UhciMR.physicalAddress();
    m_pAsyncQHPhys = m_UhciMR.physicalAddress()+0x1000;
    m_pPeriodicQHPhys = m_UhciMR.physicalAddress()+0x1000+sizeof(QH);
    m_pTDListPhys = m_UhciMR.physicalAddress()+0x1000+sizeof(QH)*2;
    m_pTransferPagesPhys = m_UhciMR.physicalAddress()+0x2000;

    dmemset(m_pFrameList, m_pAsyncQHPhys | 2, 0x400);
    memset(m_pAsyncQH, 0, sizeof(QH));
    memset(m_pPeriodicQH, 0, sizeof(QH));
    m_pAsyncQH->pNext = m_pPeriodicQHPhys>>4;
    m_pAsyncQH->bNextQH = 1;
    m_pAsyncQH->bElemInvalid = 1;
    m_pPeriodicQH->pNext = m_pAsyncQHPhys>>4;
    m_pPeriodicQH->bNextQH = 1;
    m_pPeriodicQH->bElemInvalid = 1;

    uint32_t nPciCrap = PciBus::instance().readConfigSpace(this, 1);
    NOTICE("USB: UHCI: Pci command+status: "<<nPciCrap);
    PciBus::instance().writeConfigSpace(this, 1, nPciCrap | 7);

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
    // Install the IRQ handler
    Machine::instance().getIrqManager()->registerIsaIrqHandler(getInterruptNumber(), static_cast<IrqHandler*>(this));

    NOTICE("USB: UHCI: Reseted");

    for(size_t i = 0;i<7;i++)
    {
        uint16_t nPortStatus = m_pBase->read16(UHCI_PORTSC+i*2);
        if(nPortStatus == 0xffff || !(nPortStatus & 0x80))
            break;
        NOTICE("USB: UHCI: Port "<<Dec<<i<<Hex<<" got "<<m_pBase->read16(UHCI_PORTSC+i*2));
        if(nPortStatus & UHCI_PORTSC_CONN)
        {
            m_pBase->write16(UHCI_PORTSC_PRES | UHCI_PORTSC_CSCH, UHCI_PORTSC+i*2);
            delay(50);
            m_pBase->write16(0, UHCI_PORTSC+i*2);
            NOTICE("USB: UHCI: Port "<<Dec<<i<<Hex<<" got "<<m_pBase->read16(UHCI_PORTSC+i*2));
            delay(50);
            m_pBase->write16(UHCI_PORTSC_ENABLE/* | UHCI_PORTSC_EDCH*/, UHCI_PORTSC+i*2);
            NOTICE("USB: UHCI: Port "<<Dec<<i<<Hex<<" got "<<m_pBase->read16(UHCI_PORTSC+i*2));
            bool bLoSpeed = m_pBase->read16(UHCI_PORTSC+i*2) & UHCI_PORTSC_LOSPEED;
            NOTICE("USB: UHCI: Port "<<Dec<<i<<Hex<<" is connected at "<<(bLoSpeed?"Low Speed":"Full Speed"));
            deviceConnected(i, bLoSpeed?LowSpeed:FullSpeed);
        }
        m_nPorts++;
    }
}

Uhci::~Uhci()
{
}

bool Uhci::irq(irq_id_t number, InterruptState &state)
{
    uint16_t nStatus = m_pBase->read16(UHCI_STS);
    m_pBase->write16(nStatus, UHCI_STS);
    if(nStatus & UHCI_STS_INT)
    {
        // Pause the controller
        m_pBase->write16(0, UHCI_CMD);
        // Check every async TD
        TD *pTD = m_pAsyncQH->pCurrent;
        while(pTD)
        {
            // Stop if we've reached a TD that is not finished
            if(pTD->nStatus == 0x80)
                break;
            // Call the callback, this TD is done
            ssize_t nReturn = pTD->nStatus & 0x7e?-(pTD->nStatus & 0x7e):(pTD->nActLen + 1) % 0x800;
            if(pTD->nPid == UsbPidIn && pTD->pBuffer && nReturn >= 0)
                memcpy(reinterpret_cast<void*>(pTD->pBuffer), &m_pTransferPages[pTD->pBuff-m_pTransferPagesPhys], nReturn);
            if(pTD->pCallback)
            {
                void (*pCallback)(uintptr_t, ssize_t) = reinterpret_cast<void(*)(uintptr_t, ssize_t)>(pTD->pCallback);
                pCallback(pTD->pParam, nReturn);
            }
            NOTICE("STOP "<<Dec<<pTD->nAddress<<":"<<pTD->nEndpoint<<" "<<(pTD->nPid==UsbPidOut?" OUT ":(pTD->nPid==UsbPidIn?" IN  ":(pTD->nPid==UsbPidSetup?"SETUP":"")))<<" "<<nReturn<<Hex);
            pTD = pTD->bNextInvalid?0:&m_pTDList[pTD->pNext & 0xfff];
        }
        m_pAsyncQH->pCurrent = pTD;
        m_pAsyncQH->pElem = pTD?pTD->pPhysAddr:0;
        m_pAsyncQH->bElemInvalid = pTD?0:1;

        // Check every periodic TD
        pTD = m_pPeriodicQH->pCurrent;
        while(pTD)
        {
            // Stop if we've reached a TD that is not finished
            if(pTD->nStatus == 0x80)
            {
                m_pPeriodicQH->pCurrent = pTD;
                break;
            }
            // Call the callback, this TD is done
            if(!pTD->nStatus)
            {
                ssize_t nReturn = (pTD->nActLen + 1) % 0x800;
                if(pTD->nPid == UsbPidIn && pTD->pBuffer)
                    memcpy(reinterpret_cast<void*>(pTD->pBuffer), &m_pTransferPages[pTD->pBuff-m_pTransferPagesPhys], nReturn);
                if(pTD->pCallback)
                {
                    void (*pCallback)(uintptr_t, ssize_t) = reinterpret_cast<void(*)(uintptr_t, ssize_t)>(pTD->pCallback);
                    pCallback(pTD->pParam, nReturn);
                }
            }
            pTD->nStatus = 0x80;
            // Stop if we reached where we started
            if(pTD->pNext == m_pPeriodicQH->pCurrent->pPhysAddr)
                break;
            pTD = &m_pTDList[pTD->pNext & 0xfff];
        }
        // Resume the controller
        m_pBase->write16(UHCI_CMD_RUN, UHCI_CMD);
    }
    return true;
}

void Uhci::doAsync(UsbEndpoint endpointInfo, uint8_t nPid, uintptr_t pBuffer, uint16_t nBytes, void (*pCallback)(uintptr_t, ssize_t), uintptr_t pParam)
{
    LockGuard<Mutex> guard(m_Mutex);
    NOTICE("START "<<Dec<<endpointInfo.nAddress<<":"<<endpointInfo.nEndpoint<<" "<<endpointInfo.dumpSpeed()<<" "<<(nPid==UsbPidOut?" OUT ":(nPid==UsbPidIn?" IN  ":(nPid==UsbPidSetup?"SETUP":"")))<<" "<<nBytes<<Hex);

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

    asm volatile("sti");
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
}