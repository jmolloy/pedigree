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

#include "Vga.h"
#include <machine/openfirmware/OpenFirmware.h>
#include <machine/openfirmware/Device.h>
#include <utilities/utility.h>
#include "Font.c"

PPCVga::PPCVga() :
  m_pFramebuffer(reinterpret_cast<uint8_t*> (0xb0000000)), m_Width(0), m_Height(0), m_Depth(0), m_Stride(0)
{
}

PPCVga::~PPCVga()
{
}

void PPCVga::initialise()
{
  // Firstly get the screen device.
  // TODO: deal more cleanly with failure.
  OFDevice screen(OpenFirmware::instance().findDevice("screen"));
  // Get the 'chosen' device.
  OFDevice chosen(OpenFirmware::instance().findDevice("/chosen"));
  
  // Now we can get the MMU device from chosen.
  OFDevice mmu( chosen.getProperty("mmu") );
  
  // Get the (physical) framebuffer address.
  OFParam physFbAddr = screen.getProperty("address");
  
  // Create a mapping with OpenFirmware.
  /// \todo Something with VirtualAddressSpace here?
  mmu.executeMethod("map", 4,
                    reinterpret_cast<OFParam>(0x6a),
                    reinterpret_cast<OFParam>(0x01000000),
                    static_cast<OFParam>(m_pFramebuffer),
                    physFbAddr); // 0x6a = Uncached.

  // Find the device width, height and depth.
  m_Width = reinterpret_cast<uint32_t> ( screen.getProperty("width") );
  m_Height = reinterpret_cast<uint32_t> ( screen.getProperty("height") );
  m_Depth = reinterpret_cast<uint32_t> ( screen.getProperty("depth") );
  m_Stride = m_Width;
  
  // Clear the text framebuffer.
  uint16_t clear = ' ';
  for (int i = 0; i < (m_Width/FONT_WIDTH)*(m_Height/FONT_HEIGHT); i++)
  {
    m_pTextBuffer[i] = clear;
  }
  // Clear the graphics framebuffer.
  uint16_t *p16Fb = reinterpret_cast<uint16_t*> (m_pFramebuffer);
  uint32_t *p32Fb = reinterpret_cast<uint32_t*> (m_pFramebuffer);
  for (int i = 0; i < m_Stride*m_Height; i++)
  {
    switch (m_Depth)
    {
      case 8:
        m_pFramebuffer[i] = 0x00;
        break;
      case 16:
        p16Fb[i] = 0x0000;
        break;
      case 32:
        p32Fb[i] = 0x00000000;
        break;
    }
  }

  m_pTextBuffer[0] = 'a';
  m_pTextBuffer[1] = 'b';
  pokeBuffer(0, 0);
}

void PPCVga::pokeBuffer (uint8_t *pBuffer, size_t nBufLen)
{
  if (pBuffer)
  {
    memcpy(m_pTextBuffer, pBuffer, nBufLen);
  }

  // Refresh screen.
  for(int i = 0; i < (m_Width/FONT_WIDTH); i++)
  {
    for (int j = 0; j < (m_Height/FONT_HEIGHT); j++)
    {
      uint16_t ch = m_pTextBuffer[j*(m_Width/FONT_WIDTH)+i];
      putChar(ch&0xFF, i*FONT_WIDTH, j*FONT_HEIGHT, 0xff, 0x00);
    }
  }
}

void PPCVga::peekBuffer (uint8_t *pBuffer, size_t nBufLen)
{
  memcpy(pBuffer, m_pTextBuffer, nBufLen);
}

void PPCVga::putChar(char c, int x, int y, unsigned int f, unsigned int b)
{
  int idx = ((int)c) * FONT_HEIGHT;
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

      if (m_Depth == 8)
        m_pFramebuffer[y*m_Width + i*m_Width + x + j] = col&0xFF;
      else if (m_Depth == 16)
        p16Fb[y*m_Width + i*m_Width + x + j] = col&0xFFFF;
      else
        p32Fb[y*m_Width + i*m_Width + x + j] = col;
    }
  }
}