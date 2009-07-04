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

#include "_Xterm.h"
#include "Font.h"

rgb_t g_Colours[] = { {0x00,0x00,0x00},
                      {0xB0,0x00,0x00},
                      {0x00,0xB0,0x00},
                      {0xB0,0xB0,0x00},
                      {0x00,0x00,0xB0},
                      {0xB0,0x00,0xB0},
                      {0x00,0xB0,0xB0},
                      {0xB0,0xB0,0xB0} };

rgb_t g_BrightColours[] = { {0x33,0x33,0x33},
                            {0xFF,0x33,0x33},
                            {0x33,0xFF,0x33},
                            {0xFF,0xFF,0x33},
                            {0x33,0x33,0xFF},
                            {0xFF,0x33,0xFF},
                            {0x33,0xFF,0xFF},
                            {0xFF,0xFF,0xFF} };

extern Font *g_NormalFont, *g_BoldFont;

Xterm::Xterm(rgb_t *pFramebuffer, size_t nWidth, size_t nHeight, size_t offsetLeft, size_t offsetTop, rgb_t *pBackground) :
    m_pFramebuffer(pFramebuffer), m_pBackground(pBackground),
    m_ActiveBuffer(0), m_CursorX(0), m_CursorY(0), m_FgColour(7),
    m_BgColour(0), m_Flags(0), m_ScrollStart(0), m_ScrollEnd(0),
    m_FbWidth(nWidth), m_Width(0), m_Height(0), m_OffsetLeft(offsetLeft),
    m_OffsetTop(offsetTop), m_bActive(true)
{
    m_Width = nWidth / g_NormalFont->getWidth();
    m_Height = nHeight / g_NormalFont->getHeight();

    m_ScrollEnd = m_Height-1;

    m_pAltBuffer = new rgb_t[nWidth*nHeight];

    memset(m_pAltBuffer, 0, nWidth*nHeight*3);

    m_pStatebuffer = new TermChar[m_Width*m_Height];
    m_pAltStatebuffer = new TermChar[m_Width*m_Height];

    rgb_t fg, bg;
    fg.r = 0xB0, fg.g = 0xB0; fg.b = 0xB0;
    bg.r = 0; bg.g = 0; bg.b = 0;    

    TermChar blank;
    blank.fore = fg;
    blank.back = bg;
    blank.utf32 = ' ';
    blank.flags = 0;
    for (size_t i = 0; i < m_Width*m_Height; i++)
    {
        m_pStatebuffer[i] = blank;
        m_pAltStatebuffer[i] = blank;
    }
}

Xterm::~Xterm()
{
    delete [] m_pStatebuffer;
    delete [] m_pAltStatebuffer;
    delete [] m_pAltBuffer;
}

void Xterm::write(uint32_t utf32, DirtyRectangle &rect)
{
    /*
    char str[64];
    sprintf(str, "Write: %x", utf32);
    log(str);
    */

    switch (utf32)
    {
        case '\x08':
            // Write over the cursor.
            if (m_CursorX > 0)
            {
                render(m_CursorX, m_CursorY, rect);
                m_CursorX --;
                render(m_CursorX, m_CursorY, rect, XTERM_INVERSE);
            }
            break;

        case '\n':
            // Write over the cursor.
            render(m_CursorX, m_CursorY, rect);
            cursorDownScrollIfNeeded(rect);
            m_CursorX = 0;
            render(m_CursorX, m_CursorY, rect, XTERM_INVERSE);
            break;

        case '\r':
            render(m_CursorX, m_CursorY, rect);
            m_CursorX = 0;
            render(m_CursorX, m_CursorY, rect, XTERM_INVERSE);
            break;

        default:
            set(m_CursorX, m_CursorY, utf32);
            render(m_CursorX, m_CursorY, rect);

            m_CursorX++;
            if ((m_CursorX % m_Width) == 0)
            {
                m_CursorX = 0;
                cursorDownScrollIfNeeded(rect);
            }
            render(m_CursorX, m_CursorY, rect, XTERM_INVERSE);
    }
}

void Xterm::cursorDownScrollIfNeeded(DirtyRectangle &rect)
{
    m_CursorY ++;
    if (m_CursorY > m_ScrollEnd)
    {
        m_CursorY --;

        // Scroll up one line.
        scrollUp(1, rect);
    }
}

void Xterm::scrollUp(size_t n, DirtyRectangle &rect)
{

/*
+----------+
|          |
|----------| Scroll top    ^
|----------| Top1          |
|          |     
|----------| Bottom2       ^
|----------| Scroll bottom |   
|          |  
+----------+

 */         

    size_t scrollTop_char = m_ScrollStart*m_Width;
    size_t scrollTop_px   = m_ScrollStart * m_FbWidth * g_NormalFont->getHeight() + m_OffsetTop * m_FbWidth;

    // ScrollStart and ScrollEnd are *inclusive*, so add one to make the end exclusive. 
    size_t scrollBottom_char = (m_ScrollEnd+1)*m_Width;
    size_t scrollBottom_px   = (m_ScrollEnd+1) * m_FbWidth * g_NormalFont->getHeight() + m_OffsetTop * m_FbWidth;

    size_t top1_char = (m_ScrollStart+n)*m_Width;
    size_t top1_px   = (m_ScrollStart+n) * m_FbWidth * g_NormalFont->getHeight() + m_OffsetTop * m_FbWidth;

    size_t bottom2_char = scrollBottom_char - n*m_Width;
    size_t bottom2_px   = scrollBottom_px - n*m_FbWidth*g_NormalFont->getHeight();

    rect.point(0, m_ScrollStart * g_NormalFont->getHeight() + m_OffsetTop);
    rect.point(m_FbWidth, (m_ScrollEnd+1) * g_NormalFont->getHeight() + m_OffsetTop);

//    Syscall::bitBlit(m_pFramebuffer, 

    memmove(reinterpret_cast<void*>(&m_pFramebuffer[ scrollTop_px ]),
            reinterpret_cast<void*>(&m_pFramebuffer[ top1_px ]),
            (scrollBottom_px - top1_px) * 3);
    memset (reinterpret_cast<void*>(&m_pFramebuffer[ bottom2_px ]),
            0,
            (scrollBottom_px - bottom2_px) * 3);

}

void Xterm::renderAll(DirtyRectangle &rect)
{
    rect.point(m_OffsetLeft, m_OffsetTop);
    rect.point(m_Width*g_NormalFont->getWidth()+m_OffsetLeft,
               m_Height*g_NormalFont->getHeight()+m_OffsetTop);
}

void Xterm::render(size_t x, size_t y, DirtyRectangle &rect, size_t flags)
{
    if (!m_bActive)
        return;

    TermChar c = m_pStatebuffer[y*m_Width+x];

    // If both flags and c.flags have inverse, invert the inverse by
    // unsetting it in both sets of flags.
    if ((flags & XTERM_INVERSE) && (c.flags & XTERM_INVERSE))
    {
        c.flags &= ~XTERM_INVERSE;
        flags &= ~XTERM_INVERSE;
    }

    flags = c.flags | flags;
    
    rgb_t fg = c.fore;
    rgb_t bg = c.back;

    if (flags & XTERM_INVERSE)
    {
        rgb_t tmp = fg;
        fg = bg;
        bg = tmp;
    }

    uint32_t utf32 = c.utf32;

    // Ensure the painted area is marked dirty.
    rect.point(x*g_NormalFont->getWidth()+m_OffsetLeft, y*g_NormalFont->getHeight()+m_OffsetTop);
    rect.point((x+1)*g_NormalFont->getWidth()+m_OffsetLeft+1, (y+1)*g_NormalFont->getHeight()+m_OffsetTop);

    if (flags & XTERM_BOLD)
        g_BoldFont->render(m_pFramebuffer, utf32, x*g_BoldFont->getWidth()+m_OffsetLeft,
                           y*g_BoldFont->getHeight()+m_OffsetTop, fg, bg);
    else
        g_NormalFont->render(m_pFramebuffer, utf32, x*g_NormalFont->getWidth()+m_OffsetLeft,
                             y*g_NormalFont->getHeight()+m_OffsetTop, fg, bg);

    // Blend with the background.
    for (size_t i = x*g_NormalFont->getWidth()+m_OffsetLeft; i < (x+1)*g_NormalFont->getWidth()+m_OffsetLeft; i++)
        for (size_t j = y*g_NormalFont->getHeight()+m_OffsetTop; j < (y+1)*g_NormalFont->getHeight()+m_OffsetTop; j++)
        {
            size_t idx = j*m_FbWidth+i;
            if (false /*m_pBackground*/)
                m_pFramebuffer[idx] = interpolateColour(m_pBackground[idx],
                                                        m_pFramebuffer[idx], 128);
            else
                m_pFramebuffer[idx] = m_pFramebuffer[idx];
        }
}

void Xterm::set(size_t x, size_t y, uint32_t utf32)
{
    TermChar *c = &m_pStatebuffer[y*m_Width+x];
    
    if (m_Flags & XTERM_BRIGHTFG)
        c->fore = g_BrightColours[m_FgColour];
    else
        c->fore = g_Colours[m_FgColour];

    if (m_Flags & XTERM_BRIGHTBG)
        c->back = g_BrightColours[m_BgColour];
    else
        c->back = g_Colours[m_BgColour];

    c->flags = m_Flags;
    c->utf32 = utf32;
}
