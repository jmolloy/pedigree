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

#ifndef FREETYPE_XTERM_H
#define FREETYPE_XTERM_H

#include "Xterm.h"
#include <ft2build.h>
#include FT_FREETYPE_H

/** An Xterm implementation that uses freetype as its rendering library. */
class FreetypeXterm : public Xterm
{
public:
    FreetypeXterm(Display::ScreenMode mode, void *pFramebuffer);
    virtual ~FreetypeXterm();

protected:
    virtual void putCharFb(uint32_t c, int x, int y, uint32_t f, uint32_t b);

    struct Glyph
    {
        uint8_t *buffer;
        uint8_t rows;
        uint8_t width;
        uint8_t pitch;
        uint8_t left;
        uint8_t top;
    };

    void drawBitmap(Glyph *bitmap, int left, int top, uint32_t f, uint32_t b);

    uint32_t interpolateColour(uint32_t f, uint32_t b, uint8_t a);

    FT_Library m_Library;
    FT_Face    m_NormalFont;
    FT_Face    m_BoldFont;

    size_t m_FontWidth;
    size_t m_FontHeight;

    Glyph m_Cache[0x10000];
};

#endif
