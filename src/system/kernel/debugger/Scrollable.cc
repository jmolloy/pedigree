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
#include "Scrollable.h"
#include <Log.h>

void Scrollable::move(size_t x, size_t y)
{
  m_x = x;
  m_y = y;
}
void Scrollable::resize(size_t width, size_t height)
{
  m_width = width;
  m_height = height;
}
void Scrollable::scroll(ssize_t lines)
{
  m_line += lines;

  if (m_line > (static_cast<ssize_t>(getLineCount()) - static_cast<ssize_t>(m_height)))
    m_line = static_cast<ssize_t>(getLineCount()) - static_cast<ssize_t>(m_height);

  if (m_line < 0)
    m_line = 0;
}
void Scrollable::scrollTo(size_t absolute)
{
  m_line = absolute;

  if (m_line > (static_cast<ssize_t>(getLineCount()) - static_cast<ssize_t>(m_height)))
    m_line = static_cast<ssize_t>(getLineCount()) - static_cast<ssize_t>(m_height);

  if (m_line < 0)
    m_line = 0;
}

void Scrollable::refresh(DebuggerIO *pScreen)
{
  pScreen->disableRefreshes();

  // TODO: Add a scroll bar on the right side of the window

  // Can we scroll up?
  DebuggerIO::Colour tmpColour;
  bool bCanScrollUp = (m_line > 0);
  bool bCanScrollDown = ( (m_line+m_height) < getLineCount());
  
  // For every available line.
  ssize_t line = m_line;
  for (size_t i = 0; i < m_height; i++)
  {
    pScreen->drawHorizontalLine(' ',
                                m_y + i,
                                m_x,
                                m_x + m_width - 1,
                                DebuggerIO::White,
                                DebuggerIO::Black);

    if (line >= 0 && line < static_cast<ssize_t>(getLineCount()))
    {
      DebuggerIO::Colour colour = DebuggerIO::White;
      DebuggerIO::Colour bgColour = DebuggerIO::Black;
      size_t colOffset;
      const char *Line = getLine1(line, colour, bgColour);
      if (Line)
        pScreen->drawString(Line, m_y + i, m_x, colour, bgColour);
      
      colour = DebuggerIO::White;
      bgColour = DebuggerIO::Black;
      Line = getLine2(line, colOffset, colour, bgColour);
      if (Line)
        pScreen->drawString(Line, m_y + i, m_x + colOffset, colour, bgColour);
    }
    line++;
  }
  
  // Draw scroll marks.
  char str[4] = {'^', ' ', m_ScrollUp, '\0'};
  if (bCanScrollUp)
    pScreen->drawString(str, m_y, m_x + m_width - 3, DebuggerIO::White, DebuggerIO::Blue);
  str[0] = 'v';
  str[2] = m_ScrollDown;
  if (bCanScrollDown)
    pScreen->drawString(str, m_y + m_height - 1, m_x + m_width - 3, DebuggerIO::White, DebuggerIO::Blue);

  pScreen->enableRefreshes();
}

void Scrollable::setScrollKeys(char up, char down)
{
  m_ScrollUp = up;
  m_ScrollDown = down;
}
	
void Scrollable::centreOn(size_t line)
{
  if( line > static_cast<size_t>(getLineCount()) )
	line = static_cast<size_t>(getLineCount());
  
  if( line < m_height )
  {
	scrollTo( 0 );
	return;
  }
	
  scrollTo( line - (m_height / 2) );
}
