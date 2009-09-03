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

#include "Keyboard.h"
#include <machine/Machine.h>
#include <utilities/utility.h>

#ifdef DEBUGGER
  #include <Debugger.h>

  #ifdef TRACK_PAGE_ALLOCATIONS
    #include <AllocationCommand.h>
  #endif
#endif

#include <machine/keymaps/pc102EnUs.h>

#define CAPSLOCK  0x3a
#define LSHIFT    0x2a
#define RSHIFT    0x36
#define ALT       0x38
#define CTRL      0x1d

uint8_t *g_pSparseTable = reinterpret_cast<uint8_t*>(sparse_buff);
uint8_t *g_pDataTable = reinterpret_cast<uint8_t*>(data_buff);

X86Keyboard::X86Keyboard(uint32_t portBase) :
    m_bDebugState(true),
    m_bShift(false),
    m_bCtrl(false), m_bAlt(false), m_bAltGr(false),
    m_Combinator(0), m_bEscape(false),
    m_bCapsLock(false),
    m_Port("PS/2 Keyboard controller"),
    m_BufStart(0), m_BufEnd(0), m_BufLength(0),
    m_IrqId(0), m_Callback(0)
{
}

X86Keyboard::~X86Keyboard()
{
}

void X86Keyboard::initialise()
{
    m_Port.allocate(0x60/*portBase*/, 5);

    // Register the irq
    IrqManager &irqManager = *Machine::instance().getIrqManager();
    m_IrqId = irqManager.registerIsaIrqHandler(1, this, true);
    if (m_IrqId == 0)
    {
        ERROR("X86Keyboard: failed to register IRQ handler!");
    }

    irqManager.enable(1, false);
}

char X86Keyboard::getChar()
{
    // If we're in debug state we can only poll.
    if (m_bDebugState)
    {
        uint8_t scancode, status;
        do
        {
            // Get the keyboard's status byte.
            status = m_Port.read8(4);
        }
        while ( !(status & 0x01) ); // Spin until there's a key ready.

        // Get the scancode for the pending keystroke.
        scancode = m_Port.read8(0);

        uint64_t c = scancodeToCharacter(scancode);
        if ((c&Keyboard::Special) || ((c&0xFFFFFFFF) > 0x7f)) return 0;
        else return static_cast<char>(c&0x7f);
    }
    else
    {
        ERROR("Keyboard::getChar() should not be called outside debug mode");
        return 0;
    }
}

uint64_t X86Keyboard::getCharacter()
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

char X86Keyboard::getCharNonBlock()
{
    if (m_bDebugState)
    {
        uint8_t scancode, status;
        // Get the keyboard's status byte.
        status = m_Port.read8(4);
        if (!(status & 0x01))
            return 0;

        // Get the scancode for the pending keystroke.
        scancode = m_Port.read8(0);

        uint64_t c = scancodeToCharacter(scancode);
        if ((c&Keyboard::Special) || ((c&0xFFFFFFFF) > 0x7f)) return 0;
        else return static_cast<char>(c&0x7f);
    }
    else
    {
        ERROR("Keyboard::getCharNonBlock should not be called outside debug mode");
        return 0;
    }
}

uint64_t X86Keyboard::getCharacterNonBlock()
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

void X86Keyboard::setDebugState(bool enableDebugState)
{
    m_bDebugState = enableDebugState;
    IrqManager &irqManager = *Machine::instance().getIrqManager();
    if (m_bDebugState)
    {
        irqManager.enable(1, false);

        // Zero the buffer.
        while (m_BufLength.tryAcquire(1)) ;
        m_BufStart = 0;
        m_BufEnd = 0;
    }
    else
    {
        irqManager.enable(1, true);
    }
}

bool X86Keyboard::getDebugState()
{
    return m_bDebugState;
}

bool X86Keyboard::irq(irq_id_t number, InterruptState &state)
{
    uint8_t scancode, status;

    // Get the keyboard's status byte.
    status = m_Port.read8(4);
    if (!(status & 0x01))
        return true;

    // Get the scancode for the pending keystroke.
    scancode = m_Port.read8(0);

    uint64_t c = scancodeToCharacter(scancode);
#ifdef DEBUGGER
#ifdef TRACK_PAGE_ALLOCATIONS
    if (scancode == 0x57) // F11
    {
        g_AllocationCommand.checkpoint();
    }
#endif
    if (scancode == 0x58) // F12
    {
        LargeStaticString sError;
        sError += "User-induced breakpoint";
        Debugger::instance().start(state, sError);
    }
#endif
    if (c != 0)
    {
        m_Buffer[m_BufEnd++] = c;
        m_BufEnd = m_BufEnd%BUFLEN;
        m_BufLength.release(1);
        if (m_Callback)
            m_Callback(c);
    }

    return true;
}

uint64_t X86Keyboard::scancodeToCharacter(uint8_t scancode)
{
    // Special 'escape' scancode?
    if (scancode == 0xe0)
    {
        m_bEscape = true;
        return 0;
    }

    // Was this a keypress?
    bool bKeypress = true;
    if (scancode & 0x80)
    {
        bKeypress = false;
        scancode &= 0x7f;
    }

    bool bUseUpper = false;  // Use the upper case keymap.
    // Certain scancodes have special meanings.
    switch (scancode)
    {
        case CAPSLOCK: // TODO: fix capslock. Both a make and break scancode are sent on keydown AND keyup!
            if (bKeypress)
                m_bCapsLock = !m_bCapsLock;
            return 0;
        case LSHIFT:
        case RSHIFT:
            if (bKeypress)
                m_bShift = true;
            else
                m_bShift = false;
            m_bEscape = false;
            return 0;
        case CTRL:
            if (bKeypress)
                m_bCtrl = true;
            else
                m_bCtrl = false;
            m_bEscape = false;
            return 0;
        case ALT:
            if (!m_bEscape)
            {
                if (bKeypress)
                    m_bAlt = true;
                else
                    m_bAlt = false;
                return 0;
            }
            else
            {
                if (bKeypress)
                    m_bAltGr = true;
                else
                    m_bAltGr = false;
                m_bEscape = false;
                return 0;
            }
    }

    if ( (m_bCapsLock && !m_bShift) || (!m_bCapsLock && m_bShift) )
        bUseUpper = true;

    if (!bKeypress)
        return 0;

    // Try and grab a key code for the scancode with all modifiers enabled.
    table_entry_t *pTableEntry = getTableEntry(m_Combinator, m_bAlt, m_bAltGr, m_bCtrl, m_bShift, m_bEscape, scancode);
    if (!pTableEntry || (pTableEntry->val == 0 && pTableEntry->flags == 0))
    {
        NOTICE("Needed to fallback");
        // Fall back and just use the shift modifier.
        pTableEntry = getTableEntry(0, false, false, false, m_bShift, m_bEscape, scancode);
        if (!pTableEntry || (pTableEntry->val == 0 && pTableEntry->flags == 0))
        {
            // Fall back again and use no modifier.
            pTableEntry = getTableEntry(0, false, false, false, false, false, scancode);
            if (!pTableEntry)
            {
                NOTICE("Failed completely.");
                m_bEscape = false;
                return 0;
            }
        }
    }
    m_bEscape = false;

    // Does this key set any flags?
    uint32_t flags = pTableEntry->flags & 0xFF;
    if (flags)
    {
        // Special case for shift and ctrl here...
        if (flags == SHIFT_M || flags == CTRL_M || flags == ALT_M || flags == ALTGR_M) return 0;

        // If the key sets the same combinator that we're currently using, the "dead" key becomes live and shows the default key.
        if (flags == m_Combinator)
        {
            // Unset combinator and fall through to display default glyph.
            m_Combinator = 0;
        }
        else
        {
            // Change combinator.
            m_Combinator = flags;
            NOTICE("Set combinator to " << flags);
            return 0;
        }
    }
    else
    {
        // Dead keys should be reset here.
        m_Combinator = 0;
    }

    if ((pTableEntry->flags & UNICODE_POINT) || (pTableEntry->flags & SPECIAL))
    {
        uint64_t ret = pTableEntry->val;
        NOTICE("U+" << Hex << pTableEntry->val);
        if (pTableEntry->flags & SPECIAL)
        {
            ret |= Keyboard::Special;
        }
        else if (bUseUpper && ret >= 0x0061 /* a */ && ret <= 0x007a /* z */)
            ret -= (0x0061-0x0041); /* a-A */

        if (m_Combinator == ALT_I)   ret |= Keyboard::Alt;
        if (m_Combinator == ALTGR_I) ret |= Keyboard::AltGr;
        if (m_bCtrl)  ret |= Keyboard::Ctrl;
        if (m_bShift) ret |= Keyboard::Shift;

        return ret;
    }

    return 0;
}

table_entry_t *X86Keyboard::getTableEntry(uint8_t combinator, bool bAlt, bool bAltGr, bool bCtrl, bool bShift, bool bEscape, uint8_t scancode)
{
    // Grab the keymap table index for this key.
    size_t comb = combinator;
    size_t modifiers =  ((bAlt)?ALT_I:0) | ((bAltGr)?ALTGR_I:0) | ((bCtrl)?CTRL_I:0) | ((bShift)?SHIFT_I:0);
    size_t escape = (bEscape)?1:0;
    size_t idx = TABLE_IDX(comb, modifiers, escape, scancode);

    // ??? Why???
    // No idea. JamesM
    m_bEscape = false;

    uint8_t *pSparseTable = g_pSparseTable;
    uint8_t *pDataTable   = g_pDataTable;

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
