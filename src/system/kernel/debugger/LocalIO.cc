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

#include <LocalIO.h>
#include <DebuggerCommand.h>
#include <utilities/utility.h>
#include <processor/IoPort.h>

#ifdef DEBUGGER_QWERTY
#include <keymap_qwerty.h>
#endif
#ifdef DEBUGGER_QWERTZ
#include <keymap_qwertz.h>
#endif


LocalIO::LocalIO() :
  m_UpperCliLimit(0),
  m_LowerCliLimit(0),
  m_CursorX(0),
  m_CursorY(0),
  m_bShift(false),
  m_bCtrl(false),
  m_bCapslock(false)
{
  // Copy the current screen contents to our old frame buffer.
  uint16_t *vidmem = reinterpret_cast<uint16_t*>(0xB8000);
  memcpy(m_pOldFramebuffer, vidmem, CONSOLE_WIDTH*CONSOLE_HEIGHT*2);

  // Clear the framebuffer.
  for (size_t i = 0; i < CONSOLE_WIDTH*CONSOLE_HEIGHT; i++)
  {
    uint8_t attributeByte = (DebuggerIO::Black << 4) | (DebuggerIO::White & 0x0F);
    uint16_t blank = ' ' | (attributeByte << 8);
    m_pFramebuffer[i] = blank;
  }
  m_pCommand[0] = '\0';

  // Initialise the keyboard map.
  initKeymap();
}

LocalIO::~LocalIO()
{
  // Copy our old frame buffer to the screen.
  uint16_t *vidmem = reinterpret_cast<uint16_t*>(0xB8000);
  memcpy(vidmem, m_pOldFramebuffer, CONSOLE_WIDTH*CONSOLE_HEIGHT*2);
}

void LocalIO::setCliUpperLimit(size_t nlines)
{
  // Do a quick sanity check.
  if (nlines < CONSOLE_HEIGHT)
    m_UpperCliLimit = nlines;

  // If the cursor is now in an invalid area, move it back.
  if (m_CursorY < m_UpperCliLimit)
    m_CursorY = m_UpperCliLimit;
}

void LocalIO::setCliLowerLimit(size_t nlines)
{
  // Do a quick sanity check.
  if (nlines < CONSOLE_HEIGHT)
    m_LowerCliLimit = nlines;
  
  // If the cursor is now in an invalid area, move it back.
  if (m_CursorY >= CONSOLE_HEIGHT-m_LowerCliLimit)
    m_CursorY = m_LowerCliLimit;
}

void LocalIO::enableCli()
{
  // Clear the framebuffer.
  for (size_t i = 0; i < CONSOLE_WIDTH*CONSOLE_HEIGHT; i++)
  {
    uint8_t attributeByte = (DebuggerIO::Black << 4) | (DebuggerIO::White & 0x0F);
    uint16_t blank = ' ' | (attributeByte << 8);
    m_pFramebuffer[i] = blank;
  }
  m_pCommand[0] = '\0';

  // Reposition the cursor.
  m_CursorX = 0;
  m_CursorY = m_UpperCliLimit;
  
  // We've enabled the CLI, so let's make sure the screen is ready.
  forceRefresh();
}

void LocalIO::disableCli()
{
  // Clear the framebuffer.
  for (int i = 0; i < CONSOLE_WIDTH*CONSOLE_HEIGHT; i++)
  {
    uint8_t attributeByte = (DebuggerIO::Black << 4) | (DebuggerIO::White & 0x0F);
    uint16_t blank = ' ' | (attributeByte << 8);
    m_pFramebuffer[i] = blank;
  }
  
  // Reposition the cursor.
  m_CursorX = 0;
  m_CursorY = m_UpperCliLimit;
  
  forceRefresh();
}

char LocalIO::getCharNonBlock()
{
#ifdef X86
  IoPort port;
  port.allocate(0x60, 4, "PS/2 keyboard controller");
  unsigned char status = port.read8(4);
  if (status & 0x01)
      return port.read8(0);
  else
    return '\0';
#endif
#ifndef X86
  return '\0';
#endif
}

char LocalIO::getChar()
{
#ifdef X86
  // Let's get a character from the keyboard.
  IoPort port;
  port.allocate(0x60, 4, "PS/2 keyboard controller");
  uint8_t scancode, status;
  do
  {
    // Get the keyboard's status byte.
    status = port.read8(4);
  }
  while ( !(status & 0x01) ); // Spin until there's a key ready.

  // Get the scancode for the pending keystroke.
  scancode = port.read8(0);
  
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
      m_bCapslock = !m_bCapslock;
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
  }


  if ( (m_bCapslock && !m_bShift) || (!m_bCapslock && m_bShift) )
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
#endif
  return 0;
}

void LocalIO::drawHorizontalLine(char c, size_t row, size_t colStart, size_t colEnd, DebuggerIO::Colour foreColour, DebuggerIO::Colour backColour)
{
  // colEnd must be bigger than colStart.
  if (colStart > colEnd)
  {
    size_t tmp = colStart;
    colStart = colEnd;
    colEnd = tmp;
  }

  if (colEnd >= CONSOLE_WIDTH)
    colEnd = CONSOLE_WIDTH-1;
  if (colStart < 0)
    colStart = 0;
  if (row >= CONSOLE_HEIGHT)
    row = CONSOLE_HEIGHT-1;
  if (row < 0)
    row = 0;

  uint8_t attributeByte = (backColour << 4) | (foreColour & 0x0F);
  for(size_t i = colStart; i <= colEnd; i++)
  {
    m_pFramebuffer[row*CONSOLE_WIDTH+i] = c | (attributeByte << 8);
  }
  
  if (m_bRefreshesEnabled)
    forceRefresh();
}

void LocalIO::drawVerticalLine(char c, size_t col, size_t rowStart, size_t rowEnd, DebuggerIO::Colour foreColour, DebuggerIO::Colour backColour)
{
  // rowEnd must be bigger than rowStart.
  if (rowStart > rowEnd)
  {
    size_t tmp = rowStart;
    rowStart = rowEnd;
    rowEnd = tmp;
  }

  if (rowEnd >= CONSOLE_HEIGHT)
    rowEnd = CONSOLE_HEIGHT-1;
  if (rowStart < 0)
    rowStart = 0;
  if (col >= CONSOLE_WIDTH)
    col = CONSOLE_WIDTH-1;
  if (col < 0)
    col = 0;
  
  uint8_t attributeByte = (backColour << 4) | (foreColour & 0x0F);
  for(size_t i = rowStart; i <= rowEnd; i++)
  {
    m_pFramebuffer[i*CONSOLE_WIDTH+col] = c | (attributeByte << 8);
  }
  
  if (m_bRefreshesEnabled)
    forceRefresh();
}

void LocalIO::drawString(const char *str, size_t row, size_t col, DebuggerIO::Colour foreColour, DebuggerIO::Colour backColour)
{
  // Now then, this is a lesson in cheating. Listen up.
  // Firstly we save the current cursorX and cursorY positions.
  size_t savedX = m_CursorX;
  size_t savedY = m_CursorY;
  
  // Then, we change cursorX and Y to be row, col.
  m_CursorX = col;
  m_CursorY = row;
  
  bool bRefreshWasEnabled = false;
  if (m_bRefreshesEnabled)
  {
    bRefreshWasEnabled = true;
    m_bRefreshesEnabled = false;
  }
  
  // Then, we just call putChar to put the string out for us! :)
  while (*str)
    putChar(*str++, foreColour, backColour);
  
  if (bRefreshWasEnabled)
  {
    m_bRefreshesEnabled = true;
    forceRefresh();
  }
  
  // And restore the cursor.
  m_CursorX = savedX;
  m_CursorY = savedY;
  
  // Ensure the cursor is correct in hardware.
  moveCursor();
}

void LocalIO::enableRefreshes()
{
  m_bRefreshesEnabled = true;
  forceRefresh();
}

void LocalIO::disableRefreshes()
{
  m_bRefreshesEnabled = false;
}

void LocalIO::forceRefresh()
{
  uint16_t *vidmem = reinterpret_cast<uint16_t*>(0xB8000);

  memcpy(vidmem, m_pFramebuffer, CONSOLE_WIDTH*CONSOLE_HEIGHT*2);
  moveCursor();
}

void LocalIO::scroll()
{
  // Get a space character with the default colour attributes.
  uint8_t attributeByte = (DebuggerIO::Black << 4) | (DebuggerIO::White & 0x0F);
  uint16_t blank = ' ' | (attributeByte << 8);
  if (m_CursorY >= CONSOLE_HEIGHT-m_LowerCliLimit)
  {
    for (size_t i = m_UpperCliLimit*80; i < (CONSOLE_HEIGHT-m_LowerCliLimit-1)*80; i++)
      m_pFramebuffer[i] = m_pFramebuffer[i+80];

    for (size_t i = (CONSOLE_HEIGHT-m_LowerCliLimit-1)*80; i < (CONSOLE_HEIGHT-m_LowerCliLimit)*80; i++)
      m_pFramebuffer[i] = blank;

    m_CursorY = CONSOLE_HEIGHT-m_LowerCliLimit-1;
  }

}

void LocalIO::moveCursor()
{
#ifdef X86
  uint16_t tmp = m_CursorY*80 + m_CursorX;
  
  IoPort cursorPort;
  cursorPort.allocate(0x3D4, 2, "VGA controller");
  
  cursorPort.write8(14, 0);
  cursorPort.write8(tmp>>8, 1);
  cursorPort.write8(15, 0);
  cursorPort.write8(tmp, 1);
#endif
}

void LocalIO::putChar(char c, DebuggerIO::Colour foreColour, DebuggerIO::Colour backColour)
{
  // Backspace?
  if (c == 0x08)
  {
    // Can we just move backwards? or do we have to go up?
    if (m_CursorX)
      m_CursorX--;
    else
    {
      m_CursorX = CONSOLE_WIDTH-1;
      m_CursorY--;
    }
    
    // Erase the contents of the cell currently.
    uint8_t attributeByte = (backColour << 4) | (foreColour & 0x0F);
    m_pFramebuffer[m_CursorY*CONSOLE_WIDTH + m_CursorX] = ' ' | (attributeByte << 8);
    
  }

  // Tab?
  else if (c == 0x09 && ( ((m_CursorX+8)&~(8-1)) < CONSOLE_WIDTH) )
    m_CursorX = (m_CursorX+8) & ~(8-1);

  // Carriage return?
  else if (c == '\r')
    m_CursorX = 0;

  // Newline?
  else if (c == '\n')
  {
    m_CursorX = 0;
    m_CursorY++;
  }

  // Normal character?
  else if (c >= ' ')
  {
    uint8_t attributeByte = (backColour << 4) | (foreColour & 0x0F);
    m_pFramebuffer[m_CursorY*CONSOLE_WIDTH + m_CursorX] = c | (attributeByte << 8);

    // Increment the cursor.
    m_CursorX++;
  }

  // Do we need to wrap?
  if (m_CursorX >= 80)
  {
    m_CursorX = 0;
    m_CursorY ++;
  }
}

void LocalIO::cls()
{
  // Clear the framebuffer.
  for (int i = 0; i < CONSOLE_WIDTH*CONSOLE_HEIGHT; i++)
  {
    unsigned char attributeByte = (DebuggerIO::Black << 4) | (DebuggerIO::White & 0x0F);
    unsigned short blank = ' ' | (attributeByte << 8);
    m_pFramebuffer[i] = blank;
  }
}


