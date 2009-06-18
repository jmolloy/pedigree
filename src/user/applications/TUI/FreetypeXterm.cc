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

#include "FreetypeXterm.h"

extern void log(char*);

int g_RequestedFontHeight = 12;

FreetypeXterm::FreetypeXterm(Display::ScreenMode mode, void *pFramebuffer) :
    Xterm(mode, pFramebuffer)
{
    for (int i = 0; i < 0x10000; i++)
    {
        m_Cache[i].buffer = 0;
    }

    char str[64];
    int error = FT_Init_FreeType(&m_Library);
    if (error)
    {
        sprintf(str, "Freetype init error: %d\n", error);
        log(str);
        return;
    }

    error = FT_New_Face(m_Library, "/system/fonts/DejaVuSansMono.ttf", 0,
                        &m_NormalFont);
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

    error = FT_Set_Pixel_Sizes(m_NormalFont, 0, g_RequestedFontHeight); 
    if (error)
    {
        sprintf(str, "Freetype set pixel size error: %d\n", error);
        log(str);
        return;
    }

    error = FT_Load_Char(m_NormalFont, 'a', FT_LOAD_RENDER); 

    m_FontWidth  = m_NormalFont->glyph->advance.x >> 6;
    m_FontHeight = m_NormalFont->size->metrics.y_ppem;

    m_nWidth  = mode.width/m_FontWidth;
    m_nHeight = mode.height/m_FontHeight;

    // Delete the windows the Xterm constructor made.
    delete m_pWindows[0];
    delete m_pWindows[1];
    // Make new ones with the new width/height.
    m_pWindows[0] = new Window(m_nWidth, m_nHeight, this);
    m_pWindows[1] = new Window(m_nWidth, m_nHeight, this);
}

FreetypeXterm::~FreetypeXterm()
{
}

void FreetypeXterm::putCharFb(uint32_t c, int x, int y, uint32_t f, uint32_t b)
{
    char str[64];
    if (c < 0x10000)
    {
        if (!m_Cache[c].buffer)
        {
            int error = FT_Load_Char(m_NormalFont, c, FT_LOAD_RENDER); 
            if (error)
            {
                sprintf(str, "Freetype load char error: %d\n", error);
                log(str);
                return;
            }
            m_Cache[c].rows  = m_NormalFont->glyph->bitmap.rows;
            m_Cache[c].width = m_NormalFont->glyph->bitmap.width;
            m_Cache[c].pitch = m_NormalFont->glyph->bitmap.pitch;
            m_Cache[c].left  = m_NormalFont->glyph->bitmap_left;
            m_Cache[c].top   = m_NormalFont->glyph->bitmap_top;

            m_Cache[c].buffer = reinterpret_cast<uint8_t*>(malloc(m_Cache[c].rows*m_Cache[c].pitch));
            memcpy(m_Cache[c].buffer, m_NormalFont->glyph->bitmap.buffer, m_Cache[c].rows*m_Cache[c].pitch);
        } 
        drawBitmap(&m_Cache[c],
                   x*m_FontWidth+m_Cache[c].left,
                   (y+1)*m_FontHeight-m_Cache[c].top,
                   f, b);
    }
    else
    {
        int error = FT_Load_Char(m_NormalFont, c, FT_LOAD_RENDER); 
        if (error)
        {
            sprintf(str, "Freetype load char error: %d\n", error);
            log(str);
            return;
        }
        Glyph g;
        g.rows   = m_NormalFont->glyph->bitmap.rows;
        g.width  = m_NormalFont->glyph->bitmap.width;
        g.pitch  = m_NormalFont->glyph->bitmap.pitch;
        g.left   = m_NormalFont->glyph->bitmap_left;
        g.top    = m_NormalFont->glyph->bitmap_top;
        g.buffer = m_NormalFont->glyph->bitmap.buffer;

        drawBitmap(&g,
                   x*m_FontWidth+g.left,
                   (y+1)*m_FontHeight-g.top,
                   f, b);
    }
        
}

void FreetypeXterm::drawBitmap(Glyph *bitmap, int left, int top, uint32_t f, uint32_t b)
{
    int depth = m_Mode.pf.nBpp;
    int m_Stride = m_Mode.pf.nPitch/ (depth/8);

    uint16_t *p16Fb = reinterpret_cast<uint16_t*> (m_pFramebuffer);
    uint32_t *p32Fb = reinterpret_cast<uint32_t*> (m_pFramebuffer);

    for (int i = 0; i < bitmap->rows; i++)
    {
        for (int j = 0; j < bitmap->width; j++)
        {
            uint8_t graylevel = bitmap->buffer[i*bitmap->pitch+j];
            uint32_t c = interpolateColour(f, b, graylevel);
            p16Fb[(top+i)*m_Stride + (left+j)] = c;
        }
    }
}

uint32_t FreetypeXterm::interpolateColour(uint32_t f, uint32_t b, uint8_t a)
{
    Display::PixelFormat pf = m_Mode.pf;

    // Extract the red field.
    uint8_t range = 1 << pf.mRed;
    uint8_t fr = (f >> pf.pRed) & (range-1);
    uint8_t br = (b >> pf.pRed) & (range-1);

    uint8_t r = (fr * a + br * (256-a)) / 256;

    // Green
    range = 1 << pf.mGreen;
    uint8_t fg = (f >> pf.pGreen) & (range-1);
    uint8_t bg = (b >> pf.pGreen) & (range-1);

    uint8_t g = (fg * a + bg * (256-a)) / 256;

    // Blue
    range = 1 << pf.mBlue;
    uint8_t fb = (f >> pf.pBlue) & (range-1);
    uint8_t bb = (b >> pf.pBlue) & (range-1);

    uint8_t bl = (fb * a + bb * (256-a)) / 256;

    return 0 |
         (static_cast<uint32_t>(r) << pf.pRed) |
         (static_cast<uint32_t>(g) << pf.pGreen) |
         (static_cast<uint32_t>(bl) << pf.pBlue);
}
