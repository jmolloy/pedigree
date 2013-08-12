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

#include <iconv.h>

#include <syslog.h>

#include <graphics/Graphics.h>

extern PedigreeGraphics::Framebuffer *g_pFramebuffer;

FT_Library Font::m_Library;
bool Font::m_bLibraryInitialised = false;

extern cairo_t *g_Cairo;

Font::Font(size_t requestedSize, const char *pFilename, bool bCache, size_t nWidth) :
    m_Face(), m_CellWidth(0), m_CellHeight(0), m_nWidth(nWidth), m_Baseline(requestedSize), m_bCache(bCache),
    m_pCache(0), m_CacheSize(0), key(), m_FontSize(requestedSize),
    m_Iconv(0), m_ConversionCache()
{
    char str[64];
    int error;
    if (!m_bLibraryInitialised)
    {
        error = FT_Init_FreeType(&m_Library);
        if (error)
        {
            syslog(LOG_ALERT, "Freetype init error: %d", error);
            return;
        }
        m_bLibraryInitialised = true;
    }

    error = FT_New_Face(m_Library, pFilename, 0,
                        &m_Face);
    if (error == FT_Err_Unknown_File_Format)
    {
        syslog(LOG_ALERT, "Freetype font format error");
        return;
    }
    else if (error)
    {
        syslog(LOG_ALERT, "Freetype font load error: %d", error);
        return;
    }

    font_face = cairo_ft_font_face_create_for_ft_face(m_Face, 0);
    cairo_font_face_set_user_data(font_face, &key,
                                  m_Face, (cairo_destroy_func_t) FT_Done_Face);

    cairo_save(g_Cairo);
    cairo_set_font_face(g_Cairo, font_face);
    cairo_set_font_size(g_Cairo, m_FontSize);

    cairo_font_extents_t extents;
    cairo_font_extents(g_Cairo, &extents);
    cairo_restore(g_Cairo);

    m_CellHeight = m_Baseline = extents.height;
    m_CellHeight += extents.descent;
    m_CellWidth = extents.max_x_advance;

    /// \todo UTF-32 endianness
    m_Iconv = iconv_open("UTF-8", "UTF-32LE");
    if(m_Iconv == (iconv_t) -1)
    {
        syslog(LOG_WARNING, "TUI: Font instance couldn't create iconv (%s)", strerror(errno));
    }
}

Font::~Font()
{
    iconv_close(m_Iconv);
}

size_t Font::render(PedigreeGraphics::Framebuffer *pFb, uint32_t c, size_t x, size_t y, uint32_t f, uint32_t b)
{
    // Let's try and skip any actual conversion.
    char *convertOut = 0;
    std::map<uint32_t,char *>::iterator it = m_ConversionCache.find(c);
    if(it != m_ConversionCache.end())
    {
        convertOut = it->second;
    }
    else
    {
        if(m_Iconv == (iconv_t) -1)
        {
            syslog(LOG_WARNING, "TUI: Font instance with bad iconv.");
            return 0;
        }

        // Reset iconv conversion state.
        iconv(m_Iconv, 0, 0, 0, 0);

        // Convert UTF-32 input character to UTF-8 for Cairo rendering.
        uint32_t utf32[] = {c, 0};
        char *utf32_c = (char *) utf32;
        char *out = new char[100];
        char *out_c = (char *) out;
        size_t utf32_len = 8;
        size_t out_len = 100;
        size_t res = iconv(m_Iconv, &utf32_c, &utf32_len, &out_c, &out_len);

        size_t ret = 0;
        if(res == ((size_t) -1))
        {
            syslog(LOG_WARNING, "TUI: Font::render couldn't convert input UTF-32 %x", c);
            delete [] out;
            return 0;
        }
        else
        {
            convertOut = out;
            m_ConversionCache[c] = convertOut;
        }
    }

    // Perform the render.
    return render(convertOut, x, y, f, b);
}

size_t Font::render(const char *s, size_t x, size_t y, uint32_t f, uint32_t b, bool bBack)
{
    cairo_save(g_Cairo);
    cairo_set_operator(g_Cairo, CAIRO_OPERATOR_SOURCE);
    size_t len = strlen(s);

    cairo_set_font_face(g_Cairo, font_face);
    cairo_set_font_size(g_Cairo, m_FontSize);

    if(bBack)
    {
        cairo_set_source_rgba(
                g_Cairo,
                ((b >> 16) & 0xFF) / 256.0,
                ((b >> 8) & 0xFF) / 256.0,
                ((b) & 0xFF) / 256.0,
                0.8);

        cairo_rectangle(g_Cairo, x, y, m_CellWidth * len, m_CellHeight);
        cairo_fill(g_Cairo);
    }

    cairo_set_source_rgba(
            g_Cairo,
            ((f >> 16) & 0xFF) / 256.0,
            ((f >> 8) & 0xFF) / 256.0,
            ((f) & 0xFF) / 256.0,
            1.0);

    cairo_move_to(g_Cairo, x, y + m_Baseline);
    cairo_show_text(g_Cairo, s);
    cairo_restore(g_Cairo);

    return m_CellWidth * len;
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

void Font::precacheGlyph(uint32_t c, uint32_t f, uint32_t b)
{
    generateGlyph(c, f, b);
}

Font::Glyph *Font::generateGlyph(uint32_t c, uint32_t f, uint32_t b)
{
    Glyph *pGlyph = 0;
    if((pGlyph = cacheLookup(c, f, b)))
        return pGlyph;

    pGlyph = new Glyph;
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
        syslog(LOG_ALERT, "Freetype load char error: %d", error);
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
