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

#ifndef DEBUGGER_IO_H
#define DEBUGGER_IO_H

#include <utilities/StaticString.h>

#define COMMAND_MAX    256
class DebuggerCommand;

/**
 * Defines an interface for a console for the debugger to interact with the user.
 *
 * The console can be in "raw" or "cli" mode - In CLI mode it is expected that the 
 * writeCli and readCli functions are heavily used - scrolling and backspacing is provided.
 * In "raw" mode, any function (except writeCli and readCli) may be used.
 *
 * The CLI interface has support for contracting the available window used for the CLI. The
 * setCli{Upper|Lower}Limit functions define how many lines the CLI should not touch from the
 * top and bottom of the screen, respectively.
 *
 * Note that columns are numbered starting at 0 from the left, and rows are numbered
 * starting at 0 from the top.
 */
class DebuggerIO
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

  /**
   * Default constructor and destructor.
   */
  DebuggerIO() :
    m_bReady(true),
    m_bRefreshesEnabled(true)
  {}
  virtual ~DebuggerIO(){}

  /**
   * Forces the command line interface not to use the specified number of lines
   * from either the top or bottom of the screen, respectively. Can be used to
   * create status lines that aren't destroyed by screen scrolling.
   */
  virtual void setCliUpperLimit(int nlines) = 0;
  virtual void setCliLowerLimit(int nlines) = 0;

  /**
   * Enables or disables the command line interface, allowing full access to the display.
   */
  virtual void enableCli() = 0;
  virtual void disableCli() = 0;

  /**
   * Writes the given text out to the CLI, in the given colour and background colour.
   */
  virtual void writeCli(const char *str, Colour foreColour, Colour backColour);
  virtual void writeCli(char ch, DebuggerIO::Colour foreColour, DebuggerIO::Colour backColour);
  /**
   * Reads a command from the interface. Blocks until a character is pressed, and then
   * the current buffer is returned in *str, and the return value is true if the command
   * is complete (if enter has been pressed). *str will never exceed maxLen.
   */
  virtual bool readCli(HugeStaticString &str, DebuggerCommand *pAutoComplete);

  /**
   * Draw a line of characters in the given fore and back colours, in the 
   * horizontal or vertical direction. Note that if the CLI is enabled,
   * anything drawn across the CLI area can be wiped without warning.
   */
  virtual void drawHorizontalLine(char c, int row, int colStart, int colEnd, Colour foreColour, Colour backColour) = 0;
  virtual void drawVerticalLine(char c, int col, int rowStart, int rowEnd, Colour foreColour, Colour backColour) = 0;

  /**
   * Draws a string of text at the given location in the given colour.
   * note that wrapping is not performed, the string will be clipped.
   */
  virtual void drawString(const char *str, int row, int col, Colour foreColour, Colour backColour) = 0;

  /**
   * Returns the width and height respectively of the console.
   */
  virtual int getWidth() = 0;
  virtual int getHeight() = 0;

  /**
   * Allows disabling of refreshes, for example when deleting something then writing it back.
   */
  virtual void enableRefreshes() = 0;
  virtual void disableRefreshes() = 0;
  virtual void forceRefresh() = 0;
  
  /**
   * Gets a character from the keyboard. Blocking. Returns 0 for a nonprintable character.
   */
  virtual char getChar() = 0;
protected:
  virtual void putChar(char c, DebuggerIO::Colour foreColour, DebuggerIO::Colour backColour) =0;
  
  /**
   * Scrolls the CLI screen down a line, if needed.
   */
  virtual void scroll() =0;

  /**
   * Updates the hardware cursor position.
   */
  virtual void moveCursor() =0;
  
  /**
   * Are we ready to recieve a new command? (or are we recieving one already)
   */
  bool m_bReady;
  
  /**
   * Command buffer.
   */
  char m_pCommand[COMMAND_MAX];
  
  /**
   * Should we auto-refresh the screen?
   */
  bool m_bRefreshesEnabled;
};

#endif

