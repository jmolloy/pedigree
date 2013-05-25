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

#ifndef FONT_H
#define FONT_H

#include <stdlib.h>
#include <stdint.h>
#include "environment.h"

#include <graphics/Graphics.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <cairo/cairo.h>
#include <cairo/cairo-ft.h>

class Font
{
public:
    Font(size_t requestedSize, const char *pFilename, bool bCache, size_t nWidth);
    virtual ~Font();

    virtual size_t render(PedigreeGraphics::Framebuffer *pFb, uint32_t c, size_t x, size_t y, uint32_t f, uint32_t b);

    virtual size_t render(const char *s, size_t x, size_t y, uint32_t f, uint32_t b, bool bBack = true);

    size_t getWidth()
    {return m_CellWidth;}
    size_t getHeight()
    {return m_CellHeight;}

    void setWidth(size_t w)
    {
        m_nWidth = w;
    }

    void precacheGlyph(uint32_t c, uint32_t f, uint32_t b);

private:

    Font(const Font&);
    Font &operator = (const Font&);

    struct Glyph
    {
        rgb_t *buffer;
        
        PedigreeGraphics::Buffer *pBlitBuffer;
    };
    void drawGlyph(PedigreeGraphics::Framebuffer *pFb, Glyph *pBitmap, int left, int top);
    Glyph *generateGlyph(uint32_t c, uint32_t f, uint32_t b);
    void cacheInsert(Glyph *pGlyph, uint32_t c, uint32_t f, uint32_t b);
    Glyph *cacheLookup(uint32_t c, uint32_t f, uint32_t b);

    static FT_Library m_Library;
    static bool m_bLibraryInitialised;
    
    FT_Face m_Face;
    size_t m_CellWidth;
    size_t m_CellHeight;
    size_t m_nWidth;
    size_t m_Baseline;

    bool m_bCache;
    struct CacheEntry
    {
        uint32_t c;
        uint32_t f, b;
        Glyph *value;
        CacheEntry *next;
    };
    CacheEntry **m_pCache;
    size_t m_CacheSize;

    cairo_user_data_key_t key;
    cairo_font_face_t *font_face;

    size_t m_FontSize;
};

#endif
