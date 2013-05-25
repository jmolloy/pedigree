/*
 * Copyright (c) 2010 Matthew Iselin
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include "Dm9601.h"
#include <Log.h>

#include <network-stack/NetworkStack.h>
#include <processor/Processor.h>
#include <usb/UsbHub.h>
#include <usb/UsbConstants.h>

#include <LockGuard.h>

#include <machine/Network.h>

#define delay(n) do{Semaphore semWAIT(0);semWAIT.acquire(1, 0, n*1000);}while(0)

Dm9601::Dm9601(UsbDevice *pDev) :
            UsbDevice(pDev), ::Network(), m_pInEndpoint(0), m_pOutEndpoint(0),
            m_TxLock(false), m_IncomingPackets(false), m_RxPacketQueue(),
            m_RxPacketQueueLock(), m_TxPacket(0)
{
}

Dm9601::~Dm9601()
{
}

void Dm9601::initialiseDriver()
{
    // Grab USB endpoints for the driver to use later
    for(size_t i = 0; i < m_pInterface->endpointList.count(); i++)
    {
        Endpoint *pEndpoint = m_pInterface->endpointList[i];
        if(!m_pInEndpoint && (pEndpoint->nTransferType == Endpoint::Bulk) && pEndpoint->bIn)
            m_pInEndpoint = pEndpoint;
        if(!m_pOutEndpoint && (pEndpoint->nTransferType == Endpoint::Bulk) && pEndpoint->bOut)
            m_pOutEndpoint = pEndpoint;
        if(m_pInEndpoint && m_pOutEndpoint)
            break;
    }

    if(!m_pInEndpoint)
    {
        ERROR("dm9601: no bulk IN endpoint");
        return;
    }

    if(!m_pOutEndpoint)
    {
        ERROR("dm9601: no bulk OUT endpoint");
        return;
    }

    uint8_t *pMac = new uint8_t[6];
    *reinterpret_cast<uint16_t*>(&pMac[0]) = readEeprom(0);
    *reinterpret_cast<uint16_t*>(&pMac[2]) = readEeprom(1);
    *reinterpret_cast<uint16_t*>(&pMac[4]) = readEeprom(2);
    m_StationInfo.mac.setMac(pMac, false);

    NOTICE("DM9601: MAC " << pMac[0] << ":" << pMac[1] << ":" << pMac[2] << ":" <<
                             pMac[3] << ":" << pMac[4] << ":" << pMac[5]);

    // Reset the chip
    writeRegister(NetworkControl, 1);
    delay(100);

    // Select internal MII
    writeRegister(NetworkControl, 0);

    // Enable output on GPIO
    writeRegister(GeneralPurposeCtl, 0x1);

    // Disable GPIO0 - POWER_DOWN
    writeRegister(GeneralPurpose, 0);

    // Enter into the default state - half-duplex, internal PHY
    writeRegister(NetworkControl, 0);

    // Jam the RX line when there's less than 3K in the SRAM space
    writeRegister(BackPressThreshold, 0x37);

    // Flow control: 3K high water overflow, 8K low water overflow
    writeRegister(FlowControl, 0x38);

    // Set up flow control for RX and TX
    writeRegister(RxFlowControl, 1 | (1 << 3) | (1 << 5));

    // Enable proper interrupt handling regardless of NAK state
    writeRegister(UsbControl, 0);

    // Write the physical address of this station to the card so it can filter
    // incoming packets.
    writeRegister(PhysicalAddress, reinterpret_cast<uintptr_t>(pMac), 6);

    // Configure RX control - accept runts, all multicast packets, and turn on
    // the receiver
    writeRegister(RxControl, 5 | (1 << 3));

    // Wait for the link to become active
    /// \todo Timeout
    uint8_t *p = new uint8_t;
    *p = 0;
    while(!*p)
    {
        readRegister(NetworkStatus, reinterpret_cast<uintptr_t>(p), 1);
        delay(100);
    }

    new Thread(Processor::information().getCurrentThread()->getParent(), trampoline, this);
    new Thread(Processor::information().getCurrentThread()->getParent(), recvTrampoline, this);

    NetworkStack::instance().registerDevice(this);

    m_UsbState = HasDriver;
}

int Dm9601::recvTrampoline(void *p)
{
    Dm9601 *pDm9601 = reinterpret_cast<Dm9601*>(p);
    pDm9601->receiveLoop();
    return 0;
}

int Dm9601::trampoline(void *p)
{
    Dm9601 *pDm9601 = reinterpret_cast<Dm9601*>(p);
    pDm9601->receiveThread();
    return 0;
}

void Dm9601::receiveThread()
{
    while(true)
    {
        m_IncomingPackets.acquire();

        m_RxPacketQueueLock.acquire();
        Packet *pPacket = m_RxPacketQueue.popFront();
        m_RxPacketQueueLock.release();

        NetworkStack::instance().receive(pPacket->len, pPacket->buffer + pPacket->offset, this, 0);

        delete pPacket;

        NetworkStack::instance().getMemPool().free(pPacket->buffer);
    }
}

void Dm9601::receiveLoop()
{
    while(true)
        doReceive();
}

bool Dm9601::send(size_t nBytes, uintptr_t buffer)
{
    // Don't let anything else get in our way here
    LockGuard<Mutex> guard(m_TxLock);

    uint8_t *p = new uint8_t;
    readRegister(NetworkStatus, reinterpret_cast<uintptr_t>(p), 1);
    while(*p & (1 << 4))
    {
        delay(100);
        readRegister(NetworkStatus, reinterpret_cast<uintptr_t>(p), 1);
    }

    // Avoid runt packets
    size_t padBytes = 0;
    if(nBytes < 64)
    {
        padBytes = nBytes % 64;
        memset(reinterpret_cast<void*>(buffer + nBytes), 0, padBytes);

        nBytes = 64;
    }

    size_t txSize = nBytes + 2;

    if(!(txSize % 64))
        txSize++;

    // Transmit endpoint
    uint8_t *pBuffer = new uint8_t[txSize];
    *reinterpret_cast<uint16_t*>(pBuffer) = HOST_TO_LITTLE16(static_cast<uint16_t>(nBytes));
    memcpy(&pBuffer[2], reinterpret_cast<void*>(buffer), nBytes);

    ssize_t ret = syncOut(m_pOutEndpoint, reinterpret_cast<uintptr_t>(pBuffer), txSize);
    delete [] pBuffer;

    // Grab the TX status register so we can find errors
    readRegister(TxStatus1 + m_TxPacket, reinterpret_cast<uintptr_t>(p), 1);

    m_TxPacket = (m_TxPacket + 1) % 2;

    // Read and clear the network status (which will contain the "packet complete" indicator)
    readRegister(NetworkStatus, reinterpret_cast<uintptr_t>(p), 1);

    return ret >= 0;
}

#define MAX_MTU 1518

void Dm9601::doReceive()
{
    uintptr_t buff = NetworkStack::instance().getMemPool().allocate();
    ssize_t ret = syncIn(m_pInEndpoint, buff, MAX_MTU, 0); // Never time out.

    if(ret < 0)
    {
        WARNING("dm9601: rx failure due to USB error: " << ret);
        NetworkStack::instance().getMemPool().free(buff);
        return;
    }

    uint8_t *pBuffer = reinterpret_cast<uint8_t*>(buff);
    uint8_t rxstatus = pBuffer[0];
    uint16_t len = LITTLE_TO_HOST16(*reinterpret_cast<uint16_t*>(&pBuffer[1])) - 4;

    if(rxstatus & 0x3F)
    {
        WARNING("dm9601: rx failure: " << rxstatus << ", length was " << len);
        NetworkStack::instance().getMemPool().free(buff);
        badPacket();
        return;
    }

    Packet *pPacket = new Packet;
    pPacket->buffer = buff;
    pPacket->len = len;
    pPacket->offset = 3;

    m_RxPacketQueueLock.acquire();
    m_RxPacketQueue.pushBack(pPacket);
    m_RxPacketQueueLock.release();

    m_IncomingPackets.release();
}

bool Dm9601::setStationInfo(StationInfo info)
{
    // Free the old DNS server list, if there is one
    if(m_StationInfo.dnsServers)
        delete [] m_StationInfo.dnsServers;

    // MAC isn't changeable, so set it all manually
    m_StationInfo.ipv4 = info.ipv4;
    NOTICE("DM9601: Setting ipv4, " << info.ipv4.toString() << ", " << m_StationInfo.ipv4.toString() << "...");
    m_StationInfo.ipv6 = info.ipv6;

    m_StationInfo.subnetMask = info.subnetMask;
    NOTICE("DM9601: Setting subnet mask, " << info.subnetMask.toString() << ", " << m_StationInfo.subnetMask.toString() << "...");
    m_StationInfo.gateway = info.gateway;
    NOTICE("DM9601: Setting gateway, " << info.gateway.toString() << ", " << m_StationInfo.gateway.toString() << "...");

    // Callers do not free their dnsServers memory
    m_StationInfo.dnsServers = info.dnsServers;
    m_StationInfo.nDnsServers = info.nDnsServers;
    NOTICE("DM9601: Setting DNS servers [" << Dec << m_StationInfo.nDnsServers << Hex << " servers being set]...");

    return true;
}

StationInfo Dm9601::getStationInfo()
{
    return m_StationInfo;
}

ssize_t Dm9601::readRegister(uint8_t reg, uintptr_t buffer, size_t nBytes)
{
    if(!buffer || (nBytes > 0xFF))
        return -1;
    return controlRequest(UsbRequestType::Vendor | UsbRequestDirection::In, ReadRegister, 0, reg, nBytes, buffer);
}

ssize_t Dm9601::writeRegister(uint8_t reg, uintptr_t buffer, size_t nBytes)
{
    if(!buffer || (nBytes > 0xFF))
        return -1;
    return controlRequest(UsbRequestType::Vendor | UsbRequestDirection::Out, WriteRegister, 0, reg, nBytes, buffer);
}

ssize_t Dm9601::writeRegister(uint8_t reg, uint8_t data)
{
    return controlRequest(UsbRequestType::Vendor | UsbRequestDirection::Out, WriteRegister1, data, reg);
}

ssize_t Dm9601::readMemory(uint16_t offset, uintptr_t buffer, size_t nBytes)
{
    if(!buffer || (nBytes > 0xFF))
        return -1;
    return controlRequest(UsbRequestType::Vendor | UsbRequestDirection::In, ReadMemory, 0, offset, nBytes, buffer);
}

ssize_t Dm9601::writeMemory(uint16_t offset, uintptr_t buffer, size_t nBytes)
{
    if(!buffer || (nBytes > 0xFF))
        return -1;
    return controlRequest(UsbRequestType::Vendor | UsbRequestDirection::Out, WriteMemory, 0, offset, nBytes, buffer);
}

ssize_t Dm9601::writeMemory(uint16_t offset, uint8_t data)
{
    return controlRequest(UsbRequestType::Vendor | UsbRequestDirection::Out, WriteMemory1, data, offset);
}

uint16_t Dm9601::readEeprom(uint8_t offset)
{
    /// \todo Locking
    uint16_t *ret = new uint16_t;
    writeRegister(PhyAddress, offset);
    writeRegister(PhyControl, 0x4); // Read from EEPROM
    delay(100);
    writeRegister(PhyControl, 0); // Stop the transfer
    readRegister(PhyLowByte, reinterpret_cast<uintptr_t>(ret), 2);

    uint16_t retVal = *ret;
    delete ret;

    return retVal;
}

void Dm9601::writeEeprom(uint8_t offset, uint16_t data)
{
    /// \todo Locking
    uint16_t *input = new uint16_t;
    *input = data;
    writeRegister(PhyAddress, offset);
    writeRegister(PhyLowByte, reinterpret_cast<uintptr_t>(input), 2);
    writeRegister(PhyControl, 0x12); // Write to EEPROM
    delay(100);
    writeRegister(PhyControl, 0);

    delete input;
}

uint16_t Dm9601::readMii(uint8_t offset)
{
    /// \todo Locking
    uint16_t *ret = new uint16_t;
    writeRegister(PhyAddress, offset | 0x40); // External MII starts at 0x40
    writeRegister(PhyControl, 0xc); // Read from PHY
    delay(100);
    writeRegister(PhyControl, 0); // Stop the transfer
    readRegister(PhyLowByte, reinterpret_cast<uintptr_t>(ret), 2);

    uint16_t retVal = *ret;
    delete ret;

    return retVal;
}

void Dm9601::writeMii(uint8_t offset, uint16_t data)
{
    /// \todo Locking
    uint16_t *input = new uint16_t;
    *input = data;
    writeRegister(PhyAddress, offset | 0x40);
    writeRegister(PhyLowByte, reinterpret_cast<uintptr_t>(input), 2);
    writeRegister(PhyControl, 0xa); // Transfer to PHY
    delay(100);
    writeRegister(PhyControl, 0);

    delete input;
}

