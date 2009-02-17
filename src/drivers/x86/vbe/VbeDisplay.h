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
#ifndef VBE_DISPLAY_H
#define VBE_DISPLAY_H

#include <machine/Device.h>
#include <machine/Display.h>
#include <utilities/List.h>
#include <processor/MemoryMappedIo.h>

class VbeDisplay : public Display
{
public:
  /** VBE versions, in order. */
  enum VbeVersion
  {
    Vbe1_2,
    Vbe2_0,
    Vbe3_0
  };

  VbeDisplay();
  VbeDisplay(Device *p, VbeVersion version, List<Display::ScreenMode*> &sms, uintptr_t fbAddr);

  virtual ~VbeDisplay();

  virtual void *getFramebuffer();

  virtual bool getPixelFormat(Display::PixelFormat *pPf);

  virtual bool getCurrentScreenMode(Display::ScreenMode &sm);

  virtual bool getScreenModes(List<Display::ScreenMode*> &sms);

  virtual bool setScreenMode(Display::ScreenMode sm);

private:
  /** Copy constructor is private. */

  /** VBE version. */
  VbeVersion m_VbeVersion;

  /** Screen modes. */
  List<Display::ScreenMode*> m_ModeList;

  /** Current mode. */
  Display::ScreenMode m_Mode;

  MemoryMappedIo *m_pFramebuffer;
};

#endif
