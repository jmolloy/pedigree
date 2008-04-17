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
#ifndef SCROLLABLE_H
#define SCROLLABLE_H

#include <processor/types.h>
#include "DebuggerIO.h"

/** @addtogroup kerneldebugger
 * @{ */

class Scrollable
{
  public:
    inline Scrollable()
      : m_x(0), m_y(0), m_width(0), m_height(0), m_line(0),
        m_ScrollUp('j'), m_ScrollDown('k') {};

    void move(size_t x, size_t y);
    void resize(size_t width, size_t height);
    void scroll(ssize_t lines);
    void scrollTo(size_t absolute);
    void refresh(DebuggerIO *pScreen);
    void setScrollKeys(char up, char down);
	
	void centreOn(size_t line);

    inline size_t height() const{return m_height;}
    inline size_t width() const{return m_width;}

    virtual const char *getLine1(size_t index, DebuggerIO::Colour &colour, DebuggerIO::Colour &bgColour) = 0;
    virtual const char *getLine2(size_t index, size_t &colOffset, DebuggerIO::Colour &colour, DebuggerIO::Colour &bgColour) = 0;
    virtual size_t getLineCount() = 0;
    inline virtual ~Scrollable(){}

  protected:
    size_t m_x;
    size_t m_y;
    size_t m_width;
    size_t m_height;
    ssize_t m_line;
    char m_ScrollUp;
    char m_ScrollDown;
};

/** @} */

#endif
