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

#include "Xterm.h"
#include "Font.c"
#include <string.h>
#include <stdio.h>

#define FONT_HEIGHT 16
#define FONT_WIDTH  8

#define NUM_PAGES_SCROLLBACK 3

extern void log(char*);

Xterm::Xterm(Display::ScreenMode mode, void *pFramebuffer) :
  m_Mode(mode), m_pFramebuffer(reinterpret_cast<uint8_t*>(pFramebuffer)),
  m_nWidth(mode.width/FONT_WIDTH), m_nHeight(mode.height/FONT_HEIGHT),
  m_CurrentWindow(0), m_bChangingState(false), m_bContainedBracket(false), m_bContainedParen(false), m_bDontRefresh(false),
  m_SavedX(0), m_SavedY(0), m_Cmd()
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

Xterm::~Xterm()
{
  delete [] m_pWindows;
}

void Xterm::write(char *str)
{
#if 0
   String s(str);
   char *pStr = const_cast<char*>(static_cast<const char*>(s));
   int j = s.length();
   for (int i = 0; i < j; i++)
   {
     if(pStr[i] == '\e') pStr[i] = '\\';
     if (pStr[i] < 0x20) pStr[i] = '#';
     if (i == 60) {char a = pStr[i]; pStr[i] = '\0';NOTICE("W: " << pStr); pStr[i] = a; i -= 60; j -= 60; pStr = &pStr[60];}
   }
   NOTICE("W: " << pStr);
#endif
  while (*str)
  {
      uint8_t c = static_cast<uint8_t>(*str);
      // Convert from UTF-8
      uint32_t utf32;
      // U+0000 - U+007F
      if ( (c & 0x80) == 0 )
          utf32 = c;

      // U+0080 - U+07FF : 110yyyxx 10xxxxxx
      else if ( (c & 0xE0) == 0xC0 )
      {
          str++;
          uint8_t c2 = *str;
          if ( (c2 & 0xC0) != 0x80 )
          {
              // Malformed utf-8 packet.
              log("Malformed utf-8 sequence.");
              break;
          }
          utf32 = (c2 & 0x3F) | ((c & 0x1F) << 6);
      }

      // U+0800 - U+FFFF : 1110yyyy 10yyyyxx 10xxxxxx
      else if ( (c & 0xF0) == 0xE0 )
      {
          str ++;
          uint8_t c2 = *str;
          if ( (c2 & 0xC0) != 0x80 )
          {
              // Malformed utf-8 packet.
              log("Malformed utf-8 sequence");
              break;
          }
          str ++;
          uint8_t c3 = *str;
          if ( (c3 & 0xC0) != 0x80 )
          {
              // Malformed utf-8 packet.
              log("Malformed utf-8 sequence");
              break;
          }
          utf32 = (c3 & 0x3F) | ((c2&0x3F)<<6) | ((c&0x0F)<<12);
      }

      // U+10000-U+10FFFF : 11110zzz 10zzyyyy 10yyyyxx 10xxxxxx
      {
          str ++;
          uint8_t c2 = *str;
          if ( (c2 & 0xC0) != 0x80 )
          {
              // Malformed utf-8 packet.
              log("Malformed utf-8 sequence");
              break;
          }
          str ++;
          uint8_t c3 = *str;
          if ( (c3 & 0xC0) != 0x80 )
          {
              // Malformed utf-8 packet.
              log("Malformed utf-8 sequence");
              break;
          }
          str ++;
          uint8_t c4 = *str;
          if ( (c4 & 0xC0) != 0x80 )
          {
              // Malformed utf-8 packet.
              log("Malformed utf-8 sequence");
              break;
          }
          utf32 = (c4 & 0x3F) | ((c3&0x3f)<<6) | ((c2&0x3F)<<12) | ((c<<0x07)<<18);
      }

      write(utf32);
  }
}

void Xterm::write(uint32_t utf32)
{
    char c = 0;
    if (utf32 < 0x80) c = utf32;

    log("Xterm::write");

  if (m_bChangingState)
  {
    switch (c)
    {
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

      // Newline without carriage return.
      case '\xB': // VT - vertical tab.
        m_pWindows[m_CurrentWindow]->setCursorY (m_pWindows[m_CurrentWindow]->getCursorY()+1);
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
        m_bContainedParen = false;
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
        if (!m_pWindows[m_CurrentWindow]->getLineDrawingMode())
        {
          // Add the character.
          m_pWindows[m_CurrentWindow]->writeChar(utf32);
        }
        else
        {
          switch (utf32)
          {
            case 'j': utf32 = 188; break; // Lower right corner
            case 'k': utf32 = 187; break; // Upper right corner
            case 'l': utf32 = 201; break; // Upper left corner
            case 'm': utf32 = 200; break; // Lower left corner
            case 'n': utf32 = 206; break; // Crossing lines.
            case 'q': utf32 = 205; break; // Horizontal line.
            case 't': utf32 = 204; break; // Left 'T'
            case 'u': utf32 = 185; break; // Right 'T'
            case 'v': utf32 = 202; break; // Bottom 'T'
            case 'w': utf32 = 203; break; // Top 'T'
            case 'x': utf32 = 186; break; // Vertical bar
            default:
                ;
//              WARNING("VT100: Unrecognised line character: " << c);
          }
          m_pWindows[m_CurrentWindow]->writeChar(utf32);
        }
    }
  }

}

void Xterm::putCharFb(uint32_t c, int x, int y, uint32_t f, uint32_t b)
{
  int depth = m_Mode.pf.nBpp;
  if (depth != 8 && depth != 16 && depth != 32)
  {
//    ERROR("VT100: Pixel format is neither 8, 16 or 32 bits per pixel!");
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

uint32_t Xterm::compileColour(uint8_t r, uint8_t g, uint8_t b)
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

Xterm::Window::Window(uint32_t nWidth, uint32_t nHeight, Xterm *pParent) :
  m_CursorX(0), m_CursorY((NUM_PAGES_SCROLLBACK-1)*nHeight), m_nWidth(nWidth), m_nHeight(nHeight),
  m_nScrollMin(0), m_nScrollMax(NUM_PAGES_SCROLLBACK*nHeight-1),
  m_Foreground(C_WHITE), m_Background(C_BLACK), m_bBold(false),
  m_bLineDrawingMode(false), m_pParent(pParent), m_View((NUM_PAGES_SCROLLBACK-1)*nHeight),
  m_pData(0)
{
  m_pData = new uint16_t[NUM_PAGES_SCROLLBACK*nHeight*nWidth];
  for (size_t i = 0; i < NUM_PAGES_SCROLLBACK*nHeight*nWidth; i++)
  {
    m_pData[i] = static_cast<uint16_t>(' ') | (C_WHITE<<12) | (C_BLACK<<8);
  }
}

Xterm::Window::~Window()
{
  delete [] m_pData;
}

void Xterm::Window::setLineDrawingMode(bool b)
{
  m_bLineDrawingMode = b;
}

bool Xterm::Window::getLineDrawingMode()
{
  return m_bLineDrawingMode;
}

void Xterm::Window::writeChar(uint32_t utf32)
{
    uint8_t c = utf32&0x7F;
    m_pData[m_CursorX + m_CursorY*m_nWidth] = (c&0x7F) | (m_Foreground<<12) | (m_Background<<8);
  m_pParent->putCharFb(utf32, m_CursorX, m_CursorY-m_View, m_pParent->m_pColours[m_Foreground], m_pParent->m_pColours[m_Background]);
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

void Xterm::Window::setCursorX(uint32_t x)
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

void Xterm::Window::setCursorY(uint32_t y)
{
  // Re-render whatever was at the cursor position (without the inverted colours).
  uint16_t data = m_pData[m_CursorX + m_CursorY*m_nWidth];
  m_pParent->putCharFb(data&0xFF, m_CursorX, m_CursorY-m_View, m_pParent->m_pColours[(data>>12)&0xF], m_pParent->m_pColours[(data>>8)&0xF]);

  // Now adjust the cursor and render.
  m_CursorY = y+m_View;


  // Have we gone off the screen?
  if (m_CursorY > m_nScrollMax)
  {
    size_t nLines = m_CursorY - m_nScrollMax;
    m_CursorY -= nLines;

    // If the scroll region is set (m_nScrollMin != 0), only scroll that region.
    memmove (reinterpret_cast<uint8_t*>(&m_pData[m_nScrollMin*m_nWidth]),
             reinterpret_cast<uint8_t*>(&m_pData[(m_nScrollMin+nLines)*m_nWidth]),
             nLines * ((m_nScrollMax-m_nScrollMin)*m_nWidth*2));

    // Zero out the last rows.
    size_t base = m_nScrollMax - nLines + 1;
    for(size_t line = 0; line < nLines; line++)
    {
      for (size_t i = 0; i < m_nWidth; i++)
      {
        m_pData[i+(base + line)*m_nWidth] = static_cast<uint16_t>(' ') | (m_Foreground<<12) | (m_Background<<8);
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

void Xterm::Window::setScrollRegion(uint32_t start, uint32_t end)
{
  if (start == ~0UL && end == ~0UL)
  {
    m_nScrollMin = 0;
    m_nScrollMax = m_View+m_nHeight-1;
  }
  else
  {
    if((start+m_View) > (end+m_View))
    {
//      ERROR("m_nScrollMin > m_nScrollMax [" << start << ", " << end << ", " << m_View << "]");
      return;
    }
    m_nScrollMin = start+m_View;
    m_nScrollMax = end+m_View;
  }
}

void Xterm::Window::eraseEOL()
{
  size_t row = m_CursorY;
  for (size_t col = m_CursorX; col < m_nWidth; col++)
  {
    m_pData[col+row*m_nWidth] = static_cast<uint16_t>(' ') | (m_Foreground<<12) | (m_Background<<8);
    if (col == m_CursorX)
      m_pParent->putCharFb(' ', col, row-m_View, m_pParent->m_pColours[m_Background], m_pParent->m_pColours[m_Foreground]);
    else
      m_pParent->putCharFb(' ', col, row-m_View, m_pParent->m_pColours[m_Foreground], m_pParent->m_pColours[m_Background]);
  }
}

void Xterm::Window::eraseSOL()
{
  size_t row = m_CursorY;
  for (size_t col = 0; col <= m_CursorX; col++)
  {
    m_pData[col+row*m_nWidth] = static_cast<uint16_t>(' ') | (m_Foreground<<12) | (m_Background<<8);
    if (col == m_CursorX)
      m_pParent->putCharFb(' ', col, row-m_View, m_pParent->m_pColours[m_Background], m_pParent->m_pColours[m_Foreground]);
    else
      m_pParent->putCharFb(' ', col, row-m_View, m_pParent->m_pColours[m_Foreground], m_pParent->m_pColours[m_Background]);
  }
}

void Xterm::Window::eraseLine()
{
  size_t row = m_CursorY;
  for (size_t col = 0; col < m_nWidth; col++)
  {
    m_pData[col+row*m_nWidth] = static_cast<uint16_t>(' ') | (m_Foreground<<12) | (m_Background<<8);
    if (col == m_CursorX)
      m_pParent->putCharFb(' ', col, row-m_View, m_pParent->m_pColours[m_Background], m_pParent->m_pColours[m_Foreground]);
    else
      m_pParent->putCharFb(' ', col, row-m_View, m_pParent->m_pColours[m_Foreground], m_pParent->m_pColours[m_Background]);
  }
}

void Xterm::Window::eraseChars(size_t n)
{
  size_t row = m_CursorY;
  for (size_t col = m_CursorX; col < m_CursorX+n+1; col++)
  {
    m_pData[col+row*m_nWidth] = static_cast<uint16_t>(' ') | (m_Foreground<<12) | (m_Background<<8);
    if (col == m_CursorX)
      m_pParent->putCharFb(' ', col, row-m_View, m_pParent->m_pColours[m_Background], m_pParent->m_pColours[m_Foreground]);
    else
      m_pParent->putCharFb(' ', col, row-m_View, m_pParent->m_pColours[m_Foreground], m_pParent->m_pColours[m_Background]);
  }
}


void Xterm::Window::eraseDown()
{
  for (size_t row = m_CursorY; row < m_nHeight+m_View; row++)
  {
    for (size_t col = 0; col < m_nWidth; col ++)
    {
      m_pData[col+row*m_nWidth] = static_cast<uint16_t>(' ') | (m_Foreground<<12) | (m_Background<<8);
      if (col == m_CursorX && row == m_CursorY)
        m_pParent->putCharFb(' ', col, row-m_View, m_pParent->m_pColours[m_Background], m_pParent->m_pColours[m_Foreground]);
      else
        m_pParent->putCharFb(' ', col, row-m_View, m_pParent->m_pColours[m_Foreground], m_pParent->m_pColours[m_Background]);
    }
  }
}

void Xterm::Window::eraseUp()
{
  for (size_t row = m_View; row <= m_CursorY; row++)
  {
    for (size_t col = 0; col < m_nWidth; col ++)
    {
      m_pData[col+row*m_nWidth] = static_cast<uint16_t>(' ') | (m_Foreground<<12) | (m_Background<<8);
      if (col == m_CursorX && row == m_CursorY)
        m_pParent->putCharFb(' ', col, row-m_View, m_pParent->m_pColours[m_Background], m_pParent->m_pColours[m_Foreground]);
      else
        m_pParent->putCharFb(' ', col, row-m_View, m_pParent->m_pColours[m_Foreground], m_pParent->m_pColours[m_Background]);
    }
  }
}

void Xterm::Window::eraseScreen()
{
  for (size_t row = m_View; row < m_nHeight+m_View; row++)
  {
    for (size_t col = 0; col < m_nWidth; col ++)
    {
      m_pData[col+row*m_nWidth] = static_cast<uint16_t>(' ') | (m_Foreground<<12) | (m_Background<<8);
      if (col == m_CursorX && row == m_CursorY)
        m_pParent->putCharFb(' ', col, row-m_View, m_pParent->m_pColours[m_Background], m_pParent->m_pColours[m_Foreground]);
      else
        m_pParent->putCharFb(' ', col, row-m_View, m_pParent->m_pColours[m_Foreground], m_pParent->m_pColours[m_Background]);
    }
  }
}

void Xterm::Window::refresh()
{
  for (size_t r = m_View; r < m_View+m_nHeight; r++)
  {
    for (size_t c = 0; c < m_nWidth; c++)
    {
      uint16_t data = m_pData[c+r*m_nWidth];
      if (c == m_CursorX && r == m_CursorY)
        m_pParent->putCharFb(data&0xFF, c, r-m_View, m_pParent->m_pColours[(data>>8)&0xF], m_pParent->m_pColours[(data>>12)&0xF]);
      else
        m_pParent->putCharFb(data&0xFF, c, r-m_View, m_pParent->m_pColours[(data>>12)&0xF], m_pParent->m_pColours[(data>>8)&0xF]);
    }
  }
}

void Xterm::Window::scrollDown()
{
  // If the scroll region is set (m_nScrollMin != 0), only scroll that region.
  memmove (reinterpret_cast<uint8_t*>(&m_pData[m_nScrollMin*m_nWidth]),
            reinterpret_cast<uint8_t*>(&m_pData[(m_nScrollMin+1)*m_nWidth]),
            (m_nScrollMax-m_nScrollMin)*m_nWidth*2);
  // Zero out the last row.
  for (size_t i = 0; i < m_nWidth; i++)
  {
    m_pData[i+m_nScrollMax*m_nWidth] = static_cast<uint16_t>(' ') | (m_Foreground<<12) | (m_Background<<8);
  }

  refresh();
}

void Xterm::Window::scrollUp()
{

  // If the scroll region is set (m_nScrollMin != 0), only scroll that region.
  memmove (reinterpret_cast<uint8_t*>(&m_pData[(m_nScrollMin+1)*m_nWidth]),
            reinterpret_cast<uint8_t*>(&m_pData[m_nScrollMin*m_nWidth]),
            (m_nScrollMax-m_nScrollMin)*m_nWidth*2);

  // Zero out the first row.
  for (size_t i = 0; i < m_nWidth; i++)
  {
    m_pData[i+m_nScrollMin*m_nWidth] = static_cast<uint16_t>(' ') | (m_Foreground<<12) | (m_Background<<8);
  }

  refresh();
}

void Xterm::Window::setFore(uint8_t c)
{
  m_Foreground = (m_bBold) ? c+C_BRIGHT : c;
}

void Xterm::Window::setBack(uint8_t c)
{
  // No bold for backgrounds!
  m_Background = c;
}

void Xterm::Window::setBold(bool b)
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

void Xterm::Window::deleteCharacters(uint32_t n)
{
  // current x position, so we can find out how many positions will need to be cleared
  uint32_t x = m_CursorX;
  uint32_t endX = x + n;

  // clears by shifting the following data left into this area
  memmove(reinterpret_cast<uint8_t*>(&m_pData[x+m_CursorY*m_nWidth]),
          reinterpret_cast<uint8_t*>(&m_pData[endX+m_CursorY*m_nWidth]),
          2 * (m_nWidth - endX));

  // Zero out the remainder of the line
  for (size_t i = (m_nWidth - n); i < m_nWidth; i++)
    m_pData[i+m_CursorY*m_nWidth] = static_cast<uint16_t>(' ') | (m_Foreground<<12) | (m_Background<<8);


  refresh();
}
