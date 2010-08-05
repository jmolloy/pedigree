/*
 * Copyright (c) 2009 Eduard Burtescu
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
#include "Rtl8139.h"
#include "Rtl8139Constants.h"
#include <Log.h>
#include <machine/Machine.h>
#include <machine/Network.h>
#include <network-stack/NetworkStack.h>
#include <processor/Processor.h>
#include <machine/IrqManager.h>


Rtl8139::Rtl8139(Network* pDev) :
    Network(pDev), m_pBase(0), m_StationInfo(), m_RxCurr(0), m_TxCurr(0), m_RxLock(false), m_TxLock(), m_pRxBuffVirt(0), m_pTxBuffVirt(0),
    m_pRxBuffPhys(0), m_pTxBuffPhys(0), m_RxBuffMR("rtl8139-rxbuffer"), m_TxBuffMR("rtl8139-txbuffer")
{
    setSpecificType(String("rtl8139-card"));

    // allocate the rx and tx buffers
    if(!PhysicalMemoryManager::instance().allocateRegion(m_RxBuffMR, RTL_BUFF_SIZE / PAGE_SIZE + (RTL_BUFF_SIZE % PAGE_SIZE ? 1 : 0), PhysicalMemoryManager::continuous, 0, -1))
    {
        ERROR("RTL8139: Couldn't allocate Rx Buffer!");
        return;
    }
    if(!PhysicalMemoryManager::instance().allocateRegion(m_TxBuffMR, RTL_BUFF_SIZE / PAGE_SIZE + (RTL_BUFF_SIZE % PAGE_SIZE ? 1 : 0), PhysicalMemoryManager::continuous, 0, -1))
    {
        ERROR("RTL8139: Couldn't allocate Tx Buffer!");
        delete &m_RxBuffMR;
        return;
    }
    m_pRxBuffVirt = static_cast<uint8_t *>(m_RxBuffMR.virtualAddress());
    m_pTxBuffVirt = static_cast<uint8_t *>(m_TxBuffMR.virtualAddress());
    m_pRxBuffPhys = m_RxBuffMR.physicalAddress();
    m_pTxBuffPhys = m_TxBuffMR.physicalAddress();

    // grab the ports
    m_pBase = m_Addresses[0]->m_Io;

    // reset the card
    reset();

    // get the mac
    for(int i = 0; i < 6; i++)
        m_StationInfo.mac.setMac(m_pBase->read16(RTL_MAC + i), i);

    WARNING("RTL8139: MAC is " <<
    m_StationInfo.mac[0] << ":" <<
    m_StationInfo.mac[1] << ":" <<
    m_StationInfo.mac[2] << ":" <<
    m_StationInfo.mac[3] << ":" <<
    m_StationInfo.mac[4] << ":" <<
    m_StationInfo.mac[5] << ".");

    // install the IRQ and register the NIC in the stack
    Machine::instance().getIrqManager()->registerIsaIrqHandler(getInterruptNumber(), static_cast<IrqHandler *>(this));
    NetworkStack::instance().registerDevice(this);
}

Rtl8139::~Rtl8139()
{
}

void Rtl8139::reset()
{
    // reset current offsets
    m_RxCurr = 0;
    m_TxCurr = 0;

    // clear buffers
    memset(m_pRxBuffVirt, 0, RTL_BUFF_SIZE);
    memset(m_pTxBuffVirt, 0, RTL_BUFF_SIZE);

    // reset command and wait for complete
    m_pBase->write8(RTL_CMD_RES, RTL_CMD);
    while(m_pBase->read8(RTL_CMD) & RTL_CMD_RES);

    // unlock BMCR registers
    m_pBase->write8(RTL_CFG9346_UNLOCK, RTL_CFG9346);

    // enable Rx and Tx
    m_pBase->write8(RTL_CMD_RXEN | RTL_CMD_TXEN, RTL_CMD);

    // get the card out of low power mode
    m_pBase->write8(0x00, RTL_CFG1);

    // write the RxBuffer's address
    m_pBase->write32(static_cast<uint32_t>(m_pRxBuffPhys), RTL_RXBUFF);

    // no missed packets
    m_pBase->write8(0x00, RTL_RXMIS);

    // BMCR options
    m_pBase->write16(RTL_BMCR_SPEED | RTL_BMCR_ANE | RTL_BMCR_DUPLEX, RTL_BMCR);

    // MSR options
    m_pBase->write8(RTL_MSR_RXFCE, RTL_MSR);

    // write rx and tx config
    m_pBase->write32(RTL_RXCFG_FTH_NONE | RTL_RXCFG_RBLN_64K | RTL_RXCFG_MDMA_UNLM | RTL_RXCFG_AR | RTL_RXCFG_AB | RTL_RXCFG_AM | RTL_RXCFG_APM | RTL_RXCFG_AAP, RTL_RXCFG);
    m_pBase->write32(RTL_TXCFG_MDMA_2K | RTL_TXCFG_RR_48, RTL_TXCFG);

    // write multicast addresses
    m_pBase->write32(0xFFFFFFFF, RTL_MAR);
    m_pBase->write32(0xFFFFFFFF, RTL_MAR + 4);

    // lock BCMR registers
    m_pBase->write8(RTL_CFG9346_LOCK, RTL_CFG9346);

    // enable Rx and Tx
    m_pBase->write8(RTL_CMD_RXEN | RTL_CMD_TXEN, RTL_CMD);

    // enable all good irqs
    m_pBase->write16(RTL_IMR_RXOK | RTL_IMR_RXERR, RTL_IMR);
    m_pBase->write16(0xffff, RTL_ISR);
    m_RxLock = false;
    NOTICE("RTL8139: Reset");
}

bool Rtl8139::send(size_t nBytes, uintptr_t buffer)
{
    LockGuard<Spinlock> guard(m_TxLock);

    if(nBytes > RTL_PACK_MAX)
    {
        ERROR("RTL8139: Attempt to send a packet with size > 64 KB");
        return false;
    }

    // copy to the buffer and pad the packet
    memcpy(m_pTxBuffVirt, reinterpret_cast<void *>(buffer), nBytes);
    for(int i = nBytes;i < RTL_BUFF_SIZE;i++)
        m_pTxBuffVirt[i] = 0;

    // address & status for the write
    m_pBase->write32(static_cast<uint32_t>(m_pTxBuffPhys), RTL_TXADDR0 + m_TxCurr * 4);
    m_pBase->write32(0x3F0000 | (nBytes & 0x1FFF), RTL_TXSTS0 + m_TxCurr * 4);

    // next descriptor, or go to 0 if 4 or more
    m_TxCurr++;
    m_TxCurr %= 4;
    return true;
}

void Rtl8139::recv()
{
    while(m_RxLock);
    m_RxLock = true;

    // get the address of the start of the packet;
    uintptr_t rxPacket = reinterpret_cast<uintptr_t>(m_pRxBuffVirt + m_RxCurr);
    uint16_t status = *(reinterpret_cast<uint16_t *>(rxPacket));
    // get the status and the lenght, both at the beginning of the packet
    uint16_t length = *(reinterpret_cast<uint16_t *>(rxPacket+2));

    // if bad packet, reset
    if(!(status & RTL_RXSTS_RXOK) || (status & (RTL_RXSTS_ISE | RTL_RXSTS_CRC | RTL_RXSTS_FAE)) || (length >= RTL_PACK_MAX) || (length < RTL_PACK_MIN))
    {
        WARNING("RTL8139: Bad packet: len: " << length << ", status: " << status << "!");
        reset();
        return;
    }
    // create packet buffer, needed later
    uint8_t *packBuff = new uint8_t[length - 4];

    // check if passing over the end of the buffer
    if(m_RxCurr + length > RTL_BUFF_SIZE)
    {
        // copy first the part of the packet until the end of the buffer and then the rest of it, at the beginning of the buffer
        uint32_t left = RTL_BUFF_SIZE - (m_RxCurr + 4);
        memcpy(packBuff, reinterpret_cast<void *>(rxPacket + 4), left);
        memcpy(&packBuff[left], m_pRxBuffVirt, length - 4 - left);

        // adjust current offset
        m_RxCurr = (length - left + 3) & ~3;
    }
    else
    {
        // copy the packet
        memcpy(packBuff, reinterpret_cast<void *>(rxPacket + 4), length-4);

        // adjust current offset
        m_RxCurr = (m_RxCurr + length + 4 + 3) & ~3;
    }
    // adjust current offset (it never should be over the buffer's size)
    m_RxCurr %= RTL_BUFF_SIZE;

    // send the packet to the stack
    NetworkStack::instance().receive(length-4, reinterpret_cast<uintptr_t>(packBuff), this, 0);

    m_RxLock = false;
}

bool Rtl8139::setStationInfo(StationInfo info)
{
    // free the old DNS servers list, if there is one
    if(m_StationInfo.dnsServers)
        delete [] m_StationInfo.dnsServers;

    // MAC isn't changeable, so set it all manually
    m_StationInfo.ipv4 = info.ipv4;
    NOTICE("RTL8139: Setting ipv4, " << info.ipv4.toString() << ", " << m_StationInfo.ipv4.toString() << "...");
    m_StationInfo.ipv6 = info.ipv6;

    m_StationInfo.subnetMask = info.subnetMask;
    NOTICE("RTL8139: Setting subnet mask, " << info.subnetMask.toString() << ", " << m_StationInfo.subnetMask.toString() << "...");
    m_StationInfo.gateway = info.gateway;
    NOTICE("RTL8139: Setting gateway, " << info.gateway.toString() << ", " << m_StationInfo.gateway.toString() << "...");

    // Callers do not free their dnsServers memory
    m_StationInfo.dnsServers = info.dnsServers;
    m_StationInfo.nDnsServers = info.nDnsServers;
    NOTICE("RTL8139: Setting DNS servers [" << Dec << m_StationInfo.nDnsServers << Hex << " servers being set]...");

    return true;
}

StationInfo Rtl8139::getStationInfo()
{
    return m_StationInfo;
}

bool Rtl8139::isConnected()
{
    return !(m_pBase->read8(RTL_MSR)&RTL_MSR_LINK);
}

bool Rtl8139::irq(irq_id_t number, InterruptState &state)
{
    while(true)
    {
        // grab the interrupt status and acknowledge it ASAP
        uint16_t irqStatus = m_pBase->read16(RTL_ISR);
        m_pBase->write16(irqStatus, RTL_ISR);

        //no irq left, break
        if((irqStatus & (RTL_ISR_RXOK|RTL_ISR_TXOK|RTL_ISR_RXERR|RTL_ISR_TXERR)) == 0)
            break;

        // RxOK, receive the packet
        if(irqStatus & RTL_ISR_RXOK)
            recv();

        // if rx error, reset
        if(irqStatus & RTL_ISR_RXERR)
        {
            WARNING("RTL8139: Rx error!");
            reset();
        }
        // if tx error, reset
        if(irqStatus & RTL_ISR_TXERR)
        {
            WARNING("RTL8139: Tx error!");
            reset();
        }
    }
    return true;
}
