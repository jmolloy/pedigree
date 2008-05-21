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

PPCVga::PPCVga() :
  m_pFramebuffer(0xb0000000), m_Width(0), m_Height(0), m_Depth(0), m_Stride(0)
{
  // Firstly get the screen device.
  // TODO: deal more cleanly with failure.
  OFDevice screen(OpenFirmware::instance().findDevice("screen"));
  // Get the 'chosen' device.
  OFDevice chosen(OpenFirmware::instance().findDevice("chosen"));
  
  // Now we can get the MMU device from chosen.
  OFDevice mmu( chosen.getProperty("mmu") );
  
  // Get the (physical) framebuffer address.
  OFParam physFbAddr = screen.getProperty("address");
  
  // Create a mapping with OpenFirmware.
  /// \todo Something with VirtualAddressSpace here?
  mmu.executeMethod("map", 3,
                    static_cast<OFParam>(m_pFramebuffer),
                    static_cast<OFParam>(0x01000000),
                    static_cast<OFParam>(-1) ); /// \warning the -1 is wrong! we need it uncached!
  
  // Find the device width, height and depth.
  m_Width = static_cast<uint32_t> ( screen.getProperty("width") );
  m_Height = static_cast<uint32_t> ( screen.getProperty("height") );
  m_Depth = static_cast<uint32_t> ( screen.getProperty("depth") );
  m_Stride = m_Width;
  
  // Clear the text framebuffer.
  uint16_t clear = ' ';
  for (int i = 0; i < (m_Width/FONT_WIDTH)*(m_Height/FONT_HEIGHT); i++)
  {
    m_pTextBuffer[i] = clear;
  }
  // Clear the graphics framebuffer.
  uint16_t *p16Fb = static_cast<uint16_t*> (m_pFramebuffer);
  uint32_t *p32Fb = static_cast<uint32_t*> (m_pFramebuffer);
  for (int i = 0; i < m_Stride*m_Height; i++)
  {
    switch (m_Depth)
    {
      case 8:
        m_pFramebuffer[i] = 0x00;
        break;
      case 16:
        m_p16Fb[i] = 0x0000;
        break;
      case 32:
        m_p32Fb[i] = 0x00000000;
        break;
    }
  }
}

PPCVga::~PPCVga()
{
}

void PPCVga::pokeBuffer (uint8_t *pBuffer, size_t nBufLen)
{
  
}

void PPCVga::peekBuffer (uint8_t *pBuffer, size_t nBufLen)
{
  
}
