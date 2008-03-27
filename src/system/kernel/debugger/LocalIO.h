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
#ifndef LOCALIO_H
#define LOCALIO_H

#include <DebuggerIO.h>

class DebuggerCommand;

#define CONSOLE_WIDTH  80
#define CONSOLE_HEIGHT 25
#define CONSOLE_DEFAULT_MODE   g_80x25_text

#define VGA_BASE                0x3C0
#define VGA_AC_INDEX            0x0
#define VGA_AC_WRITE            0x0
#define VGA_AC_READ             0x1
#define VGA_MISC_WRITE          0x2
#define VGA_SEQ_INDEX           0x4
#define VGA_SEQ_DATA            0x5
#define VGA_DAC_READ_INDEX      0x7
#define VGA_DAC_WRITE_INDEX     0x8
#define VGA_DAC_DATA            0x9
#define VGA_MISC_READ           0xC
#define VGA_GC_INDEX            0xE
#define VGA_GC_DATA             0xF
/*                              COLOR emulation  MONO emulation */
#define VGA_CRTC_INDEX          0x14             /* 0x3B4 */
#define VGA_CRTC_DATA           0x15             /* 0x3B5 */
#define VGA_INSTAT_READ         0x1A

#define VGA_NUM_SEQ_REGS        5
#define VGA_NUM_CRTC_REGS       25
#define VGA_NUM_GC_REGS         9
#define VGA_NUM_AC_REGS         21
#define VGA_NUM_REGS            (1 + VGA_NUM_SEQ_REGS + VGA_NUM_CRTC_REGS + \
                                VGA_NUM_GC_REGS + VGA_NUM_AC_REGS)

/**
 * Provides an implementation of DebuggerIO, using the monitor and
 * keyboard.
 */
class LocalIO : public DebuggerIO
{
public:
  /**
   * Default constructor and destructor.
   */
  LocalIO();
  ~LocalIO();

  /**
   * Forces the command line interface not to use the specified number of lines
   * from either the top or bottom of the screen, respectively. Can be used to 
   * create status lines that aren't destroyed by screen scrolling.
   */
  void setCliUpperLimit(size_t nlines);
  void setCliLowerLimit(size_t nlines);

  /**
   * Enables or disables the command line interface, allowing full access to the display.
   * disableCli blanks the screen, enableCli blanks it and puts a prompt up.
   */
  void enableCli();
  void disableCli();

  void cls();
  
  virtual char getCharNonBlock();
  
  /**
   * Draw a line of characters in the given fore and back colours, in the 
   * horizontal or vertical direction. Note that if the CLI is enabled,
   * anything drawn across the CLI area can be wiped without warning.
   */
  void drawHorizontalLine(char c, size_t row, size_t colStart, size_t colEnd, DebuggerIO::Colour foreColour, DebuggerIO::Colour backColour);
  void drawVerticalLine(char c, size_t col, size_t rowStart, size_t rowEnd, DebuggerIO::Colour foreColour, DebuggerIO::Colour backColour);

  /**
   * Draws a string of text at the given location in the given colour.
   * note that wrapping is not performed, the string will be clipped.
   */
  void drawString(const char *str, size_t row, size_t col, DebuggerIO::Colour foreColour, DebuggerIO::Colour backColour);

  /**
   * Returns the width and height respectively of the console.
   */
  size_t getWidth()
  {
    return CONSOLE_WIDTH;
  }
  size_t getHeight()
  {
    return CONSOLE_HEIGHT;
  }

  /**
   * Allows disabling of refreshes, for example when deleting something then writing it back.
   */
  void enableRefreshes();
  void disableRefreshes();
  void forceRefresh();

  /**
   * Gets a character from the keyboard. Blocking. Returns 0 for a nonprintable character.
   */
  char getChar();
  
protected:
 
  /**
   * Scrolls the CLI screen down a line, if needed.
   */
  void scroll();

  /**
   * Updates the hardware cursor position.
   */
  void moveCursor();

  void putChar(char c, DebuggerIO::Colour foreColour, DebuggerIO::Colour backColour);

  void setMode(int nMode);
  int getMode();
  
  /**
   * Framebuffer.
   */
  uint16_t m_pFramebuffer[CONSOLE_WIDTH*CONSOLE_HEIGHT];

  /**
   * Framebuffer for the screen before we took control.
   */
  uint16_t m_pOldFramebuffer[CONSOLE_WIDTH*CONSOLE_HEIGHT];

  /**
   * Current upper and lower CLI limits.
   */
  size_t m_UpperCliLimit; /// How many lines from the top of the screen the top of our CLI area is.
  size_t m_LowerCliLimit; /// How many lines from the bottom of the screen the bottom of our CLI area is.

  /**
   * Current cursor position.
   */
  size_t m_CursorX, m_CursorY;

  /**
   * Keyboard state.
   */
  bool m_bShift;
  bool m_bCtrl;
  bool m_bCapslock;
};

#endif
