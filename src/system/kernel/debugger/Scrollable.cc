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
void Scrollable::draw(DebuggerIO *pScreen)
{
  // NOTE: We assume pScreen is in raw mode (say member function disableCli already called)

  // Clear the top status lines.
  pScreen->drawHorizontalLine(' ',
                              m_y,
                              m_x,
                              m_width - 1,
                              DebuggerIO::White,
                              DebuggerIO::Green);

  // Write the correct text in the upper status line.
  pScreen->drawString(getName(),
                      m_y,
                      m_x,
                      DebuggerIO::White,
                      DebuggerIO::Green);
}
void Scrollable::refresh(DebuggerIO *pScreen)
{
  pScreen->disableRefreshes();

  // TODO: Add a scroll bar on the right side of the window

  // For every available line.
  ssize_t line = m_line;
  for (size_t i = 0; i < m_height - 1; i++)
  {
    pScreen->drawHorizontalLine(' ',
                                m_y + i + 1,
                                m_x,
                                m_width - 1,
                                DebuggerIO::White,
                                DebuggerIO::Black);

    if (line >= 0 && line < static_cast<ssize_t>(getLineCount()))
    {
      DebuggerIO::Colour colour = DebuggerIO::White;
      const char *Line = getLine(line, colour);

      pScreen->drawString(Line, m_y + i + 1, m_x, colour, DebuggerIO::Black);
    }
    line++;
  }

  pScreen->enableRefreshes();
}
