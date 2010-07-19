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

#define delay(n) do{Semaphore semWAIT(0);semWAIT.acquire(1, 0, n*1000);}while(0)

Ehci::Ehci(Device* pDev) : Device(pDev), m_TransferPagesAllocator(0, 0x5000), m_EhciMR("Ehci-MR")
{
    setSpecificType(String("EHCI"));

    // Allocate the pages we need
    if(!PhysicalMemoryManager::instance().allocateRegion(m_EhciMR, 9, PhysicalMemoryManager::continuous, VirtualAddressSpace::KernelMode | VirtualAddressSpace::Write))
    {
        ERROR("USB: EHCI: Couldn't allocate Memory Region!");
        return;
    }

    uintptr_t virtualBase   = reinterpret_cast<uintptr_t>(m_EhciMR.virtualAddress());
    uintptr_t physicalBase  = m_EhciMR.physicalAddress();
    m_pQHList               = reinterpret_cast<QH*>(virtualBase);
    m_pFrameList            = reinterpret_cast<uint32_t*>(virtualBase + 0x2000);
    m_pqTDList              = reinterpret_cast<qTD*>(virtualBase + 0x3000);
    m_pTransferPages        = reinterpret_cast<uint8_t*>(virtualBase + 0x4000);
    m_pQHListPhys           = physicalBase;
    m_pFrameListPhys        = physicalBase + 0x2000;
    m_pqTDListPhys          = physicalBase + 0x3000;
    m_pTransferPagesPhys    = physicalBase + 0x4000;

    dmemset(m_pFrameList, 1, 0x400);

#ifdef X86_COMMON
    uint32_t nPciCmdSts = PciBus::instance().readConfigSpace(this, 1);
    NOTICE("USB: EHCI: Pci command: "<<(nPciCmdSts&0xffff));
    PciBus::instance().writeConfigSpace(this, 1, (nPciCmdSts & ~0x4) | 0x4);
#endif

    // Grab the ports
    m_pBase = m_Addresses[0]->m_Io;
    m_nOpRegsOffset = m_pBase->read8(EHCI_CAPLENGTH);

    // Get structural capabilities to determine the number of physical ports
    // we have available to us.
    m_nPorts = m_pBase->read8(EHCI_HCSPARAMS) & 0xF;

    // Don't reset a running controller
    pause();

    delay(5);

    // Write reset command and wait for it to complete
    m_pBase->write32(EHCI_CMD_HCRES, m_nOpRegsOffset + EHCI_CMD);
    while(m_pBase->read32(m_nOpRegsOffset + EHCI_CMD) & EHCI_CMD_HCRES)
        delay(5);
    DEBUG_LOG("USB: EHCI: Reset complete, status: " << m_pBase->read32(m_nOpRegsOffset + EHCI_STS) << ".");

    // Install the IRQ
#ifdef X86_COMMON
    Machine::instance().getIrqManager()->registerIsaIrqHandler(getInterruptNumber(), static_cast<IrqHandler*>(this));
#else
    InterruptManager::instance().registerInterruptHandler(pDev->getInterruptNumber(), this);
#endif

    // Zero the top 64 bits for addresses of EHCI data structures
    m_pBase->write32(0, m_nOpRegsOffset + EHCI_CTRLDSEG);

    // Enable interrupts
    m_pBase->write32(0x3b, m_nOpRegsOffset + EHCI_INTR);

    // Write the base address of the periodic frame list - all T-bits are set to one
    m_pBase->write32(m_pFrameListPhys, m_nOpRegsOffset + EHCI_PERIODICLP);

    delay(5);

    // Set the desired interrupt threshold (frame list size = 4096 bytes)
    m_pBase->write32(0x80000, m_nOpRegsOffset+EHCI_CMD);

    // Turn on the controller
    resume();

    delay(5);

    // Take over the ports
    m_pBase->write32(1, m_nOpRegsOffset+EHCI_CFGFLAG);

    // Set up the RequestQueue
    initialise();

    // Search for ports with devices and initialise them
    for(size_t i = 0; i < m_nPorts; i++)
    {
        DEBUG_LOG("USB: EHCI: Port " << Dec << i << Hex << " - status initially: " << m_pBase->read32(m_nOpRegsOffset+EHCI_PORTSC+i*4));
        if(!(m_pBase->read32(m_nOpRegsOffset+EHCI_PORTSC+i*4) & EHCI_PORTSC_PPOW))
        {
            m_pBase->write32(EHCI_PORTSC_PPOW, m_nOpRegsOffset+EHCI_PORTSC+i*4);
            delay(20);
            DEBUG_LOG("USB: EHCI: Port " << Dec << i << Hex << " - status after power-up: " << m_pBase->read32(m_nOpRegsOffset+EHCI_PORTSC+i*4));
        }

        // If connected, send it to the RequestQueue
        if(m_pBase->read32(m_nOpRegsOffset+EHCI_PORTSC+i*4) & EHCI_PORTSC_CONN)
            addRequest(1, i);
        else
            m_pBase->write32(m_pBase->read32(m_nOpRegsOffset+EHCI_PORTSC+i*4), m_nOpRegsOffset+EHCI_PORTSC+i*4);
    }

    // Enable port status change interrupt and clear it from status
    m_pBase->write32(EHCI_STS_PORTCH, m_nOpRegsOffset + EHCI_STS);
    m_pBase->write32(0x3f, m_nOpRegsOffset + EHCI_INTR);
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
    uint32_t nStatus = m_pBase->read32(m_nOpRegsOffset+EHCI_STS) & m_pBase->read32(m_nOpRegsOffset+EHCI_INTR);
    DEBUG_LOG("IRQ "<<nStatus);
    if(nStatus & EHCI_STS_PORTCH)
        for(size_t i = 0;i < m_nPorts;i++)
            if(m_pBase->read32(m_nOpRegsOffset+EHCI_PORTSC+i*4) & EHCI_PORTSC_CSCH)
                addAsyncRequest(1, i);
    if(nStatus & EHCI_STS_INT)
    {
        pause();
        for(size_t i = 0;i<128;i++)
        {
            if(!m_QHBitmap.test(i))
                continue;
            QH *pQH = &m_pQHList[i];
            qTD *pqTD = &m_pqTDList[i];
            if(pqTD->nStatus == 0x80)
                continue;
            bool bPeriodic = pQH->bNextInvalid;
			if(nStatus & EHCI_STS_ERR)
			{
				ERROR("USB ERROR!");
				ERROR("qTD Status: " << pqTD->nBytes << " [overlay status=" << pQH->nStatus << "]");
				ERROR("qTD Error Counter: " << pqTD->nErr << " [overlay counter=" << pQH->nErr << "]");
				ERROR("QH NAK counter: " << pqTD->res1 << " [overlay count=" << pQH->nNakCounter << "]");
				ERROR("qTD PID: " << pqTD->nPid << ".");
			}
            ssize_t ret = pqTD->nStatus & 0x7c?-((pqTD->nStatus & 0x7c)>>2):pQH->nBufferSize-pqTD->nBytes;
            if(pqTD->nPid == 1 && pQH->pBuffer && ret > 0)
                memcpy(reinterpret_cast<void*>(pQH->pBuffer), &m_pTransferPages[pQH->nBufferOffset], ret);
            if(pQH->pCallback && (ret > 0 || !bPeriodic))
            {
                void (*func)(uintptr_t, ssize_t) = reinterpret_cast<void(*)(uintptr_t, ssize_t)>(pQH->pCallback);
                func(pQH->pParam, ret);
                DEBUG_LOG("STOP "<<Dec<<pQH->nAddress<<":"<<pQH->nEndpoint<<" "<<(pqTD->nPid==0?" OUT ":(pqTD->nPid==1?" IN  ":(pqTD->nPid==2?"SETUP":"")))<<" "<<ret<<Hex);
            }
            if(!bPeriodic)
            {
                m_TransferPagesAllocator.free(pQH->nBufferOffset, pQH->nBufferSize);
                m_QHBitmap.clear(i);
            }
            else
            {
                pqTD->nStatus = 0x80;
                pqTD->nBytes = pQH->nBufferSize;
                pqTD->nPage = pQH->nBufferOffset/0x1000;
                pqTD->nOffset = pQH->nBufferOffset%0x1000;
                pqTD->nErr = 0;
                pqTD->pPage0 = m_pTransferPagesPhys>>12;
                pqTD->pPage1 = (m_pTransferPagesPhys+0x1000)>>12;
                pqTD->pPage2 = (m_pTransferPagesPhys+0x2000)>>12;
                pqTD->pPage3 = (m_pTransferPagesPhys+0x3000)>>12;
                pqTD->pPage4 = (m_pTransferPagesPhys+0x4000)>>12;
                // memcpy(&pQH->overlay, pqTD, sizeof(qTD));
            }
        }
        resume();
    }
    m_pBase->write32(nStatus, m_nOpRegsOffset+EHCI_STS);

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
    while(!(m_pBase->read32(m_nOpRegsOffset + EHCI_STS) & EHCI_STS_HALTED));
}

void Ehci::resume()
{
    // Return if we are already running
    if(!(m_pBase->read32(m_nOpRegsOffset + EHCI_STS) & EHCI_STS_HALTED))
        return;
    // Set run bit and wait until it's running
    m_pBase->write32(m_pBase->read32(m_nOpRegsOffset + EHCI_CMD) | EHCI_CMD_RUN, m_nOpRegsOffset + EHCI_CMD);
    while(m_pBase->read32(m_nOpRegsOffset + EHCI_STS) & EHCI_STS_HALTED);
}

void Ehci::doAsync(UsbEndpoint endpointInfo, uint8_t nPid, uintptr_t pBuffer, uint16_t nBytes, void (*pCallback)(uintptr_t, ssize_t), uintptr_t pParam)
{
    LockGuard<Mutex> guard(m_Mutex);
    DEBUG_LOG("START "<<Dec<<endpointInfo.nAddress<<":"<<endpointInfo.nEndpoint<<" "<<endpointInfo.dumpSpeed()<<" "<<(nPid==UsbPidOut?" OUT ":(nPid==UsbPidIn?" IN  ":(nPid==UsbPidSetup?"SETUP":"")))<<" "<<nBytes<<Hex);

    // Pause the controller
    pause();

    // Get a buffer somewhere in our transfer pages
    uintptr_t nBufferOffset = 0;
    if(nBytes)
    {
        if(!m_TransferPagesAllocator.allocate(nBytes, nBufferOffset))
            FATAL("USB: EHCI: Buffers full :(");
        if(nPid != UsbPidIn && pBuffer)
            memcpy(&m_pTransferPages[nBufferOffset], reinterpret_cast<void*>(pBuffer), nBytes);
    }

    // Get an unused QH
    size_t nQHIndex = m_QHBitmap.getFirstClear();
    if(nQHIndex > 127)
        FATAL("USB: EHCI: QH/qTD space full :(");
    m_QHBitmap.set(nQHIndex);

    QH *pQH = &m_pQHList[nQHIndex];
    memset(pQH, 0, sizeof(QH));
    pQH->pNext = (m_pQHListPhys+nQHIndex*sizeof(QH))>>5;
    pQH->nNextType = 1;
    // pQH->nNakReload = 1;
    pQH->nMaxPacketSize = 8;
    pQH->bControlEndpoint = endpointInfo.speed != HighSpeed && !endpointInfo.nEndpoint;
    pQH->hrcl = 1;
    pQH->bDataToggleSrc = 1;
    pQH->nSpeed = endpointInfo.speed;
    pQH->nEndpoint = endpointInfo.nEndpoint;
    pQH->nAddress = endpointInfo.nAddress;
    pQH->nHubAddress = endpointInfo.speed != HighSpeed?endpointInfo.nHubAddress:0;
    pQH->nHubPort = endpointInfo.speed != HighSpeed?endpointInfo.nHubPort:0;
    pQH->mult = 1;
    pQH->pQTD = (m_pqTDListPhys+nQHIndex*sizeof(qTD))>>5;
    pQH->pCallback = reinterpret_cast<uintptr_t>(pCallback);
    pQH->pParam = pParam;
    pQH->pBuffer = pBuffer;
    pQH->nBufferSize = nBytes;
    pQH->nBufferOffset = nBufferOffset;

    qTD *pqTD = &m_pqTDList[nQHIndex];
    memset(pqTD, 0, sizeof(qTD));
    pqTD->bNextInvalid = pQH->bQNextInvalid = 1;
    pqTD->bAltNextInvalid = pQH->bQAltNextInvalid = 1;
    pqTD->bDataToggle = pQH->bDataToggle = endpointInfo.bDataToggle;
    pqTD->nBytes = pQH->nBytes = nBytes;
    pqTD->bIoc = pQH->bIoc = 1;
    pqTD->nPage = pQH->nPage = nBufferOffset/0x1000;
    pqTD->nErr = pQH->nErr = 1;
    pqTD->nPid = pQH->nPid = nPid==UsbPidOut?0:(nPid==UsbPidIn?1:(nPid==UsbPidSetup?2:3));
    pqTD->nStatus = pQH->nStatus = 0x80;
    pqTD->pPage0 = pQH->pPage0 = m_pTransferPagesPhys>>12;
    pqTD->nOffset = pQH->nOffset = nBufferOffset%0x1000;
    pqTD->pPage1 = pQH->pPage1 = (m_pTransferPagesPhys+0x1000)>>12;
    pqTD->pPage2 = pQH->pPage2 = (m_pTransferPagesPhys+0x2000)>>12;
    pqTD->pPage3 = pQH->pPage3 = (m_pTransferPagesPhys+0x3000)>>12;
    pqTD->pPage4 = pQH->pPage4 = (m_pTransferPagesPhys+0x4000)>>12;

    // memcpy(&pQH->overlay, pqTD, sizeof(qTD));

    // Make sure we've disabled the async schedule
    m_pBase->write32(m_pBase->read32(m_nOpRegsOffset+EHCI_CMD) & ~EHCI_CMD_ASYNCLE, m_nOpRegsOffset+EHCI_CMD);
    while(m_pBase->read32(m_nOpRegsOffset+EHCI_STS) & 0x8000);

    // Write the async list pointer
    m_pBase->write32(m_pQHListPhys+nQHIndex*sizeof(QH), m_nOpRegsOffset+EHCI_ASYNCLP);

    // Start the controller
    resume();

    // Enable async schedule
    m_pBase->write32(m_pBase->read32(m_nOpRegsOffset+EHCI_CMD) | EHCI_CMD_ASYNCLE, m_nOpRegsOffset+EHCI_CMD);
    while(!(m_pBase->read32(m_nOpRegsOffset+EHCI_STS) & 0x8000));
}

void Ehci::addInterruptInHandler(uint8_t nAddress, uint8_t nEndpoint, uintptr_t pBuffer, uint16_t nBytes, void (*pCallback)(uintptr_t, ssize_t), uintptr_t pParam)
{
    LockGuard<Mutex> guard(m_Mutex);

    // Pause the controller
    pause();

    // Make sure we've got the periodic schedule enabled
    if(!(m_pBase->read32(m_nOpRegsOffset+EHCI_STS) & 0x4000))
    {
        // Write the periodic list pointer then enable the period schedule
        m_pBase->write32(m_pFrameListPhys, m_nOpRegsOffset+EHCI_PERIODICLP);
        m_pBase->write32(m_pBase->read32(m_nOpRegsOffset+EHCI_CMD) | EHCI_CMD_PERIODICLE, m_nOpRegsOffset+EHCI_CMD);
    }

    // Get a buffer somewhere in our transfer pages
    uintptr_t nBufferOffset = 0;
    if(!m_TransferPagesAllocator.allocate(nBytes, nBufferOffset))
        FATAL("USB: EHCI: Buffers full :(");

    // Get an unused QH
    size_t nQHIndex = m_QHBitmap.getFirstClear();
    if(nQHIndex > 127)
        FATAL("USB: EHCI: QH/qTD space full :(");
    m_QHBitmap.set(nQHIndex);

    m_pFrameList[nQHIndex] = (m_pQHListPhys+nQHIndex*sizeof(QH))|2;

    QH *pQH = &m_pQHList[nQHIndex];
    memset(pQH, 0, sizeof(QH));
    pQH->bNextInvalid = 1;
    pQH->nMaxPacketSize = 0x400;
    pQH->hrcl = 1;
    pQH->bDataToggleSrc = 1;
    pQH->nSpeed = 2; //endpointInfo.speed;
    pQH->nEndpoint = nEndpoint;
    pQH->nAddress = nAddress;
    pQH->mult = 1;
    pQH->pQTD = (m_pqTDListPhys+nQHIndex*sizeof(qTD))>>5;
    pQH->pCallback = reinterpret_cast<uintptr_t>(pCallback);
    pQH->pParam = pParam;
    pQH->pBuffer = pBuffer;
    pQH->nBufferSize = nBytes;
    pQH->nBufferOffset = nBufferOffset;

    qTD *pqTD = &m_pqTDList[nQHIndex];
    memset(pqTD, 0, sizeof(qTD));
    pqTD->bNextInvalid = 1;
    pqTD->bAltNextInvalid = 1;
    pqTD->bDataToggle = 0;
    pqTD->nBytes = nBytes;
    pqTD->bIoc = 1;
    pqTD->nPage = nBufferOffset/0x1000;
    pqTD->nPid = 1;
    pqTD->nStatus = 0x80;
    pqTD->pPage0 = m_pTransferPagesPhys>>12;
    pqTD->nOffset = nBufferOffset%0x1000;
    pqTD->pPage1 = (m_pTransferPagesPhys+0x1000)>>12;
    pqTD->pPage2 = (m_pTransferPagesPhys+0x2000)>>12;
    pqTD->pPage3 = (m_pTransferPagesPhys+0x3000)>>12;
    pqTD->pPage4 = (m_pTransferPagesPhys+0x4000)>>12;

    // memcpy(&pQH->overlay, pqTD, sizeof(qTD));

    // Write the periodic list frame index
    m_pBase->write32(nQHIndex, m_nOpRegsOffset+EHCI_FRINDEX);

    // Start the controller
    resume();
}

uint64_t Ehci::executeRequest(uint64_t p1, uint64_t p2, uint64_t p3, uint64_t p4, uint64_t p5,
                              uint64_t p6, uint64_t p7, uint64_t p8)
{
    // See if there's any device attached on the port
    if(m_pBase->read32(m_nOpRegsOffset+EHCI_PORTSC+p1*4) & EHCI_PORTSC_CONN)
    {
        while(1)
        {
            pause();

            // Set the reset bit
            m_pBase->write32(m_pBase->read32(m_nOpRegsOffset+EHCI_PORTSC+p1*4) | EHCI_PORTSC_PRES, m_nOpRegsOffset+EHCI_PORTSC+p1*4);
            delay(50);
            // Unset the reset bit
            m_pBase->write32(m_pBase->read32(m_nOpRegsOffset+EHCI_PORTSC+p1*4) & ~EHCI_PORTSC_PRES, m_nOpRegsOffset+EHCI_PORTSC+p1*4);
            // Wait for the reset to complete
            while(m_pBase->read32(m_nOpRegsOffset+EHCI_PORTSC+p1*4) & EHCI_PORTSC_PRES)
                delay(5);
            DEBUG_LOG("USB: EHCI: Port "<<Dec<<p1<<Hex<<" - status after reset: "<<m_pBase->read32(m_nOpRegsOffset+EHCI_PORTSC+p1*4));
            if(m_pBase->read32(m_nOpRegsOffset+EHCI_PORTSC+p1*4) & EHCI_PORTSC_EN)
            {
                DEBUG_LOG("USB: EHCI: Port "<<Dec<<p1<<Hex<<" is now connected");
                deviceConnected(p1, HighSpeed);

                // Check device status
                uint32_t status = m_pBase->read32(m_nOpRegsOffset+EHCI_PORTSC+p1*4);
                if(!(status & EHCI_PORTSC_EN))
                {
                    // Why isn't the port enabled?
                    if(status & 0x400)
                    {
                        WARNING("EHCI: Port " << Dec << p1 << Hex << " is in fact a low-speed device.");
                        break;
                    }
                    else
                    {
                        // Reset states as needed
                        m_pBase->write32(status, m_nOpRegsOffset+EHCI_PORTSC+p1*4);
                        continue; // Reset the port
                    }
                }
                else
                    break;
            }

        }
    }
    else
    {
        DEBUG_LOG("USB: EHCI: Port "<<Dec<<p1<<Hex<<" is now disconnected");
        deviceDisconnected(p1);
        // Clean any bits that would remain
        m_pBase->write32(m_pBase->read32(m_nOpRegsOffset+EHCI_PORTSC+p1*4), m_nOpRegsOffset+EHCI_PORTSC+p1*4);
    }
    return 0;
}
