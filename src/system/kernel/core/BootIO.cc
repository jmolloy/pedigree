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

#include "BootIO.h"
#include <processor/types.h>
#include <machine/Machine.h>
#include <machine/Vga.h>
#include <machine/Serial.h>

BootIO::BootIO() :
    m_CursorX(0), m_CursorY(0)
{
}

BootIO::~BootIO()
{
}

void BootIO::initialise()
{
  Vga *pVga = Machine::instance().getVga(0);
  if (pVga)
  {
    pVga->setLargestTextMode();
    uint16_t *pFramebuffer = *pVga;
    if (pFramebuffer != 0)
      for (size_t i = 0; i < pVga->getNumRows()*pVga->getNumCols(); i++)
        pFramebuffer[i] = 0;
  }
}

void BootIO::write(HugeStaticString &str, Colour foreColour, Colour backColour)
{
  for(size_t i = 0; i < str.length(); i++)
    putCharVga(str[i], foreColour, backColour);
  for(size_t i = 0; i < Machine::instance().getNumSerial(); i++)
  {
    startColour(Machine::instance().getSerial(i), foreColour, backColour);
    for(size_t j = 0; j < str.length(); j++)
      Machine::instance().getSerial(i)->write(str[j]);
    endColour(Machine::instance().getSerial(i));
  }
#ifdef PPC_COMMON
  // For PPC: causes the graphics framebuffer to be updated from the text one.
  Vga *pVga = Machine::instance().getVga(0);
  uint16_t *pFramebuffer = *pVga;
  pVga->pokeBuffer(reinterpret_cast<uint8_t*>(pFramebuffer),0);
#endif
}

void BootIO::putCharVga(const char c, Colour foreColour, Colour backColour)
{
  Vga *pVga = Machine::instance().getVga(0);
  if (pVga)
  {
    uint16_t *pFramebuffer = *pVga;
    if (pFramebuffer == 0)
      return;
    // Backspace?
    if (c == 0x08)
    {
      // Can we just move backwards? or do we have to go up?
      if (m_CursorX)
        m_CursorX--;
      else
      {
        m_CursorX = pVga->getNumCols()-1;
        if (m_CursorY > 0)
          m_CursorY--;
      }
      
      // Erase the contents of the cell currently.
      uint8_t attributeByte = (backColour << 4) | (foreColour & 0x0F);
      pFramebuffer[m_CursorY*pVga->getNumCols() + m_CursorX] = ' ' | (attributeByte << 8);
      
    }
  
    // Tab?
    else if (c == 0x09 && ( ((m_CursorX+8)&~(8-1)) < pVga->getNumCols()) )
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
      pFramebuffer[m_CursorY*pVga->getNumCols() + m_CursorX] = c | (attributeByte << 8);
  
      // Increment the cursor.
      m_CursorX++;
    }
  
    // Do we need to wrap?
    if (m_CursorX >= pVga->getNumCols())
    {
      m_CursorX = 0;
      m_CursorY ++;
    }
    
    // Get a space character with the default colour attributes.
    uint8_t attributeByte = (Black << 4) | (White & 0x0F);
    uint16_t blank = ' ' | (attributeByte << 8);
    if (m_CursorY >= pVga->getNumRows())
    {
      for (size_t i = 0; i < (pVga->getNumRows()-1)*pVga->getNumCols(); i++)
        pFramebuffer[i] = pFramebuffer[i+pVga->getNumCols()];

      for (size_t i = (pVga->getNumRows()-1)*pVga->getNumCols();
           i < (pVga->getNumRows())*pVga->getNumCols();
           i++)
        pFramebuffer[i] = blank;

      m_CursorY = pVga->getNumRows()-1;
    }
  }
}

void BootIO::startColour(Serial *pSerial, Colour foreColour, Colour backColour)
{
  pSerial->write("\033[");
  switch(foreColour)
  {
    case Black:        pSerial->write("30"); break;
    case Red:          pSerial->write("31"); break;
    case Green:        pSerial->write("32"); break;
    case Yellow:       pSerial->write("1;33"); break; // It's actually brown.
    case Blue:         pSerial->write("34"); break;
    case Magenta:      pSerial->write("35"); break;
    case Cyan:         pSerial->write("36"); break;
    case LightGrey:    pSerial->write("0;37"); break;
    case DarkGrey:     pSerial->write("1;30"); break;
    case LightRed:     pSerial->write("1;31"); break;
    case LightGreen:   pSerial->write("1;32"); break;
    case LightBlue:    pSerial->write("1;34"); break;
    case LightMagenta: pSerial->write("1;35"); break;
    case LightCyan:    pSerial->write("1;36"); break;
    case White:        pSerial->write("1;37"); break;
    default:                       pSerial->write('1');
  }
  pSerial->write(";");
  switch(backColour)
  {
    case Black:        pSerial->write("40"); break;
    case Red:          pSerial->write("41"); break;
    case Green:        pSerial->write("42"); break;
    case DarkGrey:     pSerial->write("43"); break; // It's actually brown.
    case Blue:         pSerial->write("44"); break;
    case Magenta:      pSerial->write("45"); break;
    case Cyan:         pSerial->write("46"); break;
    case White:        pSerial->write("47"); break;
    default:                       pSerial->write('1');
  }
  pSerial->write('m');
}

void BootIO::endColour(Serial *pSerial)
{
}
