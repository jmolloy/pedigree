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

//#include <machine/Machine.h>
//#include <machine/Timer.h>
#include <processor/state.h>
//#include <utilities/utility.h>
#include <usb/UsbHub.h>
#include <usb/UsbDevice.h>
#include <usb/UsbHumanInterfaceDevice.h>
#include <machine/InputManager.h>

// Don't interfere with the other keymaps

#define data_buff usb_data_buff
#define sparse_buff usb_sparse_buff
#define undefined -0xffffffff
#include <machine/keymaps/UsbKeyboardEnUs.h>

#define _sign(x) (x>=0?"+":"- ")<<(x>=0?x:-x)

static inline uint32_t _getField(uint8_t *pBuffer, size_t nStart, size_t nLength)
{
    uint64_t nValue = 0;
    uint8_t i = 0;
    for(uint8_t i = 0;i < nLength;)
    {
        uint8_t nBits = ((nStart % 8) + nLength - i) < 8?nLength % 8:8 - (nStart % 8);
        nValue |= ((pBuffer[nStart / 8] >> (nStart % 8)) & ((1 << nBits) - 1)) << i;
        i += nBits;
        nStart += nBits;
    }
    return nValue;
}

static inline int32_t _uint2int(uint32_t val, uint8_t len)
{
    return val&(1<<(len*8-1))?-((1<<len*8)-val):val;
}

UsbHumanInterfaceDevice::UsbHumanInterfaceDevice(UsbDevice *pDev) :
    Device(pDev), UsbDevice(pDev), m_bDebugState(true), m_bShift(false), m_bCtrl(false), m_bAlt(false), m_bAltGr(false),
    m_bCapsLock(false), m_Combinator(0)//, m_BufStart(0), m_BufEnd(0), m_BufLength(0), m_Callback(0)
{
    //Machine::instance().setKeyboard(this);

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

    //controlRequest(RequestType::Class | RequestRecipient::Interface, Request::SetInterface, 1, m_pInterface->pDescriptor->nInterface);

    uint16_t nHidSize = pHidDescriptor->pDescriptor->nDescriptorLength;
    uint8_t *pHidReportDescriptor = static_cast<uint8_t*>(getDescriptor(0x22, 0, nHidSize, RequestRecipient::Interface));
    int64_t nLogMin = undefined, nLogMax = undefined, nPhysMin = undefined, nPhysMax = undefined, nUsagePage = undefined,
            nUsageMin = undefined, nUsageMax = undefined, nReportSize = undefined, nReportCount = undefined;
    uint8_t nLogSize = 0;
    uint32_t nBits = 0;
    m_pReport = new HidReport();
    Vector<size_t> *pUsages = new Vector<size_t>();
    for(size_t i = 0;i < nHidSize;i++)
    {
        union HidItem {
            struct
            {
                uint8_t size : 2;
                uint8_t type : 2;
                uint8_t tag : 4;
            } PACKED;
            uint8_t raw;
        };
        HidItem item;
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
        DEBUG_LOG("Item tag=" << item.tag << " type=" << Dec << item.type << " size=" << size << Hex);
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
                                for(size_t j = 0;pUsages->count() < nReportCount;j++)
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
    m_pReport->nBytes = nBits/8;

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
}

UsbHumanInterfaceDevice::~UsbHumanInterfaceDevice()
{
}

void UsbHumanInterfaceDevice::callback(uintptr_t pParam, ssize_t ret)
{
    UsbHumanInterfaceDevice *pUKb = reinterpret_cast<UsbHumanInterfaceDevice*>(pParam);
    pUKb->inputHandler();
}

void UsbHumanInterfaceDevice::inputHandler()
{
    size_t nCurrentBit = 0;
    uint32_t nValue;
    int32_t nSignedValue;
    NOTICE("USB: HID: Handling input");
    for(List<HidReportBlock*>::Iterator it = m_pReport->pBlockList.begin();it != m_pReport->pBlockList.end();it++)
    {
        HidReportBlock *pBlock = *it;
        if(pBlock->type != HidReportBlock::Ignore)
        {
            for(size_t i = 0;i<pBlock->nCount;i++)
            {
                nValue = _getField(m_pReportBuffer, nCurrentBit+i*pBlock->nSize, pBlock->nSize);
                //uint32_t onValue = GET_FIELD(m_pOldReportBuffer, nCurrentBit, pBlock->nSize);
                switch(pBlock->type)
                {
                    case HidReportBlock::Absolute:
                        nSignedValue = nValue - _getField(m_pOldReportBuffer, nCurrentBit+i*pBlock->nSize, pBlock->nSize);
                        if(nSignedValue)
                        {
                            if(pBlock->nUsagePage == 9)
                            {
                                if(nSignedValue > 0)
                                    NOTICE("    Button "<<Dec<<((*pBlock->pUsages)[i])<<Hex<<" down");
                                else
                                    NOTICE("    Button "<<Dec<<((*pBlock->pUsages)[i])<<Hex<<" up");
                            }
                            else
                                NOTICE("    Absolute input, usage "<<Dec<<pBlock->nUsagePage<<":"<<((*pBlock->pUsages)[i])<<", value "<<_sign(nSignedValue)<<" "<<pBlock->nSize<<Hex);
                        }
                        break;
                    case HidReportBlock::Relative:
                        nSignedValue = _uint2int(nValue, pBlock->nLogSize);
                        if(nSignedValue)
                            NOTICE("    Relative input, usage "<<Dec<<pBlock->nUsagePage<<":"<<((*pBlock->pUsages)[i])<<", value "<<_sign(nSignedValue)<<Hex);
                        break;
                    case HidReportBlock::Array:
                        /*bool found = false;
                        for(size_t j = 0;j<pBlock->nCount;j++)
                        {
                            if(_getField(m_pOldReportBuffer, nCurrentBit+j*pBlock->nSize, pBlock->nSize) == nValue)
                            {
                                found = true;
                                break;
                            }
                        }*/
                        if(nValue)//found)
                        {
                            NOTICE("    Array input, usage entry "<<Dec<<pBlock->nUsagePage<<":"<<(pBlock->nUsageBase+nValue)<<Hex);
                            //if(pBlock->nUsagePage == 7)
                            //    InputManager::instance().keyPressed(scancodeToCharacter(pBlock->nUsageBase+nValue));
                        }
                    default:
                        break;
                }
            }
        }
        nCurrentBit += pBlock->nCount*pBlock->nSize;
    }
    memcpy(m_pOldReportBuffer, m_pReportBuffer, m_pReport->nBytes);
    /*
    if(m_pReportBuffer[2])
    {
        uint8_t modifiers = m_pReportBuffer[0];
        m_bCtrl = modifiers&(1<<4|1<<0);
        m_bShift = modifiers&(1<<5|1<<1);
        m_bAlt = modifiers&(1<<2);
        m_bAltGr = modifiers&(1<<6);
        InputManager::instance().keyPressed(scancodeToCharacter(m_pReportBuffer[2]));
    }
    */

    /*uint8_t rep[3] = {0,0,0}, scancode = 0, modifiers = 0;
    Timer *timer = Machine::instance().getTimer();
    uint64_t tick = timer->getTickCount(), c = 0;
    bool bFirstRepeat = true;
    while(true)
    {
        if(in(1, &rep, 3)>0)
        {
            // Get the scancode for the pending keystroke.
            scancode = rep[2];
            modifiers = rep[0];
            m_bCtrl = modifiers&(1<<4|1<<0);
            m_bShift = modifiers&(1<<5|1<<1);
            m_bAlt = modifiers&(1<<2);
            m_bAltGr = modifiers&(1<<6);
            c = scancode?scancodeToCharacter(scancode):0;
            bFirstRepeat = true;
        }
        else if(c)
        {
            // Handle repeat
            if(!bFirstRepeat)
            {
                if((timer->getTickCount() - tick) < 40000)
                    continue;
            }
            else
            {
                if((timer->getTickCount() - tick) < 660000)
                    continue;
                bFirstRepeat = false;
            }

        }
        if(c)
        {
            m_Buffer[m_BufEnd++] = c;
            m_BufEnd = m_BufEnd%BUFLEN;
            m_BufLength.release(1);

            InputManager::instance().keyPressed(c);
        }
        tick = timer->getTickCount();
    }*/
}
/*
uint64_t UsbHumanInterfaceDevice::getCharacter()
{
    if(!m_bDebugState)
    {
        m_BufLength.acquire(1);

        uint64_t c = m_Buffer[m_BufStart++];
        m_BufStart = m_BufStart%BUFLEN;

        return c;
    }
    else
    {
        ERROR("Keyboard::getCharacter() should not be called in debug mode");
        return 0;
    }
}

uint64_t UsbHumanInterfaceDevice::getCharacterNonBlock()
{
    if(!m_bDebugState)
    {
        if(m_BufLength.tryAcquire(1))
        {
            uint64_t c = m_Buffer[m_BufStart++];
            m_BufStart = m_BufStart%BUFLEN;

            return c;
        }
        return 0;
    }
    else
    {
        ERROR("Keyboard::getCharacterNonBlock should not be called in debug mode");
        return 0;
    }
}

void UsbHumanInterfaceDevice::setDebugState(bool enableDebugState)
{
    m_bDebugState = enableDebugState;
    if(m_bDebugState)
    //{
        panic("USB: UsbHumanInterfaceDevice: Debugger not supported");
        // Zero the buffer.
        //while(m_BufLength.tryAcquire(1));
        //m_BufStart = 0;
        //m_BufEnd = 0;
    //}
}

bool UsbHumanInterfaceDevice::getDebugState()
{
    return m_bDebugState;
}

uint64_t UsbHumanInterfaceDevice::scancodeToCharacter(uint8_t scancode)
{
    /*if(scancode==0x39)
    {
        m_bCapsLock = !m_bCapsLock;
        return 0;
    }*//*

    bool bUseUpper = false;  // Use the upper case keymap.

    if((m_bCapsLock && !m_bShift) || (!m_bCapsLock && m_bShift) )
        bUseUpper = true;

    // No valid combination for these two keys together.
    if(m_bAlt && m_bAltGr)
        return 0;

    // Try and grab a key code for the scancode with all modifiers enabled.
    table_entry_t *pTableEntry = getTableEntry(m_Combinator, m_bAlt, m_bAltGr, m_bCtrl, m_bShift, scancode);
    if(!pTableEntry || (pTableEntry->val == 0 && pTableEntry->flags == 0))
    {
        // Fall back and just use the shift modifier.
        pTableEntry = getTableEntry(0, false, false, false, m_bShift, scancode);
        if(!pTableEntry || (pTableEntry->val == 0 && pTableEntry->flags == 0))
        {
            // Fall back again and use no modifier.
            pTableEntry = getTableEntry(0, false, false, false, false, scancode);
            if(!pTableEntry)
                return 0;
        }
    }

    // Does this key set any flags?
    uint32_t flags = pTableEntry->flags & 0xFF;
    if(flags)
    {
        // Special case for shift and ctrl here...
        if(flags == SHIFT_M || flags == CTRL_M || flags == ALT_M || flags == ALTGR_M) return 0;

        // If the key sets the same combinator that we're currently using, the "dead" key becomes live and shows the default key.
        if(flags == m_Combinator)
        {
            // Unset combinator and fall through to display default glyph.
            m_Combinator = 0;
        }
        else
        {
            // Change combinator.
            m_Combinator = flags;
            return 0;
        }
    }
    else
    {
        // Dead keys should be reset here.
        m_Combinator = 0;
    }

    if((pTableEntry->flags & UNICODE_POINT) || (pTableEntry->flags & SPECIAL))
    {
        uint64_t ret = pTableEntry->val;

        if(pTableEntry->flags & SPECIAL)
        {
            ret |= Keyboard::Special;
        }
        else if(bUseUpper && ret >= 0x0061 /* a *//* && ret <= 0x007a /* z *//*)
            ret -= (0x0061-0x0041); /* a-A *//*

            if(m_bAlt)   ret |= Keyboard::Alt;
            if(m_bAltGr) ret |= Keyboard::AltGr;
            if(m_bCtrl)  ret |= Keyboard::Ctrl;
            if(m_bShift) ret |= Keyboard::Shift;

            return ret;
    }

    return 0;
}

table_entry_t *UsbHumanInterfaceDevice::getTableEntry(uint8_t combinator, bool bAlt, bool bAltGr, bool bCtrl, bool bShift, uint8_t scancode)
{
    // Grab the keymap table index for this key.
    size_t comb = combinator;
    size_t modifiers =  ((bAlt)?ALT_I:0) | ((bAltGr)?ALTGR_I:0) | ((bCtrl)?CTRL_I:0) | ((bShift)?SHIFT_I:0);
    size_t idx = TABLE_IDX(comb, modifiers, 0, scancode);

    uint8_t *pSparseTable = reinterpret_cast<uint8_t*>(sparse_buff);
    uint8_t *pDataTable   = reinterpret_cast<uint8_t*>(data_buff);

    // Now walk the sparse tree.
    size_t bisect = (TABLE_MAX+1)/2;
    size_t size = (TABLE_MAX+1)/2;
    sparse_t *pSparse = reinterpret_cast<sparse_t*> (&pSparseTable[0]);
    size_t data_idx = 0;
    while(true)
    {
        if(idx < bisect)
        {
            if(pSparse->left == SPARSE_NULL)
                return 0;
            if(pSparse->left & SPARSE_DATA_FLAG)
            {
                size_t off = idx - (bisect-size);
                data_idx = (pSparse->left & ~SPARSE_DATA_FLAG) + off*sizeof(table_entry_t);
                break;
            }
            size /= 2;
            bisect = bisect - size;
            pSparse = reinterpret_cast<sparse_t*>(&pSparseTable[pSparse->left]);
        }
        else
        {
            if(pSparse->right == SPARSE_NULL)
                return 0;
            if(pSparse->right & SPARSE_DATA_FLAG)
            {
                size_t off = idx - bisect;
                data_idx = (pSparse->right & ~SPARSE_DATA_FLAG) + off*sizeof(table_entry_t);
                break;
            }
            size /= 2;
            bisect = bisect + size;
            pSparse = reinterpret_cast<sparse_t*>(&pSparseTable[pSparse->right]);
        }
    }

    table_entry_t *pTabEntry = reinterpret_cast<table_entry_t*>(&pDataTable[data_idx]);

    return pTabEntry;
}*/
