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

#include <graphics/Graphics.h>

extern PedigreeGraphics::Framebuffer *g_pFramebuffer;

FT_Library Font::m_Library;
bool Font::m_bLibraryInitialised = false;

Font::Font(size_t requestedSize, const char *pFilename, bool bCache, size_t nWidth) :
    m_Face(), m_CellWidth(0), m_CellHeight(0), m_nWidth(nWidth), m_Baseline(requestedSize), m_bCache(bCache),
    m_pCache(0), m_CacheSize(0)
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

    m_CellWidth  = (m_Face->glyph->advance.x >> 6); // Because of hinting it's possible to go a pixel over the boundary.

    m_CellHeight = m_Face->size->metrics.height >> 6;
    m_Baseline = m_CellHeight;
    m_CellHeight += -(m_Face->size->metrics.descender >> 6);

    if (m_bCache)
    {
        m_CacheSize = 32256-1;
        m_pCache = new CacheEntry *[m_CacheSize];
        memset(m_pCache, 0, m_CacheSize*sizeof(CacheEntry*));
    }
}

Font::~Font()
{
}

size_t Font::render(PedigreeGraphics::Framebuffer *pFb, uint32_t c, size_t x, size_t y, uint32_t f, uint32_t b)
{
    Glyph *pGlyph = 0;
    bool bKillGlyph = true;
    // We can't cache characters over 0xFFFF anyway.
    if (m_bCache && c < 0xFFFF)
    {
        pGlyph = cacheLookup(c, f, b);
        if (!pGlyph)
        {
            pGlyph = generateGlyph(c, f, b);
            cacheInsert(pGlyph, c, f, b);
        }
        bKillGlyph = false;
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

void Font::drawGlyph(PedigreeGraphics::Framebuffer *pFb, Glyph *pBitmap, int left, int top)
{
    if(!g_pFramebuffer)
        return;
    
    pFb->blit(pBitmap->pBlitBuffer, 0, 0, left, top, m_CellWidth, m_CellHeight);
    
    /*
    for (size_t y = top; y < top+m_CellHeight; y++)
    {
        size_t idx = y*m_nWidth+left;

        memcpy(reinterpret_cast<uint8_t*>(&pFb[idx]),
               reinterpret_cast<uint8_t*>(&pBitmap->buffer[(y-top)*m_CellWidth]),
               m_CellWidth*sizeof(rgb_t));
    }
    */
}

Font::Glyph *Font::generateGlyph(uint32_t c, uint32_t f, uint32_t b)
{
    Glyph *pGlyph = new Glyph;
    pGlyph->buffer = new rgb_t[m_CellWidth*m_CellHeight];
    
    rgb_t back, front;
    back.r = (b & 0xFF0000) >> 16;
    back.g = (b & 0xFF00) >> 8;
    back.b = (b & 0xFF);
    front.r = (f & 0xFF0000) >> 16;
    front.g = (f & 0xFF00) >> 8;
    front.b = (f & 0xFF);
    front.a = back.a = 0;

    // Erase to background colour.
    for (size_t i = 0; i < m_CellWidth*m_CellHeight; i++)
        pGlyph->buffer[i] = back;

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
        if (r >= static_cast<ssize_t>(m_CellHeight)) break;
        for (ssize_t c = 0; c < m_Face->glyph->bitmap.width; c++)
        {
            if (c >= static_cast<ssize_t>(m_CellWidth)) break;
            size_t idx = (r+m_Baseline-m_Face->glyph->bitmap_top)*m_CellWidth + (m_Face->glyph->bitmap_left+c);
            if ((static_cast<int32_t>(idx) < 0) || idx >= m_CellWidth*m_CellHeight)
                continue;
            size_t gidx = (r*m_Face->glyph->bitmap.pitch)+c;
            pGlyph->buffer[idx] = interpolateColour(front, back, m_Face->glyph->bitmap.buffer[gidx]);
        }
    }
    
    pGlyph->pBlitBuffer = g_pFramebuffer->createBuffer(pGlyph->buffer, PedigreeGraphics::Bits32_Rgb, m_CellWidth, m_CellHeight);
    
    return pGlyph;
}

Font::Glyph *Font::cacheLookup(uint32_t c, uint32_t f, uint32_t b)
{
    if (m_pCache == 0) return 0;

    // Hash key is made up of the foreground, background and lower 16-bits of the character.
    uint64_t key = (static_cast<uint64_t>(f) << 40ULL) | (static_cast<uint64_t>(b) << 16ULL) | (c & 0xFFFF);

    key %= m_CacheSize;

    // Grab the bucket value.
    CacheEntry *pBucket = m_pCache[key];

    int i = 0;
    while (pBucket)
    {
        if (pBucket->c == c &&
            pBucket->f == f &&
            pBucket->b == b)
        {
            return pBucket->value;
        }
        else
            pBucket = pBucket->next;
        i++;
    }

    return 0;
}

void Font::cacheInsert(Glyph *pGlyph, uint32_t c, uint32_t f, uint32_t b)
{
    if (m_pCache == 0) return;

    // Hash key is made up of the foreground, background and lower 16-bits of the character.
    uint64_t key = (static_cast<uint64_t>(f) << 40ULL) | (static_cast<uint64_t>(b) << 16ULL) | (c & 0xFFFF);

    key %= m_CacheSize;

    // Grab the bucket value.
    CacheEntry *pBucket = m_pCache[key];

    if (!pBucket)
    {
        pBucket = new CacheEntry;
        pBucket->c = c;
        pBucket->f = f;
        pBucket->b = b;
        pBucket->value = pGlyph;
        pBucket->next = 0;

        m_pCache[key] = pBucket;
        return;
    }
    while (pBucket->next)
        pBucket = pBucket->next;

    // Last node in chain.
    pBucket->next = new CacheEntry;
    pBucket->next->c = c;
    pBucket->next->f = f;
    pBucket->next->b = b;
    pBucket->next->value = pGlyph;
    pBucket->next->next = 0;
}
