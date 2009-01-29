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
#include <utilities/utility.h>
#include <processor/VirtualAddressSpace.h>
#include <processor/PhysicalMemoryManager.h>
#include <machine/x86_common/Bios.h>

#include <Log.h>
#include <panic.h>

X86Vga::X86Vga(uint32_t nRegisterBase, uint32_t nFramebufferBase) :
  m_RegisterPort("VGA controller"),
  m_Framebuffer("VGA framebuffer"),
  m_pFramebuffer( reinterpret_cast<uint8_t*> (nFramebufferBase) ),
  m_nWidth(80),
  m_nHeight(25),
  m_nMode(3),
  m_ModeStack(0)
{
}

X86Vga::~X86Vga()
{
}

bool X86Vga::setMode (int mode)
{
  m_nMode = mode;
  return true;
}

bool X86Vga::setLargestTextMode ()
{
  return true;
}

bool X86Vga::isMode (size_t nCols, size_t nRows, bool bIsText, size_t nBpp)
{
  return false;
}

bool X86Vga::isLargestTextMode ()
{
  return true;
}

void X86Vga::rememberMode()
{
  m_ModeStack++;

  if (m_ModeStack == 1)
  {
    // SET SuperVGA VIDEO MODE - AX=4F02h, BX=new mode
    Bios::instance().setAx (0x4F02);
    Bios::instance().setBx (3);
    Bios::instance().setEs (0x0000);
    Bios::instance().setDi (0x0000);
    Bios::instance().executeInterrupt (0x10);
  }
}

void X86Vga::restoreMode()
{
  if (m_ModeStack == 0) return;
  m_ModeStack--;

  if (m_ModeStack == 0 && m_nMode != 3)
  {
    // SET SuperVGA VIDEO MODE - AX=4F02h, BX=new mode
    Bios::instance().setAx (0x4F02);
    Bios::instance().setBx (m_nMode);
    Bios::instance().setEs (0x0000);
    Bios::instance().setDi (0x0000);
    Bios::instance().executeInterrupt (0x10);
  }
}

void X86Vga::pokeBuffer (uint8_t *pBuffer, size_t nBufLen)
{
  if (!pBuffer)
    return;
  if (m_Framebuffer == true)
    memcpy(m_Framebuffer.virtualAddress(), pBuffer, nBufLen);
  else
    memcpy(m_pFramebuffer, pBuffer, nBufLen);
}

void X86Vga::peekBuffer (uint8_t *pBuffer, size_t nBufLen)
{
  if (m_Framebuffer == true)
    memcpy(pBuffer, m_Framebuffer.virtualAddress(), nBufLen);
  else
    memcpy(pBuffer, m_pFramebuffer, nBufLen);
}

void X86Vga::moveCursor (size_t nX, size_t nY)
{
  if (!m_RegisterPort)
    return;

  uint16_t tmp = nY*m_nWidth + nX;

  m_RegisterPort.write8(14, VGA_CRTC_INDEX);
  m_RegisterPort.write8(tmp>>8, VGA_CRTC_DATA);
  m_RegisterPort.write8(15, VGA_CRTC_INDEX);
  m_RegisterPort.write8(tmp, VGA_CRTC_DATA);
}

bool X86Vga::initialise()
{
  // TODO: We should allocate the value passed to the constructor
  if (m_RegisterPort.allocate(VGA_BASE, 0x1B) == false)
    return false;

  // Allocate the Video RAM
  PhysicalMemoryManager &physicalMemoryManager = PhysicalMemoryManager::instance();
  return physicalMemoryManager.allocateRegion(m_Framebuffer,
                                              2,
                                              PhysicalMemoryManager::continuous | PhysicalMemoryManager::nonRamMemory,
                                              VirtualAddressSpace::KernelMode | VirtualAddressSpace::Write | VirtualAddressSpace::WriteThrough,
                                              reinterpret_cast<uintptr_t>(m_pFramebuffer));
}
