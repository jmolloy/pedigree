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

#include <machine/HidInputManager.h>
#include <usb/UsbHub.h>
#include <usb/UsbDevice.h>
#include "UsbHumanInterfaceDevice.h"

#define undefined -0xffffffff

#define _sign(x) (x>=0?"+":"- ")<<(x>=0?x:-x)

static inline uint64_t _getField(uint8_t *pBuffer, size_t nStart, size_t nLength)
{
    uint64_t nValue = 0;
    size_t i = 0;
    while(i < nLength)
    {
        uint8_t nBits = ((nStart % 8) + nLength - i) < 8 ? nLength % 8 : 8 - (nStart % 8);
        nValue |= ((pBuffer[nStart / 8] >> (nStart % 8)) & ((1 << nBits) - 1)) << i;
        i += nBits;
        nStart += nBits;
    }
    return nValue;
}

static inline int64_t _uint2int(uint64_t val, uint8_t len)
{
    // Check for the most significant bit, it shows the sign
    if(val & (1 << (len - 1)))
        return -((1 << len) - val); // Negative value
    return val; // Positive value
}

UsbHumanInterfaceDevice::UsbHumanInterfaceDevice(UsbDevice *pDev) : Device(pDev), UsbDevice(pDev)
{
    HidDescriptor *pHidDescriptor = 0;
    for(size_t i = 0;i < m_pInterface->pOtherDescriptors.count();i++)
    {
        UnknownDescriptor *pDescriptor = m_pInterface->pOtherDescriptors[i];
        if(pDescriptor->nType == 0x21)
        {
            pHidDescriptor = new HidDescriptor(pDescriptor);
            break;
        }
    }
    if(!pHidDescriptor)
    {
        ERROR("USB: HID: No HID descriptor");
        return;
    }


    /// \bug WMware's mouse's second interface is known to cause problems
    if(m_pDescriptor->pDescriptor->nVendorId == 0x0e0f && m_pInterface->pDescriptor->nInterface)
        return;

    // Disable BIOS stuff
    //controlRequest(RequestType::Class | RequestRecipient::Interface, Request::SetInterface, 0, m_pInterface->pDescriptor->nInterface);
    // Set Idle Rate to 0
    controlRequest(RequestType::Class | RequestRecipient::Interface, Request::GetInterface, 0, 0);

    uint16_t nHidSize = pHidDescriptor->pDescriptor->nDescriptorLength;
    uint8_t *pHidReportDescriptor = static_cast<uint8_t*>(getDescriptor(0x22, 0, nHidSize, RequestRecipient::Interface));
    int64_t nLogMin = undefined, nLogMax = undefined, nPhysMin = undefined, nPhysMax = undefined, nUsagePage = undefined,
            nUsageMin = undefined, nUsageMax = undefined, nReportSize = undefined, nReportCount = undefined;
    uint8_t nLogSize = 0;
    uint32_t nBits = 0;
    m_pReport = new HidReport();
    Vector<size_t> *pUsages = new Vector<size_t>();
    for(size_t i = 0; i < nHidSize; i++)
    {
        union
        {
            struct
            {
                uint8_t size : 2;
                uint8_t type : 2;
                uint8_t tag : 4;
            } PACKED;
            uint8_t raw;
        } item;
        item.raw = pHidReportDescriptor[i];
        uint8_t size = item.size == 3 ? 4 : item.size;
        uint32_t value = 0;
        if(size == 1)
            value = pHidReportDescriptor[i + 1];
        else if(size == 2)
            value = pHidReportDescriptor[i + 1] | (pHidReportDescriptor[i + 2] << 8);
        else if(size == 4)
            value = pHidReportDescriptor[i + 1] | (pHidReportDescriptor[i + 2] << 8) | (pHidReportDescriptor[i + 3] << 16) | (pHidReportDescriptor[i + 4] << 24);
#ifdef USB_VERBOSE_DEBUG
        DEBUG_LOG("USB: HID: Item tag=" << item.tag << " type=" << Dec << item.type << " size=" << size << Hex);
#endif
        switch(item.type)
        {
            case 0:
                switch(item.tag)
                {
                    case 0x8:
                        HidReportBlock *pBlock = new HidReportBlock();
                        pBlock->nCount = nReportCount;
                        pBlock->nSize = nReportSize;
                        if(value & 1)
                        {
                            pBlock->type = HidReportBlock::Ignore;
                        }
                        else
                        {
                            pBlock->nUsagePage = nUsagePage;
                            pBlock->nLogSize = nLogSize;
                            if(value & 2)
                            {
                                for(size_t j = 0; pUsages->count() < nReportCount; j++)
                                    pUsages->pushBack(nUsageMin + j);
                                pBlock->pUsages = pUsages;
                                if(value & 4)
                                    pBlock->type = HidReportBlock::Relative;
                                else
                                    pBlock->type = HidReportBlock::Absolute;
                            }
                            else
                            {
                                pBlock->type = HidReportBlock::Array;
                                pBlock->nUsageBase = nUsageMin;
                            }
                        }
                        m_pReport->pBlockList.pushBack(pBlock);
                        nBits += nReportSize * nReportCount;
                        nUsageMin = undefined;
                        nUsageMax = undefined;
                        pUsages = new Vector<size_t>();
                        break;
                }
                break;
            case 1:
                switch(item.tag)
                {
                    case 0x0:
                        nUsagePage = value;
                        pUsages->clear();
                        break;
                    case 0x1:
                        nLogMin = value;
                        break;
                    case 0x2:
                        nLogMax = value;
                        nLogSize = size;
                        break;
                    case 0x3:
                        nPhysMin = value;
                        break;
                    case 0x4:
                        nPhysMax = value;
                        break;
                    case 0x7:
                        nReportSize = value;
                        break;
                    case 0x9:
                        nReportCount = value;
                        break;
                }
                break;
            case 2:
                switch(item.tag)
                {
                    case 0x0:
                        pUsages->pushBack(value);
                        break;
                    case 0x1:
                        nUsageMin = value;
                        break;
                    case 0x2:
                        nUsageMax = value;
                        break;
                }
                break;
        }
        i += size;
    }
    delete pUsages;
    m_pReport->nBytes = nBits / 8;

    // Allocate the report buffers
    m_pReportBuffer = new uint8_t[m_pReport->nBytes];
    m_pOldReportBuffer = new uint8_t[m_pReport->nBytes];
    memset(m_pOldReportBuffer, 0, m_pReport->nBytes);

    Endpoint *pInEndpoint = 0;

    for(size_t i = 0; i < m_pInterface->pEndpoints.count(); i++)
    {
        Endpoint *pEndpoint = m_pInterface->pEndpoints[i];
        if(pEndpoint->nTransferType == Endpoint::Interrupt && pEndpoint->bIn)
        {
            pInEndpoint = pEndpoint;
            break;
        }
    }

    if(!pInEndpoint)
    {
        ERROR("USB: HID: No Interrupt IN endpoint");
        return;
    }

    UsbEndpoint endpointInfo(m_nAddress, m_nPort, pInEndpoint->nEndpoint, m_Speed, pInEndpoint->nMaxPacketSize);
    dynamic_cast<UsbHub*>(m_pParent)->addInterruptInHandler(endpointInfo, reinterpret_cast<uintptr_t>(m_pReportBuffer), m_pReport->nBytes, callback, reinterpret_cast<uintptr_t>(this));

    m_bHasDriver = true;
}

UsbHumanInterfaceDevice::~UsbHumanInterfaceDevice()
{
}

void UsbHumanInterfaceDevice::callback(uintptr_t pParam, ssize_t ret)
{
    UsbHumanInterfaceDevice *pHid = reinterpret_cast<UsbHumanInterfaceDevice*>(pParam);
    pHid->inputHandler();
}

void UsbHumanInterfaceDevice::inputHandler()
{
    //NOTICE("USB: HID: Handling input on interface " << Dec << m_pInterface->pDescriptor->nInterface << Hex);

    size_t nCurrentBit = 0;
    for(List<HidReportBlock*>::Iterator it = m_pReport->pBlockList.begin(); it != m_pReport->pBlockList.end(); it++)
    {
        HidReportBlock *pBlock = *it;
        for(size_t i = 0; pBlock->type != HidReportBlock::Ignore && i < pBlock->nCount; i++)
        {
            uint64_t nValue = _getField(m_pReportBuffer, nCurrentBit + i * pBlock->nSize, pBlock->nSize);
            int64_t nSignedValue;
            switch(pBlock->type)
            {
                case HidReportBlock::Absolute:
                    nSignedValue = nValue - _getField(m_pOldReportBuffer, nCurrentBit + i * pBlock->nSize, pBlock->nSize);

                    if(nSignedValue)
                    {
                        if(pBlock->nUsagePage == 7)
                        {
                            if(nSignedValue > 0)
                                HidInputManager::instance().keyDown((*pBlock->pUsages)[i]);
                            else
                                HidInputManager::instance().keyUp((*pBlock->pUsages)[i]);
                        }
                        else if(pBlock->nUsagePage == 9)
                        {
                            if(nSignedValue > 0)
                                NOTICE("    Button " << Dec << ((*pBlock->pUsages)[i]) << Hex << " down");
                            else
                                NOTICE("    Button " << Dec << ((*pBlock->pUsages)[i]) << Hex << " up");
                        }
                        else
                            NOTICE("    Absolute input, usage " << Dec << pBlock->nUsagePage << ":" << ((*pBlock->pUsages)[i]) << ", value " << _sign(nSignedValue) << " " << pBlock->nSize << Hex);
                    }
                    break;
                case HidReportBlock::Relative:
                    nSignedValue = _uint2int(nValue, pBlock->nLogSize * 8);

                    if(nSignedValue)
                        NOTICE("    Relative input, usage " << Dec << pBlock->nUsagePage << ":" << ((*pBlock->pUsages)[i]) << ", value " << _sign(nSignedValue) << Hex);
                    break;
                case HidReportBlock::Array:

                    if(nValue)
                    {
                        NOTICE("    Array input, usage entry " << Dec << pBlock->nUsagePage << ":" << (pBlock->nUsageBase + nValue) << Hex);

                        // Check if this array entry is new
                        bool bNew = true;
                        for(size_t j = 0; j < pBlock->nCount; j++)
                        {
                            if(_getField(m_pOldReportBuffer, nCurrentBit + j * pBlock->nSize, pBlock->nSize) == nValue)
                            {
                                bNew = false;
                                break;
                            }
                        }
                        if(bNew)
                            if(pBlock->nUsagePage == 7)
                                HidInputManager::instance().keyDown(pBlock->nUsageBase + nValue);
                    }
                case HidReportBlock::Ignore:
                default:
                    break;
            }
        }

        // Special case here: check for array entries that disapeared
        if(pBlock->type == HidReportBlock::Array)
        {
            for(size_t i = 0; i < pBlock->nCount; i++)
            {
                uint64_t nOldValue = _getField(m_pOldReportBuffer, nCurrentBit + i * pBlock->nSize, pBlock->nSize);
                if(nOldValue)
                {
                    // Check if this array entry disapeared
                    bool bDisapeared = true;
                    for(size_t j = 0; j < pBlock->nCount; j++)
                    {
                        if(_getField(m_pReportBuffer, nCurrentBit + j * pBlock->nSize, pBlock->nSize) == nOldValue)
                        {
                            bDisapeared = false;
                            break;
                        }
                    }
                    if(bDisapeared)
                        if(pBlock->nUsagePage == 7)
                            HidInputManager::instance().keyUp(pBlock->nUsageBase + nOldValue);
                }
            }
        }

        nCurrentBit += pBlock->nCount * pBlock->nSize;
    }
    memcpy(m_pOldReportBuffer, m_pReportBuffer, m_pReport->nBytes);
}
