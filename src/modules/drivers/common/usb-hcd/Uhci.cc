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
#include <usb/UsbConstants.h>
#include <usb/UsbController.h>
#include <Log.h>
#include "Uhci.h"

#define delay(n) do{Semaphore semWAIT(0);semWAIT.acquire(1, 0, n*1000);}while(0)

Uhci::Uhci(Device* pDev) :
    Device(pDev), m_pBase(0), m_nPorts(0), m_nFrames(0), m_pTDList(0), m_pTDListPhys(0), m_pAsyncQH(0), m_pAsyncQHPhys(0),
    m_pFrameList(0), m_pFrameListPhys(0), m_pTransferPages(0), m_pTransferPagesPhys(0), m_UhciMR("Uhci-MR")
{
    setSpecificType(String("UHCI"));
    // Allocate the memory region
    if(!PhysicalMemoryManager::instance().allocateRegion(m_UhciMR, 3, PhysicalMemoryManager::continuous, 0, -1))
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
    m_pAsyncQH->next = m_pPeriodicQHPhys>>4;
    m_pAsyncQH->next_qh = 1;
    m_pAsyncQH->elem_invalid = 1;
    m_pPeriodicQH->next = m_pAsyncQHPhys>>4;
    m_pPeriodicQH->next_qh = 1;
    m_pPeriodicQH->elem_invalid = 1;

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
        if(nPortStatus & UHCI_PORTSC_CONN)
        {
            m_pBase->write16(UHCI_PORTSC_PRES | UHCI_PORTSC_ENABLE, UHCI_PORTSC+i*2);
            delay(50);
            m_pBase->write16(UHCI_PORTSC_ENABLE | UHCI_PORTSC_CSCH | UHCI_PORTSC_EDCH, UHCI_PORTSC+i*2);
            delay(50);
            m_pBase->write16(UHCI_PORTSC_ENABLE | UHCI_PORTSC_CSCH | UHCI_PORTSC_EDCH, UHCI_PORTSC+i*2);
            NOTICE("USB: UHCI: Port "<<Dec<<i<<Hex<<" is connected");
            deviceConnected(i);
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
            if(pTD->status == 0x80)
                break;
            // Call the callback, this TD is done
            ssize_t nReturn = pTD->status & 0x7e?-(pTD->status & 0x7e):(pTD->actlen + 1) % 0x800;
            if(pTD->nPid == USB_PID_IN && pTD->buffer && nReturn >= 0)
                memcpy(reinterpret_cast<void*>(pTD->buffer), &m_pTransferPages[pTD->buff-m_pTransferPagesPhys], nReturn);
            if(pTD->pCallback)
            {
                void (*pCallback)(uintptr_t, ssize_t) = reinterpret_cast<void(*)(uintptr_t, ssize_t)>(pTD->pCallback);
                pCallback(pTD->param, nReturn);
            }
            NOTICE("STOP "<<Dec<<pTD->nAddress<<":"<<pTD->nEndpoint<<" "<<(pTD->nPid==USB_PID_OUT?" OUT ":(pTD->nPid==USB_PID_IN?" IN  ":(pTD->nPid==USB_PID_SETUP?"SETUP":"")))<<" "<<nReturn<<Hex);
            pTD = pTD->next_invalid?0:&m_pTDList[pTD->next & 0xfff];
        }
        m_pAsyncQH->pCurrent = pTD;
        m_pAsyncQH->elem = pTD?pTD->phys:0;
        m_pAsyncQH->elem_invalid = pTD?0:1;

        // Check every periodic TD
        pTD = m_pPeriodicQH->pCurrent;
        while(pTD)
        {
            // Stop if we've reached a TD that is not finished
            if(pTD->status == 0x80)
            {
                m_pPeriodicQH->pCurrent = pTD;
                break;
            }
            // Call the callback, this TD is done
            if(!pTD->status)
            {
                ssize_t nReturn = (pTD->actlen + 1) % 0x800;
                if(pTD->nPid == USB_PID_IN && pTD->buffer)
                    memcpy(reinterpret_cast<void*>(pTD->buffer), &m_pTransferPages[pTD->buff-m_pTransferPagesPhys], nReturn);
                if(pTD->pCallback)
                {
                    void (*pCallback)(uintptr_t, ssize_t) = reinterpret_cast<void(*)(uintptr_t, ssize_t)>(pTD->pCallback);
                    pCallback(pTD->param, nReturn);
                }
            }
            pTD->status = 0x80;
            // Stop if we reached where we started
            if(pTD->next == m_pPeriodicQH->pCurrent->phys)
                break;
            pTD = &m_pTDList[pTD->next & 0xfff];
        }
        // Resume the controller
        m_pBase->write16(UHCI_CMD_RUN, UHCI_CMD);
    }
    return true;
}

void Uhci::doAsync(uint8_t nAddress, uint8_t nEndpoint, uint8_t nPid, uintptr_t pBuffer, uint16_t nBytes, void (*pCallback)(uintptr_t, ssize_t), uintptr_t pParam)
{
    LockGuard<Mutex> guard(m_Mutex);
    NOTICE("START "<<Dec<<nAddress<<":"<<nEndpoint<<" "<<(nPid==USB_PID_OUT?" OUT ":(nPid==USB_PID_IN?" IN  ":(nPid==USB_PID_SETUP?"SETUP":"")))<<" "<<nBytes<<Hex);

    // Pause the controller
    m_pBase->write16(0, UHCI_CMD);

    // Copy the buffer, if needed
    if(nPid != USB_PID_IN && pBuffer)
        memcpy(m_pTransferPages, reinterpret_cast<void*>(pBuffer), nBytes);

    // Fill the TD with data
    TD *pTD = &m_pTDList[0];
    memset(pTD, 0, sizeof(TD));
    pTD->next = m_pAsyncQH->pCurrent?m_pAsyncQH->pCurrent->phys:0;
    pTD->next_invalid = m_pAsyncQH->pCurrent?0:1;
    pTD->status = 0x80;
    pTD->ioc = 1;
    pTD->speed = 1;
    pTD->cerr = 1;
    pTD->spd = 0;
    pTD->nPid = nPid;
    pTD->nAddress = nAddress;
    pTD->nEndpoint = nEndpoint;
    static bool dt;
    if(nPid == USB_PID_SETUP)
        dt = 0;
    else
        dt = !dt;
    pTD->data_toggle = nBytes?dt:1;
    pTD->maxlen = nBytes?nBytes-1:0x7ff;
    pTD->buff = m_pTransferPagesPhys;
    pTD->phys = (m_pTDListPhys+0*sizeof(TD))>>4;
    pTD->pCallback = reinterpret_cast<uintptr_t>(pCallback);
    pTD->param = pParam;
    pTD->buffer = pBuffer;

    // Change our async QH to include the new TD
    m_pAsyncQH->elem = pTD->phys;
    m_pAsyncQH->elem_invalid = 0;
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
    pTD->phys = (m_pTDListPhys+1*sizeof(TD))>>4;
    pTD->next = m_pPeriodicQH->pCurrent?m_pPeriodicQH->pCurrent->next:pTD->phys;
    pTD->status = 0x80;
    pTD->ioc = 1;
    pTD->speed = 1;
    pTD->spd = 0;
    pTD->nPid = USB_PID_IN;
    pTD->nAddress = nAddress;
    pTD->nEndpoint = nEndpoint;
    pTD->data_toggle = 0;
    pTD->maxlen = nBytes?nBytes-1:0x7ff;
    pTD->buff = m_pTransferPagesPhys;
    pTD->pCallback = reinterpret_cast<uintptr_t>(pCallback);
    pTD->param = pParam;
    pTD->buffer = pBuffer;

    // Change our periodic QH to include the new TD
    if(m_pPeriodicQH->pCurrent)
        m_pPeriodicQH->pCurrent->next = pTD->phys;
    else
    {
        m_pPeriodicQH->elem = pTD->phys;
        m_pPeriodicQH->elem_invalid = 0;
    }
    m_pPeriodicQH->pCurrent = pTD;

    // Resume the controller
    m_pBase->write16(UHCI_CMD_RUN, UHCI_CMD);
}