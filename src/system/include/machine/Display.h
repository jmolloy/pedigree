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
#ifndef MACHINE_DISPLAY_H
#define MACHINE_DISPLAY_H

#include <machine/Device.h>
#include <utilities/List.h>

/**
 * A display is either a dumb framebuffer or something more accelerated.
 */
class Display : public Device
{
public:

  /** Describes the format of a pixel in a buffer. */
  struct PixelFormat
  {
    uint8_t mRed;       ///< Red mask.
    uint8_t pRed;       ///< Position of red field.
    uint8_t mGreen;     ///< Green mask.
    uint8_t pGreen;     ///< Position of green field.
    uint8_t mBlue;      ///< Blue mask.
    uint8_t pBlue;      ///< Position of blue field.
    uint8_t mAlpha;     ///< Alpha mask.
    uint8_t pAlpha;     ///< Position of the alpha field.
    uint8_t nBpp;       ///< Bits per pixel (total).
    uint32_t nPitch;    ///< Bytes per scanline.
  };

  /** Describes a screen mode / resolution */
  struct ScreenMode
  {
    uint32_t id;
    uint32_t width;
    uint32_t height;
    uint32_t refresh;
    uintptr_t framebuffer;
    PixelFormat pf;
  };

  Display()
  {
    m_SpecificType = "Generic Display";
  }
  Display(Device *p) :
    Device(p)
  {
  }
  virtual ~Display()
  {
  }

  virtual Type getType()
  {
    return Device::Display;
  }

  virtual void getName(String &str)
  {
    str = "Generic Display";
  }

  virtual void dump(String &str)
  {
    str = "Generic Display";
  }

  /** Returns a pointer to a linear framebuffer.
      Returns 0 if none available. */
  virtual void *getFramebuffer()
  {
    return 0;
  }

  /** Returns the format of each pixel in the framebuffer, along with the bits-per-pixel.
      \return True if operation succeeded, false otherwise. */
  virtual bool getPixelFormat(PixelFormat &pf)
  {
    return false;
  }

  /** Returns the current screen mode.
      \return True if operation succeeded, false otherwise. */
  virtual bool getCurrentScreenMode(ScreenMode &sm)
  {
    return false;
  }

  /** Fills the given List with all of the available screen modes.
      \return True if operation succeeded, false otherwise. */
  virtual bool getScreenModes(List<ScreenMode*> &sms)
  {
    return false;
  }

  /** Sets the current screen mode.
      \return True if operation succeeded, false otherwise. */
  virtual bool setScreenMode(ScreenMode sm)
  {
    return false;
  }

};

#endif
