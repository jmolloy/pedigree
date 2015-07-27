/*
 * Copyright (c) 2008-2014, Pedigree Developers
 *
 * Please see the CONTRIB file in the root of the source tree for a full
 * list of contributors.
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
#include <cerrno>
#include <cmath>

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

    m_CellHeight = m_Baseline = std::ceil(extents.height);
    double ascent_descent = std::ceil(extents.ascent + extents.descent);
    if(m_CellHeight < ascent_descent)
        m_CellHeight = ascent_descent;
    else
        m_CellHeight += std::ceil(extents.descent);
    m_CellWidth = std::ceil(extents.max_x_advance);

    /// \todo UTF-32 endianness
    m_Iconv = iconv_open("UTF-8", "UTF-32LE");
    if(m_Iconv == (iconv_t) -1)
    {
        syslog(LOG_WARNING, "TUI: Font instance couldn't create iconv (%s)", strerror(errno));
    }

    for(uint32_t c = 32; c < 127; ++c)
    {
        precache(c);
    }
}

Font::~Font()
{
    iconv_close(m_Iconv);
    FT_Done_Face(m_Face);

    // Destroy our precached glyphs.
    for (std::map<uint32_t,char *>::iterator it = m_ConversionCache.begin();
         it != m_ConversionCache.end();
         ++it)
    {
        delete [] it->second;
    }
}

size_t Font::render(PedigreeGraphics::Framebuffer *pFb, uint32_t c, size_t x, size_t y, uint32_t f, uint32_t b)
{
    // Cache the character, if not already.
    const char *convertOut = precache(c);
    if(!convertOut)
    {
        syslog(LOG_WARNING, "TUI: Character '%x' was not able to be precached?", c);
        return 0;
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

const char *Font::precache(uint32_t c)
{
    if(m_Iconv == (iconv_t) -1)
    {
        syslog(LOG_WARNING, "TUI: Font instance with bad iconv.");
        return 0;
    }

    // Let's try and skip any actual conversion.
    std::map<uint32_t,char *>::iterator it = m_ConversionCache.find(c);
    if(it == m_ConversionCache.end())
    {
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

        if(res == ((size_t) -1))
        {
            syslog(LOG_WARNING, "TUI: Font::render couldn't convert input UTF-32 %x", c);
            delete [] out;
        }
        else
        {
            m_ConversionCache[c] = out;
            return out;
        }
    }
    else
    {
        return it->second;
    }

    return 0;
}
