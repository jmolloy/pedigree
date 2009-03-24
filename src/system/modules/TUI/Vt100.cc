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

Vt100::Vt100(Display::ScreenMode mode, void *pFramebuffer) :
  m_Mode(mode), m_pFramebuffer(reinterpret_cast<uint8_t*>(pFramebuffer)),
  m_nWidth(mode.width/FONT_WIDTH), m_nHeight(mode.height/FONT_HEIGHT),
  m_CurrentWindow(0), m_bChangingState(false), m_bContainedBracket(false), m_bDontRefresh(false),
  m_SavedX(0), m_SavedY(0)
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

  m_pColours[C_BRIGHT + C_BLACK] = compileColour(0x33, 0x33, 0x33);
  m_pColours[C_BRIGHT + C_RED]   = compileColour(0xFF, 0x33, 0x33);
  m_pColours[C_BRIGHT + C_GREEN] = compileColour(0x33, 0xFF, 0x33);
  m_pColours[C_BRIGHT + C_YELLOW]= compileColour(0xFF, 0xFF, 0x33);
  m_pColours[C_BRIGHT + C_BLUE]  = compileColour(0x33, 0x33, 0xFF);
  m_pColours[C_BRIGHT + C_MAGENTA]=compileColour(0xFF, 0x33, 0xFF);
  m_pColours[C_BRIGHT + C_CYAN]  = compileColour(0x33, 0xFF, 0xFF);
  m_pColours[C_BRIGHT + C_WHITE] = compileColour(0xFF, 0xFF, 0xFF);

  // Create the windows.
  m_pWindows[0] = new Window(m_nWidth, m_nHeight, this);
  m_pWindows[1] = new Window(m_nWidth, m_nHeight, this);
}

Vt100::~Vt100()
{
  delete [] m_pWindows;
}

void Vt100::write(char *str)
{
  while (*str)
    write(*str++);
}

void Vt100::write(char c)
{
  //NOTICE("VT100: write: " << c << " (" << Hex << (uintptr_t)c << ")");

  if (m_bChangingState)
  {
    // A VT100 command is being received.
    if (c == '?') return; // Useless character.

    switch (c)
    {
      case '[':
        m_bContainedBracket = true;
        break;
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
      case 'h':
        m_CurrentWindow = 1;
        m_pWindows[m_CurrentWindow]->refresh();
        m_bChangingState = false;
        break;
      case 'l':
        m_CurrentWindow = 0;
        m_pWindows[m_CurrentWindow]->refresh();
        m_bChangingState = false;
        break;
      case 'H':
        m_pWindows[m_CurrentWindow]->setCursorX( (m_Cmd.params[1]) ? m_Cmd.params[1]-1 : 0);
        m_pWindows[m_CurrentWindow]->setCursorY( (m_Cmd.params[0]) ? m_Cmd.params[0]-1 : 0); // One-indexed,but 0,0 is valid too (means 1,1).
        m_bChangingState = false;
        break;
      case 'A':
        m_pWindows[m_CurrentWindow]->setCursorY( m_pWindows[m_CurrentWindow]->getCursorY() -
                                                 ((m_Cmd.params[0]) ? m_Cmd.params[0] : 1) );
        m_bChangingState = false;
        break;
      case 'B':
        m_pWindows[m_CurrentWindow]->setCursorY( m_pWindows[m_CurrentWindow]->getCursorY() +
                                                 ((m_Cmd.params[0]) ? m_Cmd.params[0] : 1) );
        m_bChangingState = false;
        break;
      case 'C':
        m_pWindows[m_CurrentWindow]->setCursorX( m_pWindows[m_CurrentWindow]->getCursorX() +
                                                 ((m_Cmd.params[0]) ? m_Cmd.params[0] : 1) );
        m_bChangingState = false;
        break;
      case 'D':
        if (m_bContainedBracket)
        {
          // If it contained a bracket, it's a cursor command.
          m_pWindows[m_CurrentWindow]->setCursorX( m_pWindows[m_CurrentWindow]->getCursorX() -
                                                  ((m_Cmd.params[0]) ? m_Cmd.params[0] : 1) );
        }
        else
        {
          // Else, it's a scroll downwards command.
          m_pWindows[m_CurrentWindow]->scrollDown();
        }
        m_bChangingState = false;
        break;
      case 'M':
        m_pWindows[m_CurrentWindow]->scrollUp();
        m_bChangingState = false;
        break;
      case 'J':
        switch (m_Cmd.params[0])
        {
          case 0: // Erase down.
            m_pWindows[m_CurrentWindow]->eraseDown();
            break;
          case 1: // Erase up.
            m_pWindows[m_CurrentWindow]->eraseUp();
            break;
          case 2: // Erase entire screen and move to home.
            m_pWindows[m_CurrentWindow]->eraseScreen();
            break;
        }
        m_bChangingState = false;
      case 'K':
        switch (m_Cmd.params[0])
        {
          case 0: // Erase end of line.
            m_pWindows[m_CurrentWindow]->eraseEOL();
            break;
          case 1: // Erase start of line.
            m_pWindows[m_CurrentWindow]->eraseSOL();
            break;
          case 2: // Erase entire line.
            m_pWindows[m_CurrentWindow]->eraseLine();
            break;
        }
        m_bChangingState = false;
      case 'r':
        m_pWindows[m_CurrentWindow]->setScrollRegion( (m_Cmd.params[0]) ? m_Cmd.params[0]-1 : ~0,
                                                      (m_Cmd.params[1]) ? m_Cmd.params[1]-1 : ~0);
        m_bChangingState = false;
        break;
      case 'm':
      {
        // Colours!
        for (int i = 0; i < m_Cmd.cur_param+1; i++)
        {
          switch (m_Cmd.params[i])
          {
            case 0:
              // Reset all attributes.
              m_pWindows[m_CurrentWindow]->setBold(false);
              m_pWindows[m_CurrentWindow]->setFore(C_WHITE);
              m_pWindows[m_CurrentWindow]->setBack(C_BLACK);
              break;
            case 1:
              // Bold
              m_pWindows[m_CurrentWindow]->setBold(true);
              break;
            case 7:
            {
              // Inverse
              m_pWindows[m_CurrentWindow]->setBold(false);
              uint8_t tmp = m_pWindows[m_CurrentWindow]->getFore();
              m_pWindows[m_CurrentWindow]->setFore(m_pWindows[m_CurrentWindow]->getBack());
              m_pWindows[m_CurrentWindow]->setBack(tmp);
              break;
            }
            case 30:
            case 31:
            case 32:
            case 33:
            case 34:
            case 35:
            case 36:
            case 37:
              // Foreground.
              m_pWindows[m_CurrentWindow]->setFore(m_Cmd.params[i]-30);
              break;
            case 40:
            case 41:
            case 42:
            case 43:
            case 44:
            case 45:
            case 46:
            case 47:
              // Background.
              m_pWindows[m_CurrentWindow]->setBack(m_Cmd.params[i]-40);
              break;
            default:
              // Do nothing.
              break;
          }
        }
        m_bChangingState = false;
      }
      case '\e':
        // We received another VT100 command while expecting a terminating command - this must mean it's one of \e7 or \e8.
        switch (m_Cmd.params[0])
        {
          case 7: // Save cursor.
            m_SavedX = m_pWindows[m_CurrentWindow]->getCursorX();
            m_SavedY = m_pWindows[m_CurrentWindow]->getCursorY();
            m_Cmd.cur_param = 0; m_Cmd.params[0] = 0;
            break;
          case 8: // Restore cursor.
            m_pWindows[m_CurrentWindow]->setCursorX(m_SavedX);
            m_pWindows[m_CurrentWindow]->setCursorY(m_SavedY);
            m_Cmd.cur_param = 0; m_Cmd.params[0] = 0;
            break;
        }
        // We're still changing state, so keep m_bChangingState = true.
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
        if (m_pWindows[m_CurrentWindow]->getCursorX() > 0)
          m_pWindows[m_CurrentWindow]->setCursorX (m_pWindows[m_CurrentWindow]->getCursorX()-1);
        break;

      // Newline.
      case '\n':
        // Newline is a cursor down and carriage return operation.
        m_pWindows[m_CurrentWindow]->setCursorY (m_pWindows[m_CurrentWindow]->getCursorY()+1);
        // Fall through...

      case '\r':
        m_pWindows[m_CurrentWindow]->setCursorX (0);
        break;

      // VT100 command - changes mode.
      case '\e':
        m_bChangingState = true;
        m_bContainedBracket = false;
        m_Cmd.cur_param = 0;
        m_Cmd.params[0] = 0;
        m_Cmd.params[1] = 0;
        m_Cmd.params[2] = 0;
        m_Cmd.params[3] = 0;
        break;

      case '\t':
        m_pWindows[m_CurrentWindow]->setCursorX ( (m_pWindows[m_CurrentWindow]->getCursorX ()+8) & ~(8-1) );
        break;

      case '\007':
      {
        // Bell.
        break;
      }

      case 0xf:
        break; // SHIFT-IN. No use at all.

      // Any other character.
      default:
        // Add the character.
        m_pWindows[m_CurrentWindow]->writeChar(c);
    }
  }

}

void Vt100::putCharFb(char c, int x, int y, uint32_t f, uint32_t b)
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

Vt100::Window::Window(uint32_t nWidth, uint32_t nHeight, Vt100 *pParent) :
  m_CursorX(0), m_CursorY((NUM_PAGES_SCROLLBACK-1)*nHeight), m_nWidth(nWidth), m_nHeight(nHeight),
  m_nScrollMin(0), m_nScrollMax(NUM_PAGES_SCROLLBACK*nHeight-1),
  m_Foreground(C_WHITE), m_Background(C_BLACK), m_bBold(false), m_pParent(pParent), m_View((NUM_PAGES_SCROLLBACK-1)*nHeight),
  m_pData(0)
{
  m_pData = new uint16_t[NUM_PAGES_SCROLLBACK*nHeight*nWidth];
  for (int i = 0; i < NUM_PAGES_SCROLLBACK*nHeight*nWidth; i++)
  {
    m_pData[i] = static_cast<uint16_t>(' ') | (C_WHITE<<12) | (C_BLACK<<8);
  }
}

Vt100::Window::~Window()
{
  delete [] m_pData;
}

void Vt100::Window::writeChar(char c)
{
  m_pData[m_CursorX + m_CursorY*m_nWidth] = c | (m_Foreground<<12) | (m_Background<<8);
  m_pParent->putCharFb(c, m_CursorX, m_CursorY-m_View, m_pParent->m_pColours[m_Foreground], m_pParent->m_pColours[m_Background]);
  m_CursorX++;
  if (m_CursorX == m_nWidth)
  {
    m_CursorX = 0;
    setCursorY (m_CursorY+1-m_View);
    // setCursorY will do the rest of the refresh - we don't have to.
  }
  else
  {
    // Draw the new cursor.
    m_pParent->putCharFb(' ', m_CursorX, m_CursorY-m_View, m_pParent->m_pColours[m_Background], m_pParent->m_pColours[m_Foreground]);
  }
}

void Vt100::Window::setCursorX(uint32_t x)
{
  // Re-render whatever was at the cursor position (without the inverted colours).
  uint16_t data = m_pData[m_CursorX + m_CursorY*m_nWidth];
  m_pParent->putCharFb(data&0xFF, m_CursorX, m_CursorY-m_View, m_pParent->m_pColours[(data>>12)&0xF], m_pParent->m_pColours[(data>>8)&0xF]);

  // Now adjust the cursor and render.
  // We assume here that we won't be going off the screen - that should only be done when chars get written!
  m_CursorX = x;
  data = m_pData[m_CursorX + m_CursorY*m_nWidth];
  // Render with inverse colours.
  m_pParent->putCharFb(data&0xFF, m_CursorX, m_CursorY-m_View, m_pParent->m_pColours[(data>>8)&0xF], m_pParent->m_pColours[(data>>12)&0xF]);
}

void Vt100::Window::setCursorY(uint32_t y)
{
  // Re-render whatever was at the cursor position (without the inverted colours).
  uint16_t data = m_pData[m_CursorX + m_CursorY*m_nWidth];
  m_pParent->putCharFb(data&0xFF, m_CursorX, m_CursorY-m_View, m_pParent->m_pColours[(data>>12)&0xF], m_pParent->m_pColours[(data>>8)&0xF]);
  
  // Now adjust the cursor and render.
  m_CursorY = y+m_View;
  

  // Have we gone off the screen?
  if (m_CursorY > m_nScrollMax)
  {
    // Assume that we can only go off the screen by one line... (bad but nice assumption).
    //m_CursorY --; // Bring the cursor back onto the screen.
    
    size_t nLines = m_CursorY - m_nScrollMax;
    m_CursorY -= nLines;

    // If the scroll region is set (m_nScrollMin != 0), only scroll that region.
    memmove (reinterpret_cast<uint8_t*>(&m_pData[m_nScrollMin*m_nWidth]),
             reinterpret_cast<uint8_t*>(&m_pData[(m_nScrollMin+nLines)*m_nWidth]),
             nLines * ((m_nScrollMax-m_nScrollMin)*m_nWidth*2));
    
    // Zero out the last rows.
    size_t base = m_nScrollMax - nLines;
    for(size_t line = 0; line <= nLines; line++)
    {
      for (int i = 0; i < m_nWidth; i++)
      {
        m_pData[i+(base + line)*m_nWidth] = static_cast<uint16_t>(' ') | (C_WHITE<<12) | (C_BLACK<<8);
      }
    }

    // Refresh all.
    refresh();
  }
  else
  {
    // Else we haven't gone off the screen, so we can just render the new cursor.
    data = m_pData[m_CursorX + m_CursorY*m_nWidth];
    m_pParent->putCharFb(data&0xFF, m_CursorX, m_CursorY-m_View, m_pParent->m_pColours[(data>>8)&0xF], m_pParent->m_pColours[(data>>12)&0xF]);
  }
}

void Vt100::Window::setScrollRegion(uint32_t start, uint32_t end)
{
  if (start == ~0 && end == ~0)
  {
    m_nScrollMin = 0;
    m_nScrollMax = m_View+m_nHeight-1;
  }
  else
  {
    if((start+m_View) > (end+m_View))
    {
      ERROR("m_nScrollMin > m_nScrollMax [" << start << ", " << end << ", " << m_View << "]");
      return;
    }
    m_nScrollMin = start+m_View;
    m_nScrollMax = end+m_View;
  }
}

void Vt100::Window::eraseEOL()
{
  int row = m_CursorY;
  for (int col = m_CursorX; col < m_nWidth; col++)
  {
    m_pData[col+row*m_nWidth] = static_cast<uint16_t>(' ') | (C_WHITE<<12) | (C_BLACK<<8);
    if (col == m_CursorX)
      m_pParent->putCharFb(' ', col, row-m_View, m_pParent->m_pColours[C_BLACK], m_pParent->m_pColours[C_WHITE]);
    else
      m_pParent->putCharFb(' ', col, row-m_View, m_pParent->m_pColours[C_WHITE], m_pParent->m_pColours[C_BLACK]);
  }
}

void Vt100::Window::eraseSOL()
{
  int row = m_CursorY;
  for (int col = 0; col <= m_CursorX; col++)
  {
    m_pData[col+row*m_nWidth] = static_cast<uint16_t>(' ') | (C_WHITE<<12) | (C_BLACK<<8);
    if (col == m_CursorX)
      m_pParent->putCharFb(' ', col, row-m_View, m_pParent->m_pColours[C_BLACK], m_pParent->m_pColours[C_WHITE]);
    else
      m_pParent->putCharFb(' ', col, row-m_View, m_pParent->m_pColours[C_WHITE], m_pParent->m_pColours[C_BLACK]);
  }
}

void Vt100::Window::eraseLine()
{
  int row = m_CursorY;
  for (int col = 0; col < m_nWidth; col++)
  {
    m_pData[col+row*m_nWidth] = static_cast<uint16_t>(' ') | (C_WHITE<<12) | (C_BLACK<<8);
    if (col == m_CursorX)
      m_pParent->putCharFb(' ', col, row-m_View, m_pParent->m_pColours[C_BLACK], m_pParent->m_pColours[C_WHITE]);
    else
      m_pParent->putCharFb(' ', col, row-m_View, m_pParent->m_pColours[C_WHITE], m_pParent->m_pColours[C_BLACK]);
  }
}

void Vt100::Window::eraseDown()
{
  for (int row = m_CursorY; row < m_nHeight+m_View; row++)
  {
    for (int col = 0; col < m_nWidth; col ++)
    {
      m_pData[col+row*m_nWidth] = static_cast<uint16_t>(' ') | (C_WHITE<<12) | (C_BLACK<<8);
      if (col == m_CursorX && row == m_CursorY)
        m_pParent->putCharFb(' ', col, row-m_View, m_pParent->m_pColours[C_BLACK], m_pParent->m_pColours[C_WHITE]);
      else
        m_pParent->putCharFb(' ', col, row-m_View, m_pParent->m_pColours[C_WHITE], m_pParent->m_pColours[C_BLACK]);
    }
  }
}

void Vt100::Window::eraseUp()
{
  for (int row = m_View; row <= m_CursorY; row++)
  {
    for (int col = 0; col < m_nWidth; col ++)
    {
      m_pData[col+row*m_nWidth] = static_cast<uint16_t>(' ') | (C_WHITE<<12) | (C_BLACK<<8);
      if (col == m_CursorX && row == m_CursorY)
        m_pParent->putCharFb(' ', col, row-m_View, m_pParent->m_pColours[C_BLACK], m_pParent->m_pColours[C_WHITE]);
      else
        m_pParent->putCharFb(' ', col, row-m_View, m_pParent->m_pColours[C_WHITE], m_pParent->m_pColours[C_BLACK]);
    }
  }
}

void Vt100::Window::eraseScreen()
{
  for (int row = m_View; row < m_nHeight+m_View; row++)
  {
    for (int col = 0; col < m_nWidth; col ++)
    {
      m_pData[col+row*m_nWidth] = static_cast<uint16_t>(' ') | (C_WHITE<<12) | (C_BLACK<<8);
      if (col == m_CursorX && row == m_CursorY)
        m_pParent->putCharFb(' ', col, row-m_View, m_pParent->m_pColours[C_BLACK], m_pParent->m_pColours[C_WHITE]);
      else
        m_pParent->putCharFb(' ', col, row-m_View, m_pParent->m_pColours[C_WHITE], m_pParent->m_pColours[C_BLACK]);
    }
  }
}

void Vt100::Window::refresh()
{
  for (int r = m_View; r < m_View+m_nHeight; r++)
  {
    for (int c = 0; c < m_nWidth; c++)
    {
      uint16_t data = m_pData[c+r*m_nWidth];
      if (c == m_CursorX && r == m_CursorY)
        m_pParent->putCharFb(data&0xFF, c, r-m_View, m_pParent->m_pColours[(data>>8)&0xF], m_pParent->m_pColours[(data>>12)&0xF]);
      else
        m_pParent->putCharFb(data&0xFF, c, r-m_View, m_pParent->m_pColours[(data>>12)&0xF], m_pParent->m_pColours[(data>>8)&0xF]);
    }
  }
}

void Vt100::Window::scrollDown()
{
  // If the scroll region is set (m_nScrollMin != 0), only scroll that region.
  memmove (reinterpret_cast<uint8_t*>(&m_pData[m_nScrollMin*m_nWidth]),
            reinterpret_cast<uint8_t*>(&m_pData[(m_nScrollMin+1)*m_nWidth]),
            (m_nScrollMax-m_nScrollMin)*m_nWidth*2);
  // Zero out the last row.
  for (int i = 0; i < m_nWidth; i++)
  {
    m_pData[i+m_nScrollMax*m_nWidth] = static_cast<uint16_t>(' ') | (C_WHITE<<12) | (C_BLACK<<8);
  }

  refresh();
}

void Vt100::Window::scrollUp()
{

  // If the scroll region is set (m_nScrollMin != 0), only scroll that region.
  memmove (reinterpret_cast<uint8_t*>(&m_pData[(m_nScrollMin+1)*m_nWidth]),
            reinterpret_cast<uint8_t*>(&m_pData[m_nScrollMin*m_nWidth]),
            (m_nScrollMax-m_nScrollMin)*m_nWidth*2);

  // Zero out the first row.
  for (int i = 0; i < m_nWidth; i++)
  {
    m_pData[i+m_nScrollMin*m_nWidth] = static_cast<uint16_t>(' ') | (C_WHITE<<12) | (C_BLACK<<8);
  }

  refresh();
}

void Vt100::Window::setFore(uint8_t c)
{
  m_Foreground = (m_bBold) ? c+C_BRIGHT : c;
}

void Vt100::Window::setBack(uint8_t c)
{
  // No bold for backgrounds!
  m_Background = c;
}

void Vt100::Window::setBold(bool b)
{
  if (m_bBold && !b)
  {
    m_Foreground -= C_BRIGHT;
  }
  else if (!m_bBold && b)
  {
    m_Foreground += C_BRIGHT;
  }
  m_bBold = b;
}
