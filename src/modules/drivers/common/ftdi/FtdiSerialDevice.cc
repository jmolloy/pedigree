/*
 * Copyright (c) 2010 Eduard Burtescu
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

#include <utilities/PointerGuard.h>
#include <usb/UsbDevice.h>
#include <usb/UsbHub.h>
#include <usb/Usb.h>
#include <usb/UsbConstants.h>
#include "FtdiSerialDevice.h"

#define FTDI_BAUD_RATE 9600

static uint8_t nSubdivisors[8] = {0, 3, 2, 4, 1, 5, 6, 7};

FtdiSerialDevice::FtdiSerialDevice(UsbDevice *dev) :
    UsbDevice(dev), Serial(), m_pInEndpoint(0), m_pOutEndpoint(0)
{
}

FtdiSerialDevice::~FtdiSerialDevice()
{
}

void FtdiSerialDevice::initialiseDriver()
{
    // Reset the device
    controlRequest(UsbRequestType::Vendor, 0, 0, 0);

    // Calculate the divisor and subdivisor for the baud rate
    uint16_t nDivisor = (48000000 / 2) / FTDI_BAUD_RATE, nSubdivisor = nSubdivisors[nDivisor % 8];
    nDivisor /= 8;

    // Set the divisor and subdivisor (0x4138 / 0x00 for 9600)
    controlRequest(UsbRequestType::Vendor, 3, (nSubdivisor & 3) << 14 | nDivisor, nSubdivisor >> 2);

    // Get the in and out endpoints
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
        ERROR("USB: FTDI: No IN endpoint");
        return;
    }

    if(!m_pOutEndpoint)
    {
        ERROR("USB: FTDI: No OUT endpoint");
        return;
    }

    // Multiple transfer at the same time test case
#if 0
    UsbHub *pParentHub = dynamic_cast<UsbHub*>(m_pParent);
    UsbEndpoint endpointInfo(m_nAddress, m_nPort, m_pOutEndpoint->nEndpoint, m_Speed, m_pOutEndpoint->nMaxPacketSize);
    for(char i = 'A'; i <= 'Z'; i++)
    {
        char *pChar = new char(i);

        uintptr_t nTransaction = pParentHub->createTransaction(endpointInfo);
        pParentHub->addTransferToTransaction(nTransaction, m_pOutEndpoint->bDataToggle, UsbPidOut, reinterpret_cast<uintptr_t>(pChar), 1);
        pParentHub->doAsync(nTransaction);
        m_pOutEndpoint->bDataToggle = !m_pOutEndpoint->bDataToggle;
    }
#endif

    m_UsbState = HasDriver;
}

char FtdiSerialDevice::read()
{
    char *pChar = new char(0);
    PointerGuard<char> guard(pChar);
    syncIn(m_pInEndpoint, reinterpret_cast<uintptr_t>(pChar), 1);
    return *pChar;
}

void FtdiSerialDevice::write(char c)
{
    char *pChar = new char(c);
    PointerGuard<char> guard(pChar);
    syncOut(m_pOutEndpoint, reinterpret_cast<uintptr_t>(pChar), 1);
}
