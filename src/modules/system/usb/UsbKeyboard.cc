/*
* Copyright (c) 2008 James Molloy, Jörg Pfähler, Matthew Iselin
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

#include <machine/Machine.h>
#include <machine/Timer.h>
#include <process/Thread.h>
#include <processor/IoPort.h>
#include <processor/state.h>
#include <utilities/utility.h>
#include <usb/UsbConstants.h>
#include <usb/UsbDevice.h>
#include <usb/UsbKeyboard.h>
#include <usb/UsbKeyboardMappingEnUs.h>
#include <usb/UsbManager.h>

UsbKeyboard::UsbKeyboard(UsbDevice *dev) :
UsbDevice(dev), m_bDebugState(true), m_bShift(false), m_bCtrl(false), m_bAlt(false), m_bAltGr(false),
m_bCapsLock(false), m_BufStart(0), m_BufEnd(0), m_BufLength(0), m_Callback(0)
{
    Machine::instance().setKeyboard(this);
}

UsbKeyboard::~UsbKeyboard()
{
}

int UsbKeyboard::trampoline(void *p)
{
    UsbKeyboard *pUKb = reinterpret_cast<UsbKeyboard*> (p);
    pUKb->thread();
    return 0;
}

void UsbKeyboard::thread()
{
    uint8_t rep[3]={0,0,0}, scancode=0, modifiers=0;
    Timer *timer = Machine::instance().getTimer();
    uint64_t tick = timer->getTickCount(), c=0;
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
            c = scancodeToCharacter(scancode);
        }
        else if(timer->getTickCount()-tick<125)
            continue;
        if (c != 0)
        {
            m_Buffer[m_BufEnd++] = c;
            m_BufEnd = m_BufEnd%BUFLEN;
            m_BufLength.release(1);
            if (m_Callback)
                m_Callback(c);
        }
        tick = timer->getTickCount();
    }
}

uint64_t UsbKeyboard::getCharacter()
{
    if (!m_bDebugState)
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

uint64_t UsbKeyboard::getCharacterNonBlock()
{
    if (!m_bDebugState)
    {
        if (m_BufLength.tryAcquire(1))
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

void UsbKeyboard::setDebugState(bool enableDebugState)
{
    m_bDebugState = enableDebugState;
    if (m_bDebugState)
    {
        FATAL("USB: UsbKeyboard: Debugger not supported");
        // Zero the buffer.
        while(m_BufLength.tryAcquire(1));
        m_BufStart = 0;
        m_BufEnd = 0;
    }
}

bool UsbKeyboard::getDebugState()
{
    return m_bDebugState;
}

uint64_t UsbKeyboard::scancodeToCharacter(uint8_t scancode)
{
    /*if(scancode==0x39)
    {
        m_bCapsLock = !m_bCapsLock;
        return 0;
    }*/

    bool bUseUpper = false;  // Use the upper case keymap.

    if ( (m_bCapsLock && !m_bShift) || (!m_bCapsLock && m_bShift) )
        bUseUpper = true;

    // No valid combination for these two keys together.
    if (m_bAlt && m_bAltGr)
        return 0;

    // Try and grab a key code for the scancode with all modifiers enabled.
    table_entry_t *pTableEntry = getTableEntry(m_bAlt, m_bAltGr, m_bCtrl, m_bShift, scancode);
    if (!pTableEntry || (pTableEntry->val == 0 && pTableEntry->flags == 0))
    {
        NOTICE("Needed to fallback, altgr: " << m_bAltGr <<", alt " << m_bAlt);
        // Fall back and just use the shift modifier.
        pTableEntry = getTableEntry(false, false, false, m_bShift, scancode);
        if (!pTableEntry || (pTableEntry->val == 0 && pTableEntry->flags == 0))
        {
            // Fall back again and use no modifier.
            pTableEntry = getTableEntry(false, false, false, false, scancode);
            if (!pTableEntry)
            {
                NOTICE("Failed completely.");
                return 0;
            }
        }
    }

    if ((pTableEntry->flags & UNICODE_POINT) || (pTableEntry->flags & SPECIAL))
    {
        uint64_t ret = pTableEntry->val;

        if (pTableEntry->flags & SPECIAL)
        {
            ret |= Keyboard::Special;
        }
        else if (bUseUpper && ret >= 0x0061 /* a */ && ret <= 0x007a /* z */)
            ret -= (0x0061-0x0041); /* a-A */

            if (m_bAlt)   ret |= Keyboard::Alt;
            if (m_bAltGr) ret |= Keyboard::AltGr;
            if (m_bCtrl)  ret |= Keyboard::Ctrl;
            if (m_bShift) ret |= Keyboard::Shift;

            return ret;
    }

    return 0;
}

table_entry_t *UsbKeyboard::getTableEntry(bool bAlt, bool bAltGr, bool bCtrl, bool bShift, uint8_t scancode)
{
    // Grab the keymap table index for this key.
    size_t alt = ((bAlt)?ALT_I:0) | ((bAltGr)?ALTGR_I:0);
    size_t modifiers =  ((bCtrl)?CTRL_I:0) | ((bShift)?SHIFT_I:0);
    size_t idx = TABLE_IDX(alt, modifiers, 0, scancode);

    uint8_t *pSparseTable = reinterpret_cast<uint8_t*>(usb_sparse_buff);
    uint8_t *pDataTable   = reinterpret_cast<uint8_t*>(usb_data_buff);

    // Now walk the sparse tree.
    size_t bisect = (TABLE_MAX+1)/2;
    size_t size = (TABLE_MAX+1)/2;
    sparse_t *pSparse = reinterpret_cast<sparse_t*> (&pSparseTable[0]);
    size_t data_idx = 0;
    while (true)
    {
        if (idx < bisect)
        {
            if (pSparse->left == SPARSE_NULL)
                return 0;
            if (pSparse->left & SPARSE_DATA_FLAG)
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
            if (pSparse->right == SPARSE_NULL)
                return 0;
            if (pSparse->right & SPARSE_DATA_FLAG)
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
}
