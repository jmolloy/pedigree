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

#include "Font.h"

FT_Library Font::m_Library;
bool Font::m_bLibraryInitialised = false;

Font::Font(size_t requestedSize, const char *pFilename, bool bCache, size_t nWidth) :
    m_Face(), m_CellWidth(0), m_CellHeight(0), m_nWidth(nWidth), m_Baseline(requestedSize), m_bCache(bCache),
    m_pCache(0)
{
    char str[64];
    int error;
    if (!m_bLibraryInitialised)
    {
        error = FT_Init_FreeType(&m_Library);
        if (error)
        {
            sprintf(str, "Freetype init error: %d\n", error);
            log(str);
            return;
        }
        m_bLibraryInitialised = true;
    }

    error = FT_New_Face(m_Library, pFilename, 0,
                        &m_Face);
    if (error == FT_Err_Unknown_File_Format)
    {
        sprintf(str, "Freetype font format error.\n");
        log(str);
        return;
    }
    else if (error)
    {
        sprintf(str, "Freetype font load error: %d\n", error);
        log(str);
        return;
    }

    error = FT_Set_Pixel_Sizes(m_Face, 0, requestedSize); 
    if (error)
    {
        sprintf(str, "Freetype set pixel size error: %d\n", error);
        log(str);
        return;
    }

    error = FT_Load_Char(m_Face, '@', FT_LOAD_RENDER);

    m_CellWidth  = (m_Face->glyph->advance.x >> 6)+1; // Because of hinting it's possible to go a pixel over the boundary.

    m_CellHeight = m_Face->size->metrics.height >> 6;
    m_Baseline = m_CellHeight;
    m_CellHeight += -(m_Face->size->metrics.descender >> 6);
}

Font::~Font()
{
}

size_t Font::render(rgb_t *pFb, uint32_t c, size_t x, size_t y, rgb_t f, rgb_t b)
{
    Glyph *pGlyph = 0;
    bool bKillGlyph = true;
    // We can't cache characters over 0xFF anyway.
    if (m_bCache && c < 0xFF)
    {
        pGlyph = cacheLookup(c, f, b);
        if (!pGlyph)
        {
            pGlyph = generateGlyph(c, f, b);
            cacheInsert(pGlyph, c, f, b);
            bKillGlyph = false;
        }
    }
    else
        pGlyph = generateGlyph(c, f, b);

    if (!pGlyph) return 0;

    drawGlyph(pFb, pGlyph, x, y);

    if (bKillGlyph)
    {
        delete [] pGlyph->buffer;
        delete pGlyph;
    }

    return m_CellWidth;
}

void Font::drawGlyph(rgb_t *pFb, Glyph *pBitmap, int left, int top)
{
    for (size_t y = top; y < top+m_CellHeight; y++)
    {
        for (size_t x = left; x < left+m_CellWidth; x++)
        {
            size_t idx = y*m_nWidth+x;

/*            if (m_pBackground)
                pFb[idx] = interpolateColour(m_pBackground[idx], pBitmap->buffer[(y-top)*m_CellWidth+(x-left)], 128);
                else*/
                pFb[idx] = pBitmap->buffer[(y-top)*m_CellWidth+(x-left)];
        }
    }
}

Font::Glyph *Font::generateGlyph(uint32_t c, rgb_t f, rgb_t b)
{
    Glyph *pGlyph = new Glyph;
    pGlyph->buffer = new rgb_t[m_CellWidth*m_CellHeight];

    // Erase to background colour.
    for (size_t i = 0; i < m_CellWidth*m_CellHeight; i++)
        pGlyph->buffer[i] = b;

    int error = FT_Load_Char(m_Face, c, FT_LOAD_RENDER);
    if (error)
    {
        char str[64];
        sprintf(str, "Freetype load char error: %d\n", error);
        log(str);
        return 0;
    }
    
    for (ssize_t r = 0; r < m_Face->glyph->bitmap.rows; r++)
    {
        for (ssize_t c = 0; c < m_Face->glyph->bitmap.width; c++)
        {
            size_t idx = (r+m_Baseline-m_Face->glyph->bitmap_top)*m_CellWidth + (m_Face->glyph->bitmap_left+c);
            if (idx < 0 || idx >= m_CellWidth*m_CellHeight)
                continue;
            size_t gidx = (r*m_Face->glyph->bitmap.pitch)+c;
            pGlyph->buffer[idx] = interpolateColour(f, b, m_Face->glyph->bitmap.buffer[gidx]);
        }
    }
    return pGlyph;
}

Font::Glyph *Font::cacheLookup(uint32_t c, rgb_t f, rgb_t b)
{
    if (m_pCache == 0) return 0;

    bool bFound = false;
    CacheEntry *pEntry = m_pCache;
    while (pEntry)
    {
        if (pEntry->key.r == f.r &&
            pEntry->key.g == f.g &&
            pEntry->key.b == f.b)
        {
            bFound = true;
            break;
        }
        pEntry = pEntry->next;
    }

    if (!bFound) return 0;

    bFound = false;
    pEntry = reinterpret_cast<CacheEntry*>(pEntry->value);
    while (pEntry)
    {
        if (pEntry->key.r == b.r &&
            pEntry->key.g == b.g &&
            pEntry->key.b == b.b)
        {
            bFound = true;
            break;
        }
        pEntry = pEntry->next;
    }

    if (!bFound) return 0;

    Glyph **ppGlyphs = reinterpret_cast<Glyph**>(pEntry->value);
    return ppGlyphs[c&0xFF];
}

void Font::cacheInsert(Glyph *pGlyph, uint32_t c, rgb_t f, rgb_t b)
{
#if 0
    bool bFound = false;
    CacheEntry *pEntry = m_pCache;
    if (m_pCache == 0)
    {
    }
    else
        while (pEntry->next)
        {
            if (pEntry->next
        }
#endif
}
