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

#include <machine/Controller.h>
#include <machine/Machine.h>
#include <processor/Processor.h>
#include <processor/InterruptManager.h>
#include <usb/Usb.h>
#include <utilities/assert.h>
#include <Log.h>
#include "Ehci.h"

#define delay(n) do{Semaphore semWAIT(0);semWAIT.acquire(1, 0, n*1000);}while(0)

Ehci::Ehci(Device* pDev) : Device(pDev), m_TransferPagesAllocator(0, 0x5000), m_EhciMR("Ehci-MR")
{
    setSpecificType(String("EHCI"));

    // Allocate the pages we need
    if(!PhysicalMemoryManager::instance().allocateRegion(m_EhciMR, 9, PhysicalMemoryManager::continuous, 0, -1))
    {
        ERROR("USB: EHCI: Couldn't allocate Memory Region!");
        return;
    }

    uintptr_t virtualAddress = reinterpret_cast<uintptr_t>(m_EhciMR.virtualAddress());
    physical_uintptr_t physicalAddress = m_EhciMR.physicalAddress();
    m_pQHListVirt = static_cast<uint8_t*>(virtualAddress);
    m_pFrameList = static_cast<uint32_t*>(virtualAddress + 0x2000);
    m_pqTDListVirt = static_cast<uint8_t*>(virtualAddress + 0x3000);
    m_pTransferPagesVirt = static_cast<uint8_t*>(virtualAddress + 0x4000);
    m_pQHListPhys = physicalAddress;
    m_pFrameListPhys = physicalAddress + 0x2000;
    m_pqTDListPhys = physicalAddress + 0x3000;
    m_pTransferPagesPhys = physicalAddress + 0x4000;
    m_pQHList = reinterpret_cast<QH*>(m_pQHListVirt);
    m_pqTDList = reinterpret_cast<qTD*>(m_pqTDListVirt);

    dmemset(m_pFrameList, 1, 0x400);

    // Grab the ports
    m_pBase = m_Addresses[0]->m_Io;
    m_nOpRegsOffset = m_pBase->read8(EHCI_CAPLENGTH);

    // Get structural capabilities to determine the number of physical ports
    // we have available to us.
    size_t nPorts = m_pBase->read8(EHCI_HCSPARAMS) & 0xF;

    // Don't reset a running controller
    pause();

    delay(5);

    // Write reset command and wait for it to complete
    uint32_t reg = m_pBase->read32(m_nOpRegsOffset + EHCI_CMD);
    reg |= EHCI_CMD_HCRES;
    m_pBase->write32(reg, m_nOpRegsOffset + EHCI_CMD);
    while(m_pBase->read32(m_nOpRegsOffset + EHCI_CMD) & EHCI_CMD_HCRES)
        delay(5);
    NOTICE("USB: EHCI: Reset complete, status: " << m_pBase->read32(m_nOpRegsOffset + EHCI_STS) << ".");

    // Install the IRQ
#ifdef X86_COMMON
    Machine::instance().getIrqManager()->registerIsaIrqHandler(getInterruptNumber(), static_cast<IrqHandler*>(this));
#else
    InterruptManager::instance().registerInterruptHandler(pDev->getInterruptNumber(), this);
#endif

    // Zero the top 64 bits for addresses of EHCI data structures
    m_pBase->write32(0, m_nOpRegsOffset + EHCI_CTRLDSEG);

    // Enable interrupts
    m_pBase->write32(0x3f, m_nOpRegsOffset + EHCI_INTR);

    // Write the base address of the periodic frame list - all T-bits are set to one
    m_pBase->write32(m_pFrameListPhys, m_nOpRegsOffset + EHCI_PERIODICLP);

    delay(5);

    // Enable async list
    //m_pBase->write32(m_pBase->read32(m_nOpRegsOffset+EHCI_CMD) | EHCI_CMD_ASYNCLE, m_nOpRegsOffset+EHCI_CMD);

    // Set the desired interrupt threshold (frame list size = 4096 bytes)
    uint32_t cmd = m_pBase->read32(m_nOpRegsOffset + EHCI_CMD);
    cmd &= ~0xFF0000;
    cmd |= 0x80000; // Maximum interrupt interval is 8 micro-frame

    // Turn on the controller
    resume();

    delay(5);

    // Take over the ports
    m_pBase->write32(1, m_nOpRegsOffset+EHCI_CFGFLAG);

    delay(500);

    // Search for ports with devices and reset them
    for(size_t i = 0; i < nPorts; i++)
    {
        // Reset the port if it's connected
        NOTICE("USB: EHCI: Port "<<Dec<<i<<Hex<<" got "<<m_pBase->read32(m_nOpRegsOffset+EHCI_PORTSC+i*4));
        if(m_pBase->read32(m_nOpRegsOffset+EHCI_PORTSC+i*4) & EHCI_PORTSC_CONN)
        {
            // Set the reset bit
            m_pBase->write32(m_pBase->read32(m_nOpRegsOffset+EHCI_PORTSC+i*4) | EHCI_PORTSC_PRES, m_nOpRegsOffset+EHCI_PORTSC+i*4);
            delay(50);
            // Unset the reset bit
            m_pBase->write32(m_pBase->read32(m_nOpRegsOffset+EHCI_PORTSC+i*4) & ~EHCI_PORTSC_PRES, m_nOpRegsOffset+EHCI_PORTSC+i*4);
            NOTICE("USB: EHCI: Port "<<Dec<<i<<Hex<<" is connected");
            deviceConnected(i);
        }
        m_pBase->write32(m_pBase->read32(m_nOpRegsOffset+EHCI_PORTSC+i*4), m_nOpRegsOffset+EHCI_PORTSC+i*4);
    }
    initialise();
}

Ehci::~Ehci()
{
}

#ifdef X86_COMMON
bool Ehci::irq(irq_id_t number, InterruptState &state)
#else
void Ehci::interrupt(size_t number, InterruptState &state)
#endif
{
    uint32_t status = m_pBase->read32(m_nOpRegsOffset+EHCI_STS);
    NOTICE("IRQ "<<status);
    pause();
    if(status & EHCI_STS_PORTCH)
        for(int i=0;i<8;i++)
            if(m_pBase->read32(m_nOpRegsOffset+EHCI_PORTSC+i*4) & EHCI_PORTSC_CSCH)
                addAsyncRequest(1, i);
    if(status & EHCI_STS_INT)
    {
        for(size_t i = 0;i<128;i++)
        {
            if(!m_QHBitmap.test(i))
                continue;
            QH *pQH = &m_pQHList[i];
            qTD *pqTD = &m_pqTDList[i];
            if(pqTD->status == 0x80)
                continue;
            bool bPeriodic = pQH->bNextInvalid;
            ssize_t ret = pqTD->status & 0x7c?-((pqTD->status & 0x7c)>>2):pQH->size-pqTD->nBytes;
            if(pqTD->nPid == 1 && pQH->pBuffer && ret > 0)
                memcpy(reinterpret_cast<void*>(pQH->pBuffer), &m_pTransferPagesVirt[pQH->offset], ret);
            if(pQH->pCallback && (ret > 0 || !bPeriodic))
            {
                void (*func)(uintptr_t, ssize_t) = reinterpret_cast<void(*)(uintptr_t, ssize_t)>(pQH->pCallback);
                func(pQH->pParam, ret);
                NOTICE("STOP "<<Dec<<pQH->nAddress<<":"<<pQH->nEndpoint<<" "<<(pqTD->nPid==0?" OUT ":(pqTD->nPid==1?" IN  ":(pqTD->nPid==2?"SETUP":"")))<<" "<<ret<<Hex);
            }
            if(!bPeriodic)
            {
                m_TransferPagesAllocator.free(pQH->offset, pQH->size);
                m_QHBitmap.clear(i);
            }
            else
            {
                pqTD->status = 0x80;
                pqTD->nBytes = pQH->size;
                pqTD->cpage = pQH->offset/0x1000;
                pqTD->coff = pQH->offset%0x1000;
                pqTD->cerr = 0;
                pqTD->page0 = m_pTransferPagesPhys>>12;
                pqTD->page1 = (m_pTransferPagesPhys+0x1000)>>12;
                pqTD->page2 = (m_pTransferPagesPhys+0x2000)>>12;
                pqTD->page3 = (m_pTransferPagesPhys+0x3000)>>12;
                pqTD->page4 = (m_pTransferPagesPhys+0x4000)>>12;
                memcpy(&pQH->overlay, pqTD, sizeof(qTD));
            }
        }
    }
    resume();
    m_pBase->write32(status, m_nOpRegsOffset+EHCI_STS);

#ifdef X86_COMMON
    return true;
#endif
}

void Ehci::pause()
{
    // Return if we are already stopped
    if(m_pBase->read32(m_nOpRegsOffset + EHCI_STS) & EHCI_STS_HALTED)
        return;

    // Clear run bit and wait until it's stopped
    m_pBase->write32(m_pBase->read32(m_nOpRegsOffset + EHCI_CMD) & ~EHCI_CMD_RUN, m_nOpRegsOffset + EHCI_CMD);
    while(!(m_pBase->read32(m_nOpRegsOffset + EHCI_STS) & EHCI_STS_HALTED))
        delay(5);
}

void Ehci::resume()
{
    // Return if we are already running
    if(!(m_pBase->read32(m_nOpRegsOffset + EHCI_STS) & EHCI_STS_HALTED))
        return;
    // Set run bit and wait until it's running
    m_pBase->write32(m_pBase->read32(m_nOpRegsOffset + EHCI_CMD) | EHCI_CMD_RUN, m_nOpRegsOffset + EHCI_CMD);
    while(m_pBase->read32(m_nOpRegsOffset + EHCI_STS) & EHCI_STS_HALTED)
        delay(5);
}

void Ehci::doAsync(UsbEndpoint endpointInfo, uint8_t nPid, uintptr_t pBuffer, uint16_t nBytes, void (*pCallback)(uintptr_t, ssize_t), uintptr_t pParam)
{
    LockGuard<Mutex> guard(m_Mutex);
    NOTICE("START "<<Dec<<endpointInfo.nAddress<<":"<<endpointInfo.nEndpoint<<" "<<(nPid==UsbPidOut?" OUT ":(nPid==UsbPidIn?" IN  ":(nPid==UsbPidSetup?"SETUP":"")))<<" "<<nBytes<<Hex);

    // Pause the controller
    pause();

    // Enable async list
    if(!(m_pBase->read32(m_nOpRegsOffset+EHCI_STS) & 0x8000))
        m_pBase->write32(m_pBase->read32(m_nOpRegsOffset+EHCI_CMD) | EHCI_CMD_ASYNCLE, m_nOpRegsOffset+EHCI_CMD);

    // Get a buffer somewhere in our transfer pages
    uintptr_t nBufferOffset = 0;
    if(!m_TransferPagesAllocator.allocate(nBytes, nBufferOffset))
        FATAL("USB: EHCI: Buffers full :(");
    if(nPid != UsbPidIn && pBuffer)
        memcpy(&m_pTransferPagesVirt[nBufferOffset], reinterpret_cast<void*>(pBuffer), nBytes);
    //NOTICE("===> "<<Dec<<nBytes<<"\t"<<nBufferOffset<<Hex);

    // Get an unused QH
    size_t nQHIndex = m_QHBitmap.getFirstClear();
    if(nQHIndex > 127)
        FATAL("USB: EHCI: QH/qTD space full :(");
    m_QHBitmap.set(nQHIndex);
    //NOTICE("===> "<<Dec<<nQHIndex<<Hex);

    QH *pQH = &m_pQHList[nQHIndex];
    memset(pQH, 0, sizeof(QH));
    pQH->next = (m_pQHListPhys>>5)+nQHIndex*2;
    pQH->next_type = 1;
    pQH->nakcrl = 1;
    pQH->ctrlend = 0;
    pQH->maxpacksz = 0x400;
    pQH->hrcl = 1;
    pQH->dtc = 1;
    pQH->speed = 2;
    pQH->nEndpoint = endpointInfo.nEndpoint;
    pQH->inactive_next = 1;
    pQH->nAddress = endpointInfo.nAddress;
    pQH->mult = 1;
    pQH->qtd_ptr = (m_pqTDListPhys>>5)+nQHIndex;
    pQH->pCallback = reinterpret_cast<uintptr_t>(pCallback);
    pQH->pParam = pParam;
    pQH->pBuffer = pBuffer;
    pQH->size = nBytes;
    pQH->offset = nBufferOffset;

    qTD *pqTD = &m_pqTDList[nQHIndex];
    memset(pqTD, 0, sizeof(qTD));
    pqTD->bNextInvalid = 1;
    pqTD->bAltNextInvalid = 1;
    pqTD->data_toggle = endpointInfo.bDataToggle;
    pqTD->nBytes = nBytes;
    pqTD->ioc = 1;
    pqTD->cpage = nBufferOffset/0x1000;
    pqTD->cerr = 1;
    pqTD->nPid = nPid==UsbPidOut?0:(nPid==UsbPidIn?1:(nPid==UsbPidSetup?2:3));
    pqTD->status = 0x80;
    pqTD->page0 = m_pTransferPagesPhys>>12;
    pqTD->coff = nBufferOffset%0x1000;
    pqTD->page1 = (m_pTransferPagesPhys+0x1000)>>12;
    pqTD->page2 = (m_pTransferPagesPhys+0x2000)>>12;
    pqTD->page3 = (m_pTransferPagesPhys+0x3000)>>12;
    pqTD->page4 = (m_pTransferPagesPhys+0x4000)>>12;

    memcpy(&pQH->overlay, pqTD, sizeof(qTD));

    // Write the async list pointer
    m_pBase->write32(m_pQHListPhys+nQHIndex*64, m_nOpRegsOffset+EHCI_ASYNCLP);

    //asm("sti");

    // Start the controller
    resume();
}

void Ehci::addInterruptInHandler(uint8_t nAddress, uint8_t nEndpoint, uintptr_t pBuffer, uint16_t nBytes, void (*pCallback)(uintptr_t, ssize_t), uintptr_t pParam)
{
    LockGuard<Mutex> guard(m_Mutex);

    // Pause the controller
    pause();

    // Enable periodic list
    if(!(m_pBase->read32(m_nOpRegsOffset+EHCI_STS) & 0x4000))
    {
        m_pBase->write32(m_pBase->read32(m_nOpRegsOffset+EHCI_CMD) | EHCI_CMD_PERIODICLE, m_nOpRegsOffset+EHCI_CMD);
        // Write the periodic list pointer
        m_pBase->write32(m_pFrameListPhys, m_nOpRegsOffset+EHCI_PERIODICLP);
    }

    // Get a buffer somewhere in our transfer pages
    uintptr_t nBufferOffset = 0;
    if(!m_TransferPagesAllocator.allocate(nBytes, nBufferOffset))
        FATAL("USB: EHCI: Buffers full :(");
    //NOTICE("===> "<<Dec<<nBytes<<"\t"<<nBufferOffset<<Hex);

    // Get an unused QH
    size_t nQHIndex = m_QHBitmap.getFirstClear();
    if(nQHIndex > 127)
        FATAL("USB: EHCI: QH/qTD space full :(");
    m_QHBitmap.set(nQHIndex);
    //NOTICE("===> "<<Dec<<nQHIndex<<Hex);

    m_pFrameList[nQHIndex] = m_pQHListPhys|((nQHIndex*2)<<5)|2;
    NOTICE("llm "<<m_pFrameList[nQHIndex]);

    QH *pQH = &m_pQHList[nQHIndex];
    memset(pQH, 0, sizeof(QH));
    pQH->bNextInvalid = 1;
    pQH->maxpacksz = 0x400;
    pQH->hrcl = 1;
    pQH->dtc = 1;
    pQH->speed = 2;
    pQH->nEndpoint = nEndpoint;
    pQH->nAddress = nAddress;
    pQH->mult = 1;
    pQH->qtd_ptr = (m_pqTDListPhys>>5)+nQHIndex;
    pQH->pCallback = reinterpret_cast<uintptr_t>(pCallback);
    pQH->pParam = pParam;
    pQH->pBuffer = pBuffer;
    pQH->size = nBytes;
    pQH->offset = nBufferOffset;
    NOTICE("lll "<<m_pqTDListPhys);

    qTD *pqTD = &m_pqTDList[nQHIndex];
    memset(pqTD, 0, sizeof(qTD));
    //pqTD->next = 0;
    pqTD->bNextInvalid = 1;
    pqTD->bAltNextInvalid = 1;
    pqTD->data_toggle = 0;
    pqTD->nBytes = nBytes;
    pqTD->ioc = 1;
    pqTD->cpage = nBufferOffset/0x1000;
    pqTD->nPid = 1;
    pqTD->status = 0x80;
    pqTD->page0 = m_pTransferPagesPhys>>12;
    pqTD->coff = nBufferOffset%0x1000;
    pqTD->page1 = (m_pTransferPagesPhys+0x1000)>>12;
    pqTD->page2 = (m_pTransferPagesPhys+0x2000)>>12;
    pqTD->page3 = (m_pTransferPagesPhys+0x3000)>>12;
    pqTD->page4 = (m_pTransferPagesPhys+0x4000)>>12;

    memcpy(&pQH->overlay, pqTD, sizeof(qTD));

    // Write the periodic list frame index
    m_pBase->write32(nQHIndex, m_nOpRegsOffset+EHCI_FRINDEX);

    //asm("sti");

    // Start the controller
    resume();
}

uint64_t Ehci::executeRequest(uint64_t p1, uint64_t p2, uint64_t p3, uint64_t p4, uint64_t p5,
                              uint64_t p6, uint64_t p7, uint64_t p8)
{
    NOTICE("USB: EHCI: Port status change on port "<<Dec<<(p1+1)<<Hex<<", now "<<(m_pBase->read32(m_nOpRegsOffset+EHCI_PORTSC+p1*4)&EHCI_PORTSC_CONN?"connected":"disconnected"));
    // Reset the port if it's connected
    if(m_pBase->read32(m_nOpRegsOffset+EHCI_PORTSC+p1*4) & EHCI_PORTSC_CONN)
    {
        // Set the reset bit
        m_pBase->write32(m_pBase->read32(m_nOpRegsOffset+EHCI_PORTSC+p1*4) | EHCI_PORTSC_PRES, m_nOpRegsOffset+EHCI_PORTSC+p1*4);
        delay(50);
        // Unset the reset bit
        m_pBase->write32(m_pBase->read32(m_nOpRegsOffset+EHCI_PORTSC+p1*4) & ~EHCI_PORTSC_PRES, m_nOpRegsOffset+EHCI_PORTSC+p1*4);
        deviceConnected(p1);
    }
    else
        deviceDisconnected(p1);
    m_pBase->write32(m_pBase->read32(m_nOpRegsOffset+EHCI_PORTSC+p1*4), m_nOpRegsOffset+EHCI_PORTSC+p1*4);
    return 0;
}
