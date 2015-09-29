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
    m_Iconv(0), m_ConversionCache(), m_FontDesc(0)
{
    m_FontDesc = pango_font_description_from_string(pFilename);

    PangoFontMetrics *metrics = 0;
    PangoFontMap *fontmap = pango_cairo_font_map_get_default();
    PangoContext *context = pango_font_map_create_context(fontmap);
    pango_context_set_font_description(context, m_FontDesc);
    metrics = pango_context_get_metrics(context, m_FontDesc, NULL);
    g_object_unref(context);

    m_CellWidth = pango_font_metrics_get_approximate_char_width(metrics) / PANGO_SCALE;
    m_CellHeight = (pango_font_metrics_get_ascent(metrics) + pango_font_metrics_get_descent(metrics)) / PANGO_SCALE;
    m_Baseline = pango_font_metrics_get_ascent(metrics) / PANGO_SCALE;

    syslog(LOG_INFO, "metrics: %dx%d", m_CellWidth, m_CellHeight);

    pango_font_metrics_unref(metrics);

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

    // Destroy our precached glyphs.
    for (std::map<uint32_t,char *>::iterator it = m_ConversionCache.begin();
         it != m_ConversionCache.end();
         ++it)
    {
        delete [] it->second;
    }

    pango_font_description_free(m_FontDesc);
}

size_t Font::render(PedigreeGraphics::Framebuffer *pFb, uint32_t c, size_t x, size_t y, uint32_t f, uint32_t b, bool bBack, bool bBold, bool bItalic, bool bUnderline)
{
    // Cache the character, if not already.
    const char *convertOut = precache(c);
    if(!convertOut)
    {
        syslog(LOG_WARNING, "TUI: Character '%x' was not able to be precached?", c);
        return 0;
    }

    // Perform the render.
    return render(convertOut, x, y, f, b, bBack, bBold, bItalic, bUnderline);
}

size_t Font::render(const char *s, size_t x, size_t y, uint32_t f, uint32_t b,
    bool bBack, bool bBold, bool bItalic, bool bUnderline)
{
    PangoAttrList *attrs = pango_attr_list_new();
    if (bBold)
    {
        PangoAttribute *attr = pango_attr_weight_new(PANGO_WEIGHT_BOLD);
        pango_attr_list_insert(attrs, attr);
    }
    if (bItalic)
    {
        PangoAttribute *attr = pango_attr_style_new(PANGO_STYLE_OBLIQUE);
        pango_attr_list_insert(attrs, attr);
    }
    if (bUnderline)
    {
        PangoAttribute *attr = pango_attr_underline_new(PANGO_UNDERLINE_SINGLE);
        pango_attr_list_insert(attrs, attr);
    }

    cairo_save(g_Cairo);
    PangoLayout *layout = pango_cairo_create_layout(g_Cairo);
    pango_layout_set_attributes(layout, attrs);
    pango_layout_set_font_description(layout, m_FontDesc);
    pango_layout_set_text(layout, s, -1);  // Not using markup here - intentional.
    pango_attr_list_unref(attrs);

    int width = 0, height = 0;
    pango_layout_get_size(layout, &width, &height);
    if ((width < 0) || (height < 0))
    {
        // Bad layout size.
        /// \todo cleanup
        return 0;
    }
    width /= PANGO_SCALE;
    height /= PANGO_SCALE;

    if(bBack)
    {
        cairo_set_operator(g_Cairo, CAIRO_OPERATOR_SOURCE);
        cairo_set_source_rgba(
                g_Cairo,
                ((b >> 16) & 0xFF) / 256.0,
                ((b >> 8) & 0xFF) / 256.0,
                ((b) & 0xFF) / 256.0,
                0.8);

        // Precondition above allows this cast to be safe.
        size_t fillW = m_CellWidth > static_cast<size_t>(width) ? m_CellWidth : width;
        size_t fillH = m_CellHeight > static_cast<size_t>(height) ? m_CellHeight : height;
        cairo_rectangle(g_Cairo, x, y, fillW, fillH);
        cairo_fill(g_Cairo);
    }

    cairo_set_operator(g_Cairo, CAIRO_OPERATOR_OVER);
    cairo_set_source_rgba(
            g_Cairo,
            ((f >> 16) & 0xFF) / 256.0,
            ((f >> 8) & 0xFF) / 256.0,
            ((f) & 0xFF) / 256.0,
            1.0);

    cairo_move_to(g_Cairo, x, y);
    pango_cairo_show_layout(g_Cairo, layout);

    g_object_unref(layout);
    cairo_restore(g_Cairo);

    return width;
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
