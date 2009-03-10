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

#include "VbeDisplay.h"
#include <Log.h>
#include <machine/x86_common/Bios.h>
#include <machine/Vga.h>
#include <machine/Machine.h>
#include <processor/PhysicalMemoryManager.h>
#include <TUI/TUI.h>

extern TUI *g_pTui;

VbeDisplay::VbeDisplay()
{
}

VbeDisplay::VbeDisplay(Device *p, VbeVersion version, List<Display::ScreenMode*> &sms, uintptr_t fbAddr) :
  Display(p), m_VbeVersion(version), m_ModeList(sms)
{
  Display::ScreenMode *pSm = 0;
  // Try to find mode 0x117 (1024x768x16)
  for (List<Display::ScreenMode*>::Iterator it = m_ModeList.begin();
       it != m_ModeList.end();
       it++)
  {
    if ((*it)->id == 0x117)
    {
      pSm = *it;
      break;
    }
  }
  if (pSm == 0)
  {
    FATAL("Screenmode not found");
  }

  for (Vector<Device::Address*>::Iterator it = m_Addresses.begin();
       it != m_Addresses.end();
       it++)
  {
    if ((*it)->m_Address == fbAddr)
    {
      m_pFramebuffer = static_cast<MemoryMappedIo*> ((*it)->m_Io);
      break;
    }
  }

  setScreenMode(*pSm);
}

VbeDisplay::~VbeDisplay()
{
}

void *VbeDisplay::getFramebuffer()
{
  return reinterpret_cast<void*> (m_pFramebuffer->virtualAddress());
}

bool VbeDisplay::getPixelFormat(Display::PixelFormat *pPf)
{
  memcpy(pPf, &m_Mode.pf, sizeof(Display::PixelFormat));
  return true;
}

bool VbeDisplay::getCurrentScreenMode(Display::ScreenMode &sm)
{
  sm = m_Mode;
  return true;
}

bool VbeDisplay::getScreenModes(List<Display::ScreenMode*> &sms)
{
  sms = m_ModeList;
  return true;
}

bool VbeDisplay::setScreenMode(Display::ScreenMode sm)
{
  m_Mode = sm;

  // SET SuperVGA VIDEO MODE - AX=4F02h, BX=new mode
  Bios::instance().setAx (0x4F02);
  Bios::instance().setBx (m_Mode.id | (1<<14));
  Bios::instance().setEs (0x0000);
  Bios::instance().setDi (0x0000);
  Bios::instance().executeInterrupt (0x10);

  // Check the signature.
  if (Bios::instance().getAx() != 0x004F)
  {
    ERROR("VBE: Set mode failed! (mode " << Hex << m_Mode.id << ")");
    return false;
  }
  NOTICE("VBE: Set mode " << m_Mode.id);

  // Tell the Machine instance what VBE mode we're in, so it can set it again if we enter the debugger and return.
  Machine::instance().getVga(0)->setMode(m_Mode.id);

  // Inform the TUI that the current mode has changed.
  g_pTui->modeChanged(m_Mode, getFramebuffer());

  return true;
}
