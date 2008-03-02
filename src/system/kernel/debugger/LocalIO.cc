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
#include <utility.h>
#include <processor/io.h>

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
  m_bRefreshesEnabled(true),
  m_bReady(true),
  m_bShift(false),
  m_bCtrl(false),
  m_bCapslock(false)
{
  // Copy the current screen contents to our old frame buffer.
  unsigned short *vidmem = reinterpret_cast<unsigned short*>(0xB8000);
  memcpy(m_pOldFramebuffer, vidmem, CONSOLE_WIDTH*CONSOLE_HEIGHT*2);

  // Clear the framebuffer.
  for (int i = 0; i < CONSOLE_WIDTH*CONSOLE_HEIGHT; i++)
  {
    unsigned char attributeByte = (DebuggerIO::Black << 4) | (DebuggerIO::White & 0x0F);
    unsigned short blank = ' ' | (attributeByte << 8);
    m_pFramebuffer[i] = blank;
  }
  m_pCommand[0] = '\0';

  // Initialise the keyboard map.
  initKeymap();
}

LocalIO::~LocalIO()
{
  // Copy our old frame buffer to the screen.
  unsigned short *vidmem = reinterpret_cast<unsigned short*>(0xB8000);
  memcpy(vidmem, m_pOldFramebuffer, CONSOLE_WIDTH*CONSOLE_HEIGHT*2);
}

void LocalIO::setCliUpperLimit(int nlines)
{
  // Do a quick sanity check.
  if (nlines < CONSOLE_HEIGHT)
    m_UpperCliLimit = nlines;

  // If the cursor is now in an invalid area, move it back.
  if (m_CursorY < m_UpperCliLimit)
    m_CursorY = m_UpperCliLimit;
}

void LocalIO::setCliLowerLimit(int nlines)
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
  for (int i = 0; i < CONSOLE_WIDTH*CONSOLE_HEIGHT; i++)
  {
    unsigned char attributeByte = (DebuggerIO::Black << 4) | (DebuggerIO::White & 0x0F);
    unsigned short blank = ' ' | (attributeByte << 8);
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
    unsigned char attributeByte = (DebuggerIO::Black << 4) | (DebuggerIO::White & 0x0F);
    unsigned short blank = ' ' | (attributeByte << 8);
    m_pFramebuffer[i] = blank;
  }
  
  // Reposition the cursor.
  m_CursorX = 0;
  m_CursorY = m_UpperCliLimit;
  
  forceRefresh();
}

void LocalIO::writeCli(const char *str, DebuggerIO::Colour foreColour, DebuggerIO::Colour backColour)
{
  // We want to disable refreshes during writing, so the screen only gets updated once.
  bool bRefreshWasEnabled = false;
  if (m_bRefreshesEnabled)
  {
    bRefreshWasEnabled = true;
    m_bRefreshesEnabled = false;
  }

  // For every character, call writeCli.
  while (*str)
    writeCli(*str++, foreColour, backColour);

  // If refreshes were enabled to start with, reenable them, and force a refresh.
  if (bRefreshWasEnabled)
  {
    m_bRefreshesEnabled = true;
    forceRefresh();
  }
}

void LocalIO::writeCli(char c, DebuggerIO::Colour foreColour, DebuggerIO::Colour backColour)
{
  // Write the character to the current cursor position.
  putChar(c, foreColour, backColour);
  
  // Scroll if required.
  scroll();

  // Move the cursor.
  moveCursor();

  if (m_bRefreshesEnabled)
    forceRefresh();

}

bool LocalIO::readCli(HugeStaticString &str, DebuggerCommand *pAutoComplete)
{
  // Are we ready to recieve another command?
  if (m_bReady)
  {
    // Yes, output a prompt...
    writeCli("(db) ", DebuggerIO::LightGrey, DebuggerIO::Black);
    // ...and zero the command string.
    m_pCommand[0] = '\0';
    m_bReady = false;
  }

  char ch = 0;
  // Spin in a loop until we get a printable character. getChar returns 0 if a non-printing
  // character is recieved.
  while ( !(ch = getChar()) )
    ;

  // Was this a newline?
  if (ch == '\n')
  {
    // Command finished, we're ready for the next.
    m_bReady = true;
    writeCli(ch, DebuggerIO::White, DebuggerIO::Black);
  }
  else
  {
    // We've got a character, try and append it to our command string.
    // But first, was it a backspace?
    if (ch == 0x08)
    {
      // Try and erase one letter of the command string.
      if (strlen(m_pCommand))
      {
        m_pCommand[strlen(m_pCommand)-1] = '\0';
      
        writeCli(ch, DebuggerIO::White, DebuggerIO::Black);
      }
    }
    // Tab?
    else if (ch == 0x09)
    {
      if (pAutoComplete)
      {
        // Get the full autocomplete string.
        const char *pACString = pAutoComplete->getString();
        // HACK:: Here we hack like complete bitches. Just find the last space in the string,
        // and memcpy the full autocomplete string in.
        int i;
        for (i = strlen(m_pCommand); i >= 0; i--)
          if (m_pCommand[i] == ' ')
            break;
        
        // We also haxxor the cursor, by writing loads of backspaces, then rewriting the whole string.
        int nBackspaces = strlen(m_pCommand)-i;
        for (int j = 0; j < nBackspaces-1; j++)
          putChar('\x08' /* backspace */, DebuggerIO::White, DebuggerIO::Black);
        
        writeCli(pACString, DebuggerIO::White, DebuggerIO::Black);
        
        // Memcpy the full autocomplete string in.
        memcpy(&m_pCommand[i+1], pACString, strlen(pACString)+1);
      }
    }
    else
    {
      // Normal, printing character.
      int len = strlen(m_pCommand);
      if (len < COMMAND_MAX-1)
      {
        // Add it to the command string, and null terminate.
        m_pCommand[len] = ch;
        m_pCommand[len+1] = '\0';
        // And echo it back to the screen, too.
        writeCli(ch, DebuggerIO::White, DebuggerIO::Black);
      }
    }
  }

  // Now do a strncpy to the target string.
  str = static_cast<const char *>(m_pCommand);
  
  return m_bReady;
}

char LocalIO::getChar()
{

  // Let's get a character from the keyboard.
  IoPort port;
  port.allocate(0x60, 4);
  unsigned char scancode, status;
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

  return 0;
}

void LocalIO::drawHorizontalLine(char c, int row, int colStart, int colEnd, DebuggerIO::Colour foreColour, DebuggerIO::Colour backColour)
{
  // colEnd must be bigger than colStart.
  if (colStart > colEnd)
  {
    int tmp = colStart;
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

  unsigned char attributeByte = (backColour << 4) | (foreColour & 0x0F);
  for(int i = colStart; i <= colEnd; i++)
  {
    m_pFramebuffer[row*CONSOLE_WIDTH+i] = c | (attributeByte << 8);
  }
  
  if (m_bRefreshesEnabled)
    forceRefresh();
}

void LocalIO::drawVerticalLine(char c, int col, int rowStart, int rowEnd, DebuggerIO::Colour foreColour, DebuggerIO::Colour backColour)
{
  // rowEnd must be bigger than rowStart.
  if (rowStart > rowEnd)
  {
    int tmp = rowStart;
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
  
  unsigned char attributeByte = (backColour << 4) | (foreColour & 0x0F);
  for(int i = rowStart; i <= rowEnd; i++)
  {
    m_pFramebuffer[i*CONSOLE_WIDTH+col] = c | (attributeByte << 8);
  }
  
  if (m_bRefreshesEnabled)
    forceRefresh();
}

void LocalIO::drawString(const char *str, int row, int col, DebuggerIO::Colour foreColour, DebuggerIO::Colour backColour)
{
  // Now then, this is a lesson in cheating. Listen up.
  // Firstly we save the current cursorX and cursorY positions.
  int savedX = m_CursorX;
  int savedY = m_CursorY;
  
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
  unsigned short *vidmem = reinterpret_cast<unsigned short*>(0xB8000);

  memcpy(vidmem, m_pFramebuffer, CONSOLE_WIDTH*CONSOLE_HEIGHT*2);
  moveCursor();
}

void LocalIO::scroll()
{
  // Get a space character with the default colour attributes.
  unsigned char attributeByte = (DebuggerIO::Black << 4) | (DebuggerIO::White & 0x0F);
  unsigned short blank = ' ' | (attributeByte << 8);
  if (m_CursorY >= CONSOLE_HEIGHT-m_LowerCliLimit)
  {
    for (int i = m_UpperCliLimit*80; i < (CONSOLE_HEIGHT-m_LowerCliLimit-1)*80; i++)
      m_pFramebuffer[i] = m_pFramebuffer[i+80];

    for (int i = (CONSOLE_HEIGHT-m_LowerCliLimit-1)*80; i < (CONSOLE_HEIGHT-m_LowerCliLimit)*80; i++)
      m_pFramebuffer[i] = blank;

    m_CursorY = CONSOLE_HEIGHT-m_LowerCliLimit-1;
  }

}

void LocalIO::moveCursor()
{
  unsigned short tmp = m_CursorY*80 + m_CursorX;
  
  IoPort cursorPort;
  cursorPort.allocate(0x3D4, 2);
  
  cursorPort.write8(14, 0);
  cursorPort.write8(tmp>>8, 1);
  cursorPort.write8(15, 0);
  cursorPort.write8(tmp, 1);
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
    unsigned char attributeByte = (backColour << 4) | (foreColour & 0x0F);
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
    unsigned char attributeByte = (backColour << 4) | (foreColour & 0x0F);
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
