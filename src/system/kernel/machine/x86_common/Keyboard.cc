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
#endif

#ifdef DEBUGGER_QWERTY
#include <keymap_qwerty.h>
#endif
#ifdef DEBUGGER_QWERTZ
#include <keymap_qwertz.h>
#endif

X86Keyboard::X86Keyboard(uint32_t portBase) :
  m_bDebugState(true),
  m_bShift(false),
  m_bCtrl(false),
  m_bAlt(false),
  m_bCapsLock(false),
  m_Port("PS/2 Keyboard controller"),
  m_BufStart(0), m_BufEnd(0), m_BufLength(0),
  m_IrqId(0)
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
  m_IrqId = irqManager.registerIsaIrqHandler(1, this);
  if (m_IrqId == 0)
  {
    ERROR("X86Keyboard: failed to register IRQ handler!");
  }

  irqManager.enable(1, false);

  // Initialise the keyboard map.
  initKeymap();
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

    return scancodeToChar(scancode);
  }
  else
  {
    FATAL("KEYBOARD: getChar() should not be called in normal mode!");
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

    return scancodeToChar(scancode);
  }
  else
  {
    FATAL("KEYBOARD: getCharNonBlock() should not be called in normal mode!");
    return 0;
  }
}

Keyboard::Character X86Keyboard::getCharacter()
{
  m_BufLength.acquire(1);

  Character c = m_Buffer[m_BufStart++];
  m_BufStart = m_BufStart%BUFLEN;

  return c;
}

Keyboard::Character X86Keyboard::getCharacterNonBlock()
{
  if (m_BufLength.tryAcquire(1))
  {
    Character c = m_Buffer[m_BufStart++];
    m_BufStart = m_BufStart%BUFLEN;

    return c;
  }
  Character c;
  c.valid = 0;
  return c;
}

bool X86Keyboard::shift()
{
  return m_bShift;
}

bool X86Keyboard::ctrl()
{
  return m_bCtrl;
}

bool X86Keyboard::alt()
{
  return m_bAlt;
}

bool X86Keyboard::capsLock()
{
  return m_bCapsLock;
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

  Character c = scancodeToCharacter(scancode);
#ifdef DEBUGGER
  if (c.valid && c.is_special && c.value == KB_F12)
  {
    LargeStaticString sError;
    sError += "User-induced breakpoint";
    Debugger::instance().start(state, sError);
  }
#endif
  if (c.valid)
  {
    m_Buffer[m_BufEnd++] = c;
    m_BufEnd = m_BufEnd%BUFLEN;
    m_BufLength.release(1);
  }

  return true;
}

char X86Keyboard::scancodeToChar(uint8_t scancode)
{
  // We don't care about 'special' scancodes which start with 0xe0.
  if (scancode == 0xe0)
    return 0;

  // Was this a keypress?
  bool bKeypress = true;
  if (scancode & 0x80)
  {
    bKeypress = false;
    scancode &= 0x7f;
  }

  bool bUseUpper = false;  // Use the upper case keymap.
  bool bUseNums = false;   // Use the upper case keymap for numbers.
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
    return 0;
  case CTRL:
    if (bKeypress)
      m_bCtrl = true;
    else
      m_bCtrl = false;
    return 0;
  case ALT:
    if (bKeypress)
      m_bAlt = true;
    else
      m_bAlt = false;
    return 0;
  }

  if ( (m_bCapsLock && !m_bShift) || (!m_bCapsLock && m_bShift) )
    bUseUpper = true;

  if (m_bShift)
    bUseNums = true;

  if (!bKeypress)
    return 0;

  if (scancode < 0x02)
    return keymap_lower[scancode];
  else if ( (scancode <  0x0e /* backspace */) ||
            (scancode >= 0x1a /*[*/ && scancode <= 0x1b /*]*/) ||
            (scancode >= 0x27 /*;*/ && scancode <= 0x29 /*`*/) ||
            (scancode == 0x2b) ||
            (scancode >= 0x33 /*,*/ && scancode <= 0x35 /*/*/) )
  {
    if (bUseNums)
      return keymap_upper[scancode];
    else
      return keymap_lower[scancode];
  }
  else if ( (scancode >= 0x10 /*Q*/ && scancode <= 0x19 /*P*/) ||
            (scancode >= 0x1e /*A*/ && scancode <= 0x26 /*L*/) ||
            (scancode >= 0x2c /*Z*/ && scancode <= 0x32 /*M*/) )
  {
    if (bUseUpper)
      return keymap_upper[scancode];
    else
      return keymap_lower[scancode];
  }
  else if (scancode <= 0x39 /* space */)
    return keymap_lower[scancode];
  else
    return 0;
}

Keyboard::Character X86Keyboard::scancodeToCharacter(uint8_t scancode)
{
  // We don't care about 'special' scancodes which start with 0xe0.
  if (scancode == 0xe0)
  {
    Character c;
    c.valid = 0;
    return c;
  }

  // Was this a keypress?
  bool bKeypress = true;
  if (scancode & 0x80)
  {
    bKeypress = false;
    scancode &= 0x7f;
  }

  Character c;
  c.valid = 0;
  c.ctrl = m_bCtrl;
  c.alt = m_bAlt;
  c.shift = m_bShift;
  c.num_lock = 0;
  c.is_special = 0;
  c.reserved = 0;
  c.value = 0;

  bool bUseUpper = false;  // Use the upper case keymap.
  bool bUseNums = false;   // Use the upper case keymap for numbers.
  // Certain scancodes have special meanings.
  switch (scancode)
  {
  case CAPSLOCK: // TODO: fix capslock. Both a make and break scancode are sent on keydown AND keyup!
    if (bKeypress)
      m_bCapsLock = !m_bCapsLock;
    return c;
  case LSHIFT:
  case RSHIFT:
    if (bKeypress)
      m_bShift = true;
    else
      m_bShift = false;
    return c;
  case CTRL:
    if (bKeypress)
      m_bCtrl = true;
    else
      m_bCtrl = false;
    return c;
  case ALT:
    if (bKeypress)
      m_bAlt = true;
    else
      m_bAlt = false;
    return c;
  }

  if ( (m_bCapsLock && !m_bShift) || (!m_bCapsLock && m_bShift) )
    bUseUpper = true;

  if (m_bShift)
    bUseNums = true;

  if (!bKeypress)
    return c;

  // Now c is valid...
  c.valid = 1;

  if (scancode < 0x02)
    c.value = keymap_lower[scancode];
  else if ( (scancode <  0x0e /* backspace */) ||
            (scancode >= 0x1a /*[*/ && scancode <= 0x1b /*]*/) ||
            (scancode >= 0x27 /*;*/ && scancode <= 0x29 /*`*/) ||
            (scancode == 0x2b) ||
            (scancode >= 0x33 /*,*/ && scancode <= 0x35 /*/*/) )
  {
    if (bUseNums)
      c.value = keymap_upper[scancode];
    else
      c.value = keymap_lower[scancode];
  }
  else if ( (scancode >= 0x10 /*Q*/ && scancode <= 0x19 /*P*/) ||
            (scancode >= 0x1e /*A*/ && scancode <= 0x26 /*L*/) ||
            (scancode >= 0x2c /*Z*/ && scancode <= 0x32 /*M*/) )
  {
    if (bUseUpper)
      c.value = keymap_upper[scancode];
    else
      c.value = keymap_lower[scancode];
  }
  else if (scancode <= 0x39 /* space */)
  {
    c.value = keymap_lower[scancode];
  }
  else
  {
    c.is_special = 1;
    c.value = keymap_special[scancode];
    if (c.value == 0) c.valid = 0;
  }
  return c;
}
