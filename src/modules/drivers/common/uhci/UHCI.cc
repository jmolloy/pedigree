/*
* Copyright (c) 2008 James Molloy, Jörg Pfähler, Matthew Iselin, Eduard Burtescu
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

#include <Log.h>
#include <machine/Machine.h>
#include <processor/Processor.h>
#include <usb/UsbConstants.h>
#include <usb/UsbManager.h>
#include "UHCI.h"
#include <usb/UsbController.h>

UHCI::UHCI(Device* pDev) :
Device(pDev), m_pBase(0), m_inTransaction(false), m_pTDList(0), m_pTDListVirt(0), m_pTDListPhys(0),
m_pFrameList(0), m_pFrmListVirt(0),m_pFrmListPhys(0), m_FrmListMR("UHCI-framelist")
{
    setSpecificType(String("UHCI"));
    // allocate the rx and tx buffers
    if(!PhysicalMemoryManager::instance().allocateRegion(m_FrmListMR, 2, PhysicalMemoryManager::continuous, 0, -1))
    {
        ERROR("USB: UHCI: Couldn't allocate Frame List!");
        return;
    }
    m_pFrmListVirt = static_cast<uint8_t *>(m_FrmListMR.virtualAddress());
    m_pTDListVirt = static_cast<uint8_t *>(m_pFrmListVirt+0x1000);
    m_pFrmListPhys = m_FrmListMR.physicalAddress();
    m_pTDListPhys = m_FrmListMR.physicalAddress()+0x1000;
    memset(m_pFrmListVirt, 0xff, 0x1000);
    m_pFrameList = reinterpret_cast<uint32_t *>(m_pFrmListVirt);
    m_pTDList = reinterpret_cast<uint32_t *>(m_pTDListVirt);
    // grab the ports
    m_pBase = m_Addresses[0]->m_Io;
    m_pBase->write16(UHCI_CMD_GRES, UHCI_CMD);
    m_pBase->write32(m_pFrmListPhys, UHCI_FRLP);
    m_pBase->write16(0xf, UHCI_INTR);
    m_pBase->write16(1<<9, 0x10);
    m_pBase->write16(1<<2, 0x10);
    m_pBase->write16(1<<9, 0x12);
    m_pBase->write16(1<<2, 0x12);
    NOTICE("USB: UHCI: Reseted");

    // install the IRQ and set UHCI as default controller
    Machine::instance().getIrqManager()->registerIsaIrqHandler(getInterruptNumber(), static_cast<IrqHandler *>(this));
    UsbManager::instance().setController(this);
    UsbManager::instance().enumerate();
}

UHCI::~UHCI()
{
}

ssize_t UHCI::setup(uint8_t addr, UsbSetup *setup)
{
    while(m_inTransaction);
    m_pFrameList[1]=m_pTDListPhys;
    TD *stp_td = reinterpret_cast<TD *>(m_pTDList);
    stp_td->link = 1;
    stp_td->ctrl = 1<<24|1<<23;
    stp_td->token = 7<<21|addr<<8|USB_PID_SETUP;
    stp_td->buff = m_pTDListPhys+32;
    memcpy(&m_pTDListVirt[32], setup, 8);
    m_pBase->write16(0, UHCI_FRMN);
    m_inTransaction = true;
    m_pBase->write16(UHCI_CMD_RUN, UHCI_CMD);
    while(m_inTransaction);
    if(stp_td->ctrl&0x7e0000)
        return -((stp_td->ctrl&0x7e0000)>>17);
    return (stp_td->ctrl&0x7ff)+1;
}

ssize_t UHCI::in(uint8_t addr, uint8_t endpoint, void *buff, size_t size)
{
    while(m_inTransaction);
    m_pFrameList[1]=m_pTDListPhys;
    TD *in_td = reinterpret_cast<TD *>(m_pTDList);
    in_td->link = 1;
    in_td->ctrl = 1<<24|1<<23;
    in_td->token = (size-1)<<21|endpoint<<15|addr<<8|USB_PID_IN;
    in_td->buff = m_pTDListPhys+32;
    m_pBase->write16(0, UHCI_FRMN);
    m_inTransaction = true;
    m_pBase->write16(UHCI_CMD_RUN, UHCI_CMD);
    while(m_inTransaction);
    if(in_td->ctrl&0x7e0000)
        return -((in_td->ctrl&0x7e0000)>>17);
    size_t act_size = ((in_td->ctrl&0x7ff)+1)%0x800;
    memcpy(buff, &m_pTDListVirt[32], act_size);
    return act_size;
}

ssize_t UHCI::out(uint8_t addr, void *buff, size_t size)
{
    while(m_inTransaction);
    memcpy(buff, &m_pTDListVirt[32], size);
    m_pFrameList[1]=m_pTDListPhys;
    TD *in_td = reinterpret_cast<TD *>(m_pTDList);
    in_td->link = 1;
    in_td->ctrl = 1<<24|1<<23;
    in_td->token = (size-1)<<21|addr<<8|USB_PID_OUT;
    in_td->buff = m_pTDListPhys+32;
    m_pBase->write16(0, UHCI_FRMN);
    m_inTransaction = true;
    m_pBase->write16(UHCI_CMD_RUN, UHCI_CMD);
    while(m_inTransaction);
    if(in_td->ctrl&0x7e0000)
        return -((in_td->ctrl&0x7e0000)>>17);
    return ((in_td->ctrl&0x7ff)+1)%0x800;
}

bool UHCI::irq(irq_id_t number, InterruptState &state)
{
    m_inTransaction = false;
    m_pBase->write16(m_pBase->read16(UHCI_STS), UHCI_STS);
    m_pBase->write16(0, UHCI_CMD);
    return true;
}
