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
  void setCliUpperLimit(int nlines);
  void setCliLowerLimit(int nlines);

  /**
   * Enables or disables the command line interface, allowing full access to the display.
   * disableCli blanks the screen, enableCli blanks it and puts a prompt up.
   */
  void enableCli();
  void disableCli();

  /**
   * Draw a line of characters in the given fore and back colours, in the 
   * horizontal or vertical direction. Note that if the CLI is enabled,
   * anything drawn across the CLI area can be wiped without warning.
   */
  void drawHorizontalLine(char c, int row, int colStart, int colEnd, DebuggerIO::Colour foreColour, DebuggerIO::Colour backColour);
  void drawVerticalLine(char c, int col, int rowStart, int rowEnd, DebuggerIO::Colour foreColour, DebuggerIO::Colour backColour);

  /**
   * Draws a string of text at the given location in the given colour.
   * note that wrapping is not performed, the string will be clipped.
   */
  void drawString(const char *str, int row, int col, DebuggerIO::Colour foreColour, DebuggerIO::Colour backColour);

  /**
   * Returns the width and height respectively of the console.
   */
  int getWidth()
  {
    return CONSOLE_WIDTH;
  }
  int getHeight()
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

  /**
   * Framebuffer.
   */
  unsigned short m_pFramebuffer[CONSOLE_WIDTH*CONSOLE_HEIGHT];

  /**
   * Framebuffer for the screen before we took control.
   */
  unsigned short m_pOldFramebuffer[CONSOLE_WIDTH*CONSOLE_HEIGHT];

  /**
   * Current upper and lower CLI limits.
   */
  int m_UpperCliLimit; /// How many lines from the top of the screen the top of our CLI area is.
  int m_LowerCliLimit; /// How many lines from the bottom of the screen the bottom of our CLI area is.

  /**
   * Current cursor position.
   */
  int m_CursorX, m_CursorY;

  /**
   * Keyboard state.
   */
  bool m_bShift;
  bool m_bCtrl;
  bool m_bCapslock;
};

#endif
