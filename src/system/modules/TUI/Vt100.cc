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

#include "Vt100.h"
#include "../../kernel/core/BootIO.h"
#include <machine/Machine.h>
#include <machine/Keyboard.h>
#include "Font.c"

#define FONT_HEIGHT 16
#define FONT_WIDTH  8

#define NUM_PAGES_SCROLLBACK 3
#define NUM_PAGES_ALT_SCROLLBACK 2

#define LINE(x) ((x)%m_nLines)
#define ALTLINE(x) ((x)%m_nAlternateLines)

Vt100::Vt100(Display::ScreenMode mode, void *pFramebuffer) :
  m_Mode(mode), m_pFramebuffer(reinterpret_cast<uint8_t*>(pFramebuffer)), m_CursorX(0), m_CursorY(0),
  m_nWidth(mode.width/FONT_WIDTH), m_nHeight(mode.height/FONT_HEIGHT), m_WinViewStart(0), m_AlternateWinViewStart(0), m_bIsAlternateWindow(false), m_pLines(0), m_pAlternateLines(0),
  m_nLines(m_nHeight*NUM_PAGES_SCROLLBACK), m_nAlternateLines(m_nHeight*NUM_PAGES_ALT_SCROLLBACK), m_nLinesPos(0),
  m_nAlternateLinesPos(0), m_nLastPos(0), m_nAlternateLastPos(0), m_bChangingState(false), m_bDontRefresh(false)
{
  // Precompile colour list.
  m_pColours[C_BLACK] = compileColour(0x00, 0x00, 0x00);
  m_pColours[C_RED]   = compileColour(0xB0, 0x00, 0x00);
  m_pColours[C_GREEN] = compileColour(0x00, 0xB0, 0x00);
  m_pColours[C_YELLOW]= compileColour(0xB0, 0xB0, 0x00);
  m_pColours[C_BLUE]  = compileColour(0x00, 0x00, 0xB0);
  m_pColours[C_MAGENTA]=compileColour(0xB0, 0x00, 0xB0);
  m_pColours[C_CYAN]  = compileColour(0x00, 0xB0, 0xB0);
  m_pColours[C_WHITE] = compileColour(0xB0, 0xB0, 0xB0);

  m_pColours[C_BRIGHT + C_BLACK] = compileColour(0x00, 0x00, 0x00);
  m_pColours[C_BRIGHT + C_RED]   = compileColour(0xFF, 0x00, 0x00);
  m_pColours[C_BRIGHT + C_GREEN] = compileColour(0x00, 0xFF, 0x00);
  m_pColours[C_BRIGHT + C_YELLOW]= compileColour(0xFF, 0xFF, 0x00);
  m_pColours[C_BRIGHT + C_BLUE]  = compileColour(0x00, 0x00, 0xFF);
  m_pColours[C_BRIGHT + C_MAGENTA]=compileColour(0xFF, 0x00, 0xFF);
  m_pColours[C_BRIGHT + C_CYAN]  = compileColour(0x00, 0xFF, 0xFF);
  m_pColours[C_BRIGHT + C_WHITE] = compileColour(0xFF, 0xFF, 0xFF);

  // Create the lines queue.
  m_pLines = new Line[m_nLines];
  m_pAlternateLines = new Line[m_nAlternateLines];

  // Create the initial state.
  m_State.fg = m_pColours[C_WHITE];
  m_State.bg = m_pColours[C_BLACK];

  // Initialise all lines to a consistent state.
  for (int i = 0; i < m_nLines; i++)
  {
    m_pLines[i].str = 0;
    m_pLines[i].col = 0;
    m_pLines[i].rows = 0;
  }
  for (int i = 0; i < m_nAlternateLines; i++)
  {
    m_pAlternateLines[i].str = 0;
    m_pAlternateLines[i].col = 0;
    m_pAlternateLines[i].rows = 0;
  }

  // Invariant - current insert line is constructed and has a valid 'str' member.
  m_pLines[0].str = new char[m_nWidth];
  memset(m_pLines[0].str, 0, m_nWidth);
  m_pLines[0].initialState = m_State;
  m_pLines[0].rows = 1;

  m_pAlternateLines[0].str = new char[m_nWidth];
  memset(m_pAlternateLines[0].str, 0, m_nWidth);
  m_pAlternateLines[0].initialState = m_State;
  m_pAlternateLines[0].rows = 1;

}

Vt100::~Vt100()
{
  delete [] m_pLines;
  delete [] m_pAlternateLines;
}

void Vt100::write(char *str)
{
  m_bDontRefresh = true;
  uint32_t refreshX = m_CursorX;
  uint32_t refreshY = m_CursorY;

  while (*str)
    write(*str++);

  m_bDontRefresh = false;
  refresh(refreshY, 0/*refreshX*/);
}

void Vt100::write(char c)
{
  Line *pLines;
  uint32_t nLines;
  if (m_bIsAlternateWindow)
  {
    pLines = m_pAlternateLines;
    nLines = m_nAlternateLines;
  }
  else
  {
    pLines = m_pLines;
    nLines = m_nLines;
  }

  if (m_bChangingState)
  {
    // A VT100 command is being received.
    if (c == '[') return; // Useless character.

    switch (c)
    {
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        m_Cmd.params[m_Cmd.cur_param] = m_Cmd.params[m_Cmd.cur_param] * 10 + (c-'0');
        break;
      case ';':
        m_Cmd.cur_param++;
        break;
      default:
        WARNING("VT100: Invalid character: " << c);
        m_bChangingState = false;
        break;
    }
  }
  else
  {
    switch (c)
    {
      // Backspace.
      case 0x08:
      {
        if (pLines[m_CursorY].col > 0)
        {
          //memmove(&pLines[m_CursorY].str[m_CursorX-1], &pLines[m_CursorY].str[m_CursorX], pLines[m_CursorY].col-m_CursorX);
          //if ( (pLines[m_CursorY].col / m_nWidth) != ((pLines[m_CursorY].col - 1) / m_nWidth) )
          //  pLines[m_CursorY].rows --;
          //pLines[m_CursorY].col --;
          // Don't erase the character, just move the cursor!
          m_CursorX --;
          refresh(m_CursorY, (m_CursorX != 0) ? 0/*(m_CursorX-1)*/ : 0);
        }
        break;
      }

      // Newline.
      case '\n':
      {
        // Save the old cursor position for rendering from.
        uint32_t renderX = m_CursorX;
        uint32_t renderY = m_CursorY;
        // Create a new Line.
        m_CursorY = (m_CursorY+1) % nLines;
        pLines[m_CursorY].col = 0;
        pLines[m_CursorY].rows = 1;
        pLines[m_CursorY].initialState = m_State;
        for (List<StateChange*>::Iterator it = pLines[m_CursorY].changes.begin();
             it != pLines[m_CursorY].changes.end();
             it ++)
          delete (*it);
        pLines[m_CursorY].changes.clear();
        if (pLines[m_CursorY].str == 0)
          pLines[m_CursorY].str = new char[m_nWidth];

        // If the new line is being created and the last line was the insert position, this line is now the insert position.
        if (m_bIsAlternateWindow && (m_CursorY-1 == m_nAlternateLinesPos))
          m_nAlternateLinesPos ++;
        else if (!m_bIsAlternateWindow && (m_CursorY-1 == m_nLinesPos))
          m_nLinesPos ++;

        ensureLineDisplayed(m_CursorY);

        m_CursorX = 0;

        // The state may have changed from the cursor position onwards, so render from there.
        refresh(renderY, renderX);
        break;
      }

      case '\r':
      {
        m_CursorX = 0;
        refresh(m_CursorY, 0);
        break;
      }

      // VT100 command - changes mode.
      case '\e':
      {
        m_bChangingState = true;
        m_Cmd.cur_param = 0;
        m_Cmd.params[0] = 0;
        m_Cmd.params[1] = 0;
        m_Cmd.params[2] = 0;
        m_Cmd.params[3] = 0;
        break;
      }

      case '\t':
      {
        m_CursorX = (m_CursorX+8) & ~(8-1);
        for (uint32_t i = pLines[m_CursorY].col; i < m_CursorX; i++)
        {
          pLines[m_CursorY].str[pLines[m_CursorY].col++] = ' ';
        }
        break;
      }

      case '\007':
      {
        // Bell.
        break;
      }

      // Any other character.
      default:
      {
        // Is this going to push us over the edge onto another row?
        if ( (pLines[m_CursorY].col / m_nWidth) != ((pLines[m_CursorY].col + 1) / m_nWidth) )
        {
          pLines[m_CursorY].rows ++;
          ensureLineDisplayed(m_CursorY); // This will handle scrolling in the case the line is now off the screen.
        }
        NOTICE("C: " << (uint8_t)c);
        // Add the character.
        pLines[m_CursorY].str[m_CursorX] = c;
        if (m_CursorX == pLines[m_CursorY].col) pLines[m_CursorY].col++; 
        m_CursorX ++;
        /// \todo Check for str becoming too large and realloc.

        // The state may have changed from the cursor position onwards, so render from there.
        refresh(m_CursorY, (m_CursorX != 0) ? 0/*(m_CursorX-1)*/ : 0);
      }
    }
  }

}

void Vt100::putChar(char c, int x, int y, uint32_t f, uint32_t b)
{
  int depth = m_Mode.pf.nBpp;
  if (depth != 8 && depth != 16 && depth != 32)
  {
    ERROR("VT100: Pixel format is neither 8, 16 or 32 bits per pixel!");
    return;
  }
  int m_Stride = m_Mode.pf.nPitch/ (depth/8);

  x *= FONT_WIDTH;
  y *= FONT_HEIGHT;

  int idx = static_cast<int>(c) * FONT_HEIGHT;
  uint16_t *p16Fb = reinterpret_cast<uint16_t*> (m_pFramebuffer);
  uint32_t *p32Fb = reinterpret_cast<uint32_t*> (m_pFramebuffer);
  for (int i = 0; i < FONT_HEIGHT; i++)
  {
    unsigned char row = ppc_font[idx+i];
    for (int j = 0; j < FONT_WIDTH; j++)
    {
      unsigned int col;
      if ( (row & (0x80 >> j)) != 0 )
      {
        col = f;
      }
      else
      {
        col = b;
      }

      if (depth == 8)
        m_pFramebuffer[y*m_Stride + i*m_Stride + x + j] = col&0xFF;
      else if (depth == 16)
        p16Fb[y*m_Stride + i*m_Stride + x + j] = col&0xFFFF;
      else
        p32Fb[y*m_Stride + i*m_Stride + x + j] = col;
    }
  }
}

uint32_t Vt100::compileColour(uint8_t r, uint8_t g, uint8_t b)
{
  Display::PixelFormat pf = m_Mode.pf;

  // Calculate the range of the Red field.
  uint8_t range = 1 << pf.mRed;

  // Clamp the red value to this range.
  r = (r * range) / 256;

  range = 1 << pf.mGreen;

  // Clamp the green value to this range.
  g = (g * range) / 256;

  range = 1 << pf.mBlue;

  // Clamp the blue value to this range.
  b = (b * range) / 256;

  // Assemble the colour.
  return 0 |
         (static_cast<uint32_t>(r) << pf.pRed) |
         (static_cast<uint32_t>(g) << pf.pGreen) |
         (static_cast<uint32_t>(b) << pf.pBlue);
}

void Vt100::refresh(int32_t line, int32_t col)
{
  if (m_bDontRefresh) return;
NOTICE("Refresh: l:" << Dec << line << ", c:" << col);
  uint32_t winStart = (m_bIsAlternateWindow) ? m_AlternateWinViewStart : m_WinViewStart;
  Line *pLines = (m_bIsAlternateWindow) ? m_pAlternateLines : m_pLines;
  uint32_t nLines = (m_bIsAlternateWindow) ? m_nAlternateLines : m_nLines;
  uint32_t pos = (m_bIsAlternateWindow) ? m_nAlternateLinesPos : m_nLinesPos;

  bool bStartedRendering = false;
  for (int i = 0; i < m_nHeight;)
  {
    if (!bStartedRendering && ((i + winStart) % nLines) != line)
    {
      i += pLines[ (i + winStart) % nLines ].rows;
      continue;
    }
    bStartedRendering = true;
    State state = renderLine(&pLines[ (i + winStart) % nLines ], i, col, (m_CursorY == ((i+winStart)%nLines)) ? m_CursorX : -1);
    col = 0;
    if ( ((i + winStart) % nLines) == pos )
      break;
    i += pLines[ (i + winStart) % nLines ].rows;
    pLines[ (i + winStart) % nLines ].initialState = state;
  }
}

Vt100::State Vt100::renderLine(Line *pLine, int row, int colStart, int cursorCol)
{
  // Render pLine to 'row' on the screen, starting at colStart. Away we go!
  State state = pLine->initialState;
  // We make the assumption here that the list of state changes is ordered.
  List<StateChange*>::Iterator it = pLine->changes.begin();

  int x = 0;
  int i;
  for (i = 0; i < pLine->col; i++,x++)
  {
    if ((it != pLine->changes.end()) && ((*it)->col == i))
    {
      if ((*it)->fg != ~0) state.fg = (*it)->fg;
      if ((*it)->bg != ~0) state.bg = (*it)->bg;
      it ++;
    }

    if (x == m_nWidth)
    {
      /// \todo Here we have to decide whether to wrap or not.
      row++;
      x = 0;
    }

    if (i < colStart) continue;
    if (i == cursorCol)
      putChar (pLine->str[i], x, row, state.bg, state.fg);
    else
      putChar (pLine->str[i], x, row, state.fg, state.bg);
  }

  if (i != m_nWidth)
  {
    for (; i < m_nWidth; i++,x++)
    {
      if (x == m_nWidth)
      {
        /// \todo Here we have to decide whether to wrap or not.
        row++;
        x = 0;
      }
      if (i == cursorCol)
        putChar (' ', x, row, state.bg, state.fg);
      else
        putChar (' ', x, row, state.fg, state.bg);
    }
  }

  return state;
}

void Vt100::ensureLineDisplayed(int l)
{
  // Is this line currently in the viewable region?
  uint32_t &winStart = (m_bIsAlternateWindow) ? m_AlternateWinViewStart : m_WinViewStart;
  uint32_t pos = (m_bIsAlternateWindow) ? m_nAlternateLinesPos : m_nLinesPos;
  uint32_t nLines = (m_bIsAlternateWindow) ? m_nAlternateLines : m_nLines;
  Line *pLines = (m_bIsAlternateWindow) ? m_pAlternateLines : m_pLines;

  // We have to advance linearly through the lines when calculating the end of the window in case we encounter the last line,
  // at which point we must stop. Linearly, because we're using a circular queue so because of the wraparound we can't use
  // greater-than/less-than comparison ops.
  uint32_t winEnd = winStart;
  for (uint32_t i = 0; i < m_nHeight;)
  {
    if (winEnd == l)
      // Line is visible, stop.
      return;
    if (winEnd == pos) break;
    i += pLines[winEnd].rows;
    winEnd = (winEnd+1) % nLines;
  }

  // This is a speedup for the common case, where a newline character is entered and we fall one line off the screen (i.e. screen
  // needs scrolling one line up.
  if (l == winEnd)
  {
    winStart++;
    int bytespp = m_Mode.pf.nBpp / 8;
    memmove(reinterpret_cast<void*>(m_pFramebuffer), reinterpret_cast<void*>(&m_pFramebuffer[m_nWidth*FONT_WIDTH*FONT_HEIGHT*bytespp]),
            m_nWidth*FONT_WIDTH*(m_nHeight-1)*FONT_HEIGHT*bytespp);
    return;
  }

  // The line is not in the current visible region. We want to scroll so that we can see 'l', but not so that we're off the
  // end of the screen, so start at the insert position and work backwards until we find 'l'.
  // NOTE: We add nLines in before the modulus to ensure that the number being modded is positive - % doesn't work on negative values.
  uint32_t lastPossibleWinStart = (pos - m_nHeight + 1 + nLines) % nLines;

  for (int32_t i = 0; i < m_nHeight;)
  {
    if ( ((pos - i + nLines) % nLines) == l )
    {
      winStart = lastPossibleWinStart;
      refresh(winStart, 0);
      return;
    }
    i += pLines[(pos - i + nLines) % nLines].rows;
  }

  // Wasn't found in the first screen's-worth, so just set the window offset as 'l'.
  winStart = l;
  refresh(winStart, 0);
}
