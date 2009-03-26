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
#include <processor/VirtualAddressSpace.h>
#include <processor/PhysicalMemoryManager.h>
#include "Font.c"

PPCVga::PPCVga() :
  m_Framebuffer("VGA framebuffer"), m_pFramebuffer(0),
  m_Width(0), m_Height(0), m_Depth(0), m_Stride(0)
{
}

PPCVga::~PPCVga()
{
}

void PPCVga::initialise()
{
  // Firstly get the screen device.
  OFDevice screen(OpenFirmware::instance().findDevice("screen"));

  // Get the 'chosen' device.
  OFDevice chosen(OpenFirmware::instance().findDevice("/chosen"));

  // Get the (physical) framebuffer address.
  OFParam physFbAddr = screen.getProperty("address");

  PhysicalMemoryManager &physicalMemoryManager = PhysicalMemoryManager::instance();
  if (!physicalMemoryManager.allocateRegion(m_Framebuffer,
					    0x01000000/0x1000,
					    PhysicalMemoryManager::continuous | PhysicalMemoryManager::nonRamMemory | PhysicalMemoryManager::force,
					    VirtualAddressSpace::KernelMode | VirtualAddressSpace::Write | VirtualAddressSpace::WriteThrough,
					    reinterpret_cast<uintptr_t>(physFbAddr)))
  {
    // Panic! but we have no way of indicating failure to the user! argh!!
    for(;;);
    return;
  }

  m_pFramebuffer = reinterpret_cast<uint8_t*> (m_Framebuffer.virtualAddress());

  // Create a mapping with OpenFirmware.
  /// \todo Something with VirtualAddressSpace here?
  //  mmu.executeMethod("map", 4,
  //                  reinterpret_cast<OFParam>(0x6a),
  //    /                reinterpret_cast<OFParam>(0x01000000),
  //	       //               static_cast<OFParam>(m_pFramebuffer),
    //	       physF//bAddr); // 0x6a = Uncached.

  // Find the device width, height and depth.
  m_Width = reinterpret_cast<uint32_t> ( screen.getProperty("width") );
  m_Height = reinterpret_cast<uint32_t> ( screen.getProperty("height") );
  m_Depth = reinterpret_cast<uint32_t> ( screen.getProperty("depth") );
  m_Stride = reinterpret_cast<uint32_t> ( screen.getProperty("linebytes") );

  if (m_Depth == 8)
  {
    for (int i = 0; i < 16; i++)
      m_pColours[i] = i;
  }
  else if (m_Depth == 16)
  {
    m_pColours[0] = RGB_16 (0x00, 0x00, 0x00); // Black
    m_pColours[1] = RGB_16 (0x00, 0x00, 0x99); // Blue
    m_pColours[2] = RGB_16 (0x00, 0x99, 0x00); // Green
    m_pColours[3] = RGB_16 (0x00, 0x99, 0x99); // Cyan
    m_pColours[4] = RGB_16 (0x99, 0x00, 0x00); // Red
    m_pColours[5] = RGB_16 (0x99, 0x00, 0x99); // Magenta
    m_pColours[6] = RGB_16 (0x99, 0x66, 0x00); // Brown
    m_pColours[7] = RGB_16 (0xBB, 0xBB, 0xBB); // Light grey
    m_pColours[8] = RGB_16 (0x99, 0x99, 0x99); // Dark grey
    m_pColours[9] = RGB_16 (0x00, 0x00, 0xFF); // Light blue
    m_pColours[10] =RGB_16 (0x00, 0xFF, 0x00); // Light green
    m_pColours[11] =RGB_16 (0x00, 0xFF, 0xFF); // Light cyan
    m_pColours[12] =RGB_16 (0xFF, 0x00, 0x00); // Light red
    m_pColours[13] =RGB_16 (0xFF, 0x00, 0xFF); // Light magenta
    m_pColours[14] =RGB_16 (0xFF, 0xFF, 0x00); // Light brown / yellow
    m_pColours[15] =RGB_16 (0xFF, 0xFF, 0xFF); // White
    m_Stride /= 2; // 16bpp = 2 bytes per pixel.
  }
  
  // Clear the text framebuffer.
  uint16_t clear = ' ';
  for (unsigned int i = 0; i < (m_Width/FONT_WIDTH)*(m_Height/FONT_HEIGHT); i++)
  {
    m_pTextBuffer[i] = clear;
  }

  // Clear the graphics framebuffer.
  uint16_t *p16Fb = reinterpret_cast<uint16_t*> (m_pFramebuffer);
  uint32_t *p32Fb = reinterpret_cast<uint32_t*> (m_pFramebuffer);
  for (unsigned int i = 0; i < m_Stride*m_Height; i++)
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
}

void PPCVga::pokeBuffer (uint8_t *pBuffer, size_t nBufLen)
{
  uint16_t *pBuffer16 = reinterpret_cast<uint16_t*> (pBuffer);
  // Refresh screen.
  for(unsigned int i = 0; i < (m_Width/FONT_WIDTH); i++)
  {
    for (unsigned int j = 0; j < (m_Height/FONT_HEIGHT); j++)
    {
      int idx = j*(m_Width/FONT_WIDTH)+i;
      if ((pBuffer16 == m_pTextBuffer) || (pBuffer16[idx] != m_pTextBuffer[idx]))
      {
        m_pTextBuffer[idx] = pBuffer16[idx];
        uint16_t ch = m_pTextBuffer[j*(m_Width/FONT_WIDTH)+i];
        unsigned int fg, bg;
        fg = m_pColours[(ch>>8)&0xF];
        bg = m_pColours[(ch>>12)&0xF];
        putChar(ch&0xFF, i*FONT_WIDTH, j*FONT_HEIGHT, fg, bg);
      }
    }
  }
}

void PPCVga::peekBuffer (uint8_t *pBuffer, size_t nBufLen)
{
  memcpy(pBuffer, m_pTextBuffer, nBufLen);
}

void PPCVga::putChar(char c, int x, int y, unsigned int f, unsigned int b)
{
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

      if (m_Depth == 8)
        m_pFramebuffer[y*m_Stride + i*m_Stride + x + j] = col&0xFF;
      else if (m_Depth == 16)
        p16Fb[y*m_Stride + i*m_Stride + x + j] = col&0xFFFF;
      else
        p32Fb[y*m_Stride + i*m_Stride + x + j] = col;
    }
  }
}
