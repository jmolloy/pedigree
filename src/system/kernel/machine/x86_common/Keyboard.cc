/*
 * Copyright (c) 2008 James Molloy, James Pritchett, Jörg Pfähler, Matthew Iselin
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
#include <utilities/utility.h>

#ifdef DEBUGGER_QWERTY
#include <keymap_qwerty.h>
#endif
#ifdef DEBUGGER_QWERTZ
#include <keymap_qwertz.h>
#endif

X86Keyboard::X86Keyboard(uint32_t portBase) :
  m_bShift(false),
  m_bCtrl(false),
  m_bAlt(false),
  m_bCapsLock(false),
  m_Port("PS/2 Keyboard controller")
{ 
}

X86Keyboard::~X86Keyboard()
{
}

void X86Keyboard::initialise()
{
  m_Port.allocate(0x60/*portBase*/, 5);
  // Initialise the keyboard map.
  initKeymap();
}

char X86Keyboard::getChar()
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
}
  
char X86Keyboard::getCharNonBlock()
{
  uint8_t scancode, status;
  // Get the keyboard's status byte.
  status = m_Port.read8(4);
  if (!(status & 0x01))
    return 0;

  // Get the scancode for the pending keystroke.
  scancode = m_Port.read8(0);
  
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
