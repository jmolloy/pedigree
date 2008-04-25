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
#ifndef BOOTIO_H
#define BOOTIO_H

#include <utilities/StaticString.h>
#include <machine/Serial.h>

/**
 * A class which provides *extremely* simple output to both the Vga class and Serial classes,
 * intended to be used for displaying messages during bootup.
 */
class BootIO
{
public:
  /**
   * Enumeration of all possible foreground and background colours.
   */
  enum Colour
  {
    Black       =0,
    Blue        =1,
    Green       =2,
    Cyan        =3,
    Red         =4,
    Magenta     =5,
    Orange      =6,
    LightGrey   =7,
    DarkGrey    =8,
    LightBlue   =9,
    LightGreen  =10,
    LightCyan   =11,
    LightRed    =12,
    LightMagenta=13,
    Yellow      =14,
    White       =15
  };
  
  /** Constructor. I hope you knew that already. */
  BootIO ();
  /** Destructor. Let's hope you knew that too. */
  ~BootIO ();
  
  /** Initialise the BootIO - If we have a VGA device this clears the screen. */
  void initialise();
  
  /** Write a string out to each output device.
   * \param str The string to write
   * \param foreColour The foreground colour
   * \param backColour The background colour */
  void write (HugeStaticString &str, Colour foreColour, Colour backColour);

private:
  void putCharVga(const char c, Colour foreColour, Colour backColour);
  void startColour(Serial *pSerial, Colour foreColour, Colour backColour);
  void endColour(Serial *pSerial);
  
  /**
   * Current cursor position for VGA.
   */
  size_t m_CursorX, m_CursorY;
};

#endif
