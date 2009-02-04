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

#include <processor/types.h>
#include <machine/Display.h>

#define C_BLACK  0
#define C_RED    1
#define C_GREEN  2
#define C_YELLOW 3
#define C_BLUE   4
#define C_MAGENTA 5
#define C_CYAN   6
#define C_WHITE  7

#define C_BRIGHT 8

/** \brief A VT100 compatible terminal emulator.

    This implementation uses the idea of a "line" to simplify line editing and wrapping - a line can be larger than the width of
    the screen. The implementation uses a circular queue of Lines as the primary data store, and uses these along with a sliding
    window to render the screen contents. */
class Vt100
{
public:
  Vt100(Display::ScreenMode mode, void *pFramebuffer);
  ~Vt100();

  void write(char c);
  void write(char *str);

private:

  /** The persistent state between characters/lines. */
  struct State
  {
    uint32_t fg;
    uint32_t bg;
  };

  /** Here, a field in State with value ~0 indicates the field should not change. */
  struct StateChange
  {
    uint32_t col;

    uint32_t fg;
    uint32_t bg;
  };

  /** One line - a set of characters and an associated list of state changes. */
  struct Line
  {
    char *str;
    int col;
    State initialState;
    int rows;
    List<StateChange*> changes;
  };

  /** Writes a character out to the framebuffer. Takes a preformatted integer for the foreground and background colours.
      x and y are in characters, not pixels. */
  void putChar(char c, int x, int y, uint32_t f, uint32_t b);

  /** Perform a full-on screen refresh. */
  void refresh(int32_t line=-1, int32_t col=-1);

  /** Render one line, starting at colStart. */
  State renderLine(Line *pLine, int row, int colStart, int cursorCol);

  /** Force the display engine to display a line - this doesn't refresh it, merely makes sure the visible window
      is adjusted such that the given line will be on the screen when a refresh occurs. */
  void ensureLineDisplayed(int l);

  /** Creates a colour integer suitable for inserting into the framebuffer from an R, G and B list. */
  uint32_t compileColour(uint8_t r, uint8_t g, uint8_t b);

  /** Screen mode, pixel format etc */
  Display::ScreenMode m_Mode;

  /** Framebuffer address. */
  uint8_t *m_pFramebuffer;

  /** Colour list. This is the list of precompiled colour values for the current pixel format. */
  uint32_t m_pColours[16];

  State m_State;

  //
  // Cursor position.
  //
  int m_CursorX, m_CursorY;

  //
  // General properties
  //
  uint32_t m_nWidth, m_nHeight;

  //
  // Window properties
  //
  uint32_t m_WinViewStart;
  uint32_t m_AlternateWinViewStart;
  bool m_bIsAlternateWindow; // An alternate window can be used - fullscreen things like nano use this.

  //
  // Actual data.
  //

  // These are circular queues of lines.
  Line *m_pLines;
  Line *m_pAlternateLines;
  uint32_t m_nLines;
  uint32_t m_nAlternateLines;

  // Current insert position in the m_Lines queue.
  uint32_t m_nLinesPos;
  // ... and in the m_AlternateLines queue.
  uint32_t m_nAlternateLinesPos;

  // Last populated line in the m_Lines queue.
  uint32_t m_nLastPos;
  // ... and in the m_AlternateLines queue.
  uint32_t m_nAlternateLastPos;

  // Are we currently interpreting a state change?
  bool m_bChangingState;

  bool m_bDontRefresh;

  typedef struct
  {
    int params[4];
    int cur_param;
  } Vt100Cmd;
  Vt100Cmd m_Cmd;
};
