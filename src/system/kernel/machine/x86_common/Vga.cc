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
#include "Vga.h"
#include <utilities/utility.h>

/// \warning 80x50 and 90x60 modes don't work, because we don't have an 8x8 font to install.
uint8_t g_80x25_text[] =
{
  /* MISC */
  0x67,
  /* SEQ */
  0x03, 0x00, 0x03, 0x00, 0x02,
  /* CRTC */
  0x5F, 0x4F, 0x50, 0x82, 0x55, 0x81, 0xBF, 0x1F,
  0x00, 0x4F, 0x0D, 0x0E, 0x00, 0x00, 0x00, 0x50,
  0x9C, 0x0E, 0x8F, 0x28, 0x1F, 0x96, 0xB9, 0xA3,
  0xFF,
  /* GC */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x0E, 0x00,
  0xFF,
  /* AC */
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x14, 0x07,
  0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
  0x0C, 0x00, 0x0F, 0x08, 0x00
};

uint8_t g_80x50_text[] =
{
  /* MISC */
  0x67,
  /* SEQ */
  0x03, 0x00, 0x03, 0x00, 0x02,
  /* CRTC */
  0x5F, 0x4F, 0x50, 0x82, 0x55, 0x81, 0xBF, 0x1F,
  0x00, 0x47, 0x06, 0x07, 0x00, 0x00, 0x01, 0x40,
  0x9C, 0x8E, 0x8F, 0x28, 0x1F, 0x96, 0xB9, 0xA3,
  0xFF, 
  /* GC */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x0E, 0x00,
  0xFF, 
  /* AC */
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x14, 0x07,
  0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
  0x0C, 0x00, 0x0F, 0x08, 0x00,
};

uint8_t g_90x30_text[] =
{
  /* MISC */
  0xE7,
  /* SEQ */
  0x03, 0x01, 0x03, 0x00, 0x02,
  /* CRTC */
  0x6B, 0x59, 0x5A, 0x82, 0x60, 0x8D, 0x0B, 0x3E,
  0x00, 0x4F, 0x0D, 0x0E, 0x00, 0x00, 0x00, 0x00,
  0xEA, 0x0C, 0xDF, 0x2D, 0x10, 0xE8, 0x05, 0xA3,
  0xFF,
  /* GC */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x0E, 0x00,
  0xFF,
  /* AC */
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x14, 0x07,
  0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
  0x0C, 0x00, 0x0F, 0x08, 0x00,
};

uint8_t g_90x60_text[] =
{
  /* MISC */
  0xE7,
  /* SEQ */
  0x03, 0x01, 0x03, 0x00, 0x02,
  /* CRTC */
  0x6B, 0x59, 0x5A, 0x82, 0x60, 0x8D, 0x0B, 0x3E,
  0x00, 0x47, 0x06, 0x07, 0x00, 0x00, 0x00, 0x00,
  0xEA, 0x0C, 0xDF, 0x2D, 0x08, 0xE8, 0x05, 0xA3,
  0xFF,
  /* GC */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x0E, 0x00,
  0xFF,
  /* AC */
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x14, 0x07,
  0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
  0x0C, 0x00, 0x0F, 0x08, 0x00,
};

unsigned char *g_pModeDescriptions[] = {g_80x25_text,
                                        g_80x50_text,
                                        g_90x30_text,
                                        g_90x60_text,
                                        0};
unsigned int g_pModeWidths[]  = {80,80,90,90,0};
unsigned int g_pModeHeights[] = {25,50,30,60,0};
#define NUM_MODES 4

X86Vga::X86Vga(uint32_t nRegisterBase, uint32_t nFramebufferBase) :
  m_RegisterPort(),
  m_pFramebuffer( reinterpret_cast<uint8_t*> (nFramebufferBase) ),
  m_nWidth(80),
  m_nHeight(25),
  m_nMode(0)
{
  m_RegisterPort.allocate(nRegisterBase, 0x1B, "X86Vga");
}

X86Vga::~X86Vga()
{
}

bool X86Vga::setMode (size_t nCols, size_t nRows, bool bIsText, size_t nBpp)
{
  int nMode = -1;
  if (nCols == 80 &&
      nRows == 25 &&
      bIsText == true)
    nMode = 0;
  else if (nCols == 80 &&
           nRows == 50 &&
           bIsText == true)
    nMode = 1;
  else if (nCols == 90 &&
           nRows == 30 &&
           bIsText == true)
    nMode = 2;
  else if (nCols == 90 &&
           nRows == 60 &&
           bIsText == true)
    nMode = 3;
  if (nMode > -1)
  {
    setMode (nMode);
    m_nMode = nMode;
    return true;
  }
  else
    return false;
}

bool X86Vga::setLargestTextMode ()
{
  setMode(2);
  m_nMode = 2;
  return true;
}

bool X86Vga::isMode (size_t nCols, size_t nRows, bool bIsText, size_t nBpp)
{
  if (m_nMode == 0 &&
      nCols == 80 &&
      nRows == 25 &&
      bIsText == true)
    return true;
  else if (m_nMode == 1 &&
           nCols == 80 &&
           nRows == 50 &&
           bIsText == true)
    return true;
  else if (m_nMode == 2 &&
           nCols == 90 &&
           nRows == 30 &&
           bIsText == true)
    return true;
  else if (m_nMode == 3 &&
           nCols == 90 &&
           nRows == 60 &&
           bIsText == true)
    return true;
  return false;
}

bool X86Vga::isLargestTextMode ()
{
  return m_nMode == 2;
}

void X86Vga::rememberMode()
{
  int mode = getMode();
  if (mode > -1)
  {
    memcpy (m_pStoredMode, g_pModeDescriptions[mode], 61);
  }
  else
  {
    memcpy (m_pStoredMode, g_pModeDescriptions[0], 61);
  }
}
  
void X86Vga::restoreMode()
{
  g_pModeDescriptions[4] = m_pStoredMode;
  setMode(4);
  m_nMode = -1;
}

void X86Vga::pokeBuffer (uint8_t *pBuffer, size_t nBufLen)
{
  memcpy (m_pFramebuffer, pBuffer, nBufLen);
}

void X86Vga::peekBuffer (uint8_t *pBuffer, size_t nBufLen)
{
  memcpy (pBuffer, m_pFramebuffer, nBufLen);
}

void X86Vga::moveCursor (size_t nX, size_t nY)
{
  uint16_t tmp = nY*m_nWidth + nX;
  
  m_RegisterPort.write8(14, VGA_CRTC_INDEX);
  m_RegisterPort.write8(tmp>>8, VGA_CRTC_DATA);
  m_RegisterPort.write8(15, VGA_CRTC_INDEX);
  m_RegisterPort.write8(tmp, VGA_CRTC_DATA);
}

void X86Vga::setMode(int nMode)
{
  unsigned int i;
  unsigned char *pMode = g_pModeDescriptions[nMode];
  
  m_RegisterPort.allocate(VGA_BASE, 0x1B, "VGA controller");

  /* write MISCELLANEOUS reg */
  m_RegisterPort.write8(*pMode, VGA_MISC_WRITE);
  pMode++;
  /* write SEQUENCER regs */
  for(i = 0; i < VGA_NUM_SEQ_REGS; i++)
  {
    m_RegisterPort.write8(i, VGA_SEQ_INDEX);
    m_RegisterPort.write8(*pMode, VGA_SEQ_DATA);
    pMode++;
  }
  /* unlock CRTC registers */
  m_RegisterPort.write8(0x03, VGA_CRTC_INDEX);
  m_RegisterPort.write8( m_RegisterPort.read8(VGA_CRTC_DATA) | 0x80, VGA_CRTC_DATA);
  m_RegisterPort.write8(0x11, VGA_CRTC_INDEX);
  m_RegisterPort.write8( m_RegisterPort.read8(VGA_CRTC_DATA) & ~0x80, VGA_CRTC_DATA);
  /* make sure they remain unlocked */
  pMode[0x03] |= 0x80;
  pMode[0x11] &= ~0x80;
  /* write CRTC pMode */
  for(i = 0; i < VGA_NUM_CRTC_REGS; i++)
  {
    m_RegisterPort.write8(i, VGA_CRTC_INDEX);
    m_RegisterPort.write8(*pMode, VGA_CRTC_DATA);
    pMode++;
  }
  /* write GRAPHICS CONTROLLER regs */
  for(i = 0; i < VGA_NUM_GC_REGS; i++)
  {
    m_RegisterPort.write8(i, VGA_GC_INDEX);
    m_RegisterPort.write8(*pMode, VGA_GC_DATA);
    pMode++;
  }
  /* write ATTRIBUTE CONTROLLER regs */
  for(i = 0; i < VGA_NUM_AC_REGS; i++)
  {
    (void)m_RegisterPort.read8(VGA_INSTAT_READ);
    m_RegisterPort.write8(i, VGA_AC_INDEX);
    m_RegisterPort.write8(*pMode, VGA_AC_WRITE);
    pMode++;
  }
  /* lock 16-color palette and unblank display */
  (void)m_RegisterPort.read8(VGA_INSTAT_READ);
  m_RegisterPort.write8(0x20, VGA_AC_INDEX);
  
  m_nWidth = g_pModeWidths[nMode];
  m_nHeight = g_pModeHeights[nMode];
}

int X86Vga::getMode()
{
  unsigned char aMode[61];
  unsigned char *pMode = &aMode[0];
  unsigned int i;

  /* read MISCELLANEOUS reg */
  *pMode = m_RegisterPort.read8(VGA_MISC_READ);
  pMode++;
  /* read SEQUENCER regs */
  for(i = 0; i < VGA_NUM_SEQ_REGS; i++)
  {
    m_RegisterPort.write8(i, VGA_SEQ_INDEX);
    *pMode = m_RegisterPort.read8(VGA_SEQ_DATA);
    pMode++;
  }
  /* read CRTC regs */
  for(i = 0; i < VGA_NUM_CRTC_REGS; i++)
  {
    m_RegisterPort.write8(i, VGA_CRTC_INDEX);
    *pMode = m_RegisterPort.read8(VGA_CRTC_DATA);
    pMode++;
  }
  /* read GRAPHICS CONTROLLER regs */
  for(i = 0; i < VGA_NUM_GC_REGS; i++)
  {
    m_RegisterPort.write8(i, VGA_GC_INDEX);
    *pMode = m_RegisterPort.read8(VGA_GC_DATA);
    pMode++;
  }
  /* read ATTRIBUTE CONTROLLER regs */
  for(i = 0; i < VGA_NUM_AC_REGS; i++)
  {
    (void)m_RegisterPort.read8(VGA_INSTAT_READ);
    m_RegisterPort.write8(i, VGA_AC_INDEX);
    *pMode = m_RegisterPort.read8(VGA_AC_READ);
    pMode++;
  }
  /* lock 16-color palette and unblank display */
  (void)m_RegisterPort.read8(VGA_INSTAT_READ);
  m_RegisterPort.write8(0x20, VGA_AC_INDEX);
  
  // Check our array of known modes.
  bool bFound;
  for (int i = 0; i < NUM_MODES; i++)
  {
    bFound = true;
    for (int j = 0; j < VGA_NUM_REGS; j++)
    {
      if (aMode[j] != g_pModeDescriptions[i][j])
      {
        bFound = false;
        break;
      }
    }
    if (bFound)
      return i;
  }
  // If we didn't find the mode, store it in our global remember array.
  memcpy (m_pStoredMode, aMode, 61);
  return -1;
}
