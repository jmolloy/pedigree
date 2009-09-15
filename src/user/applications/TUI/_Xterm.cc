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

#define C_BLACK   0
#define C_RED     1
#define C_GREEN   2
#define C_YELLOW  3
#define C_BLUE    4
#define C_MAGENTA 5
#define C_CYAN    6
#define C_WHITE   7

#define C_BRIGHT  8

#define DEFAULT_FG 7
#define DEFAULT_BG 0

rgb_t g_Colours[] = { {0x00,0x00,0x00},
                      {0xB0,0x22,0x22},
                      {0x22,0xB0,0x22},
                      {0xB0,0xB0,0x22},
                      {0x22,0x22,0xB0},
                      {0xB0,0x22,0xB0},
                      {0x22,0xB0,0xB0},
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

Xterm::Xterm(rgb_t *pFramebuffer, size_t nWidth, size_t nHeight, size_t offsetLeft, size_t offsetTop) :
    m_ActiveBuffer(0), m_Cmd(), m_bChangingState(false), m_bContainedBracket(false),
    m_bContainedParen(false), m_SavedX(0), m_SavedY(0)
{
    size_t width = nWidth / g_NormalFont->getWidth();
    size_t height = nHeight / g_NormalFont->getHeight();

    m_pWindows[0] = new Window(height, width, pFramebuffer, 1000, offsetLeft, offsetTop, nWidth);
    m_pWindows[1] = new Window(height, width, pFramebuffer, 1000, offsetLeft, offsetTop, nWidth);
}

Xterm::~Xterm()
{
    delete m_pWindows[0];
    delete m_pWindows[1];
}

void Xterm::write(uint32_t utf32, DirtyRectangle &rect)
{
    //char str[64];
    //sprintf(str, "XTerm: Write: %c/%x", utf32, utf32);
    //log(str);

    if(m_bChangingState)
    {
        //char tmp32[128];
        //sprintf(tmp32, "XTerm: Command '%c'\n", utf32);
        //log(tmp32);

        if(utf32 == '?') return; // Useless character.

        if (m_bContainedParen)
        {
            if (utf32 == '0')
            {
                // \e(0 -- enter alternate character mode.
                m_pWindows[m_ActiveBuffer]->setLineRenderMode(true);
            }
            else if (utf32 == 'B')
            {
                // \e(B -- exit line drawing mode.
                m_pWindows[m_ActiveBuffer]->setLineRenderMode(false);
            }
            m_bChangingState = false;
            return;
        }

        switch(utf32)
        {
            case '[':
                m_bContainedBracket = true;
                break;
            case '(':
                m_bContainedParen = true;
                break;
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                m_Cmd.params[m_Cmd.cur_param] = m_Cmd.params[m_Cmd.cur_param] * 10 + (utf32-'0');
                break;
            case ';':
                m_Cmd.cur_param++;
                break;

            case 'h':
                // 1049l is the code to switch to the alternate window.
                if (m_Cmd.params[0] == 1049)
                {
                  m_ActiveBuffer = 1;
                  m_pWindows[m_ActiveBuffer]->renderAll(rect, m_pWindows[0]);
                }
                m_bChangingState = false;
                break;
            case 'l':
                // 1049l is the code to switch back to the main window.
                if (m_Cmd.params[0] == 1049)
                {
                  m_ActiveBuffer = 0;
                  m_pWindows[m_ActiveBuffer]->renderAll(rect, m_pWindows[1]);
                }
                m_bChangingState = false;
                break;

            case 'H':
            {
                size_t x = (m_Cmd.params[1]) ? m_Cmd.params[1]-1 : 0;
                size_t y = (m_Cmd.params[0]) ? m_Cmd.params[0]-1 : 0;
                m_pWindows[m_ActiveBuffer]->setCursor(x, y, rect);
                m_bChangingState = false;
                break;
            }

            case 'A':
                m_pWindows[m_ActiveBuffer]->setCursorY(m_pWindows[m_ActiveBuffer]->getCursorY() -
                                                        ((m_Cmd.params[0]) ? m_Cmd.params[0] : 1), rect);
                m_bChangingState = false;
                break;
            case 'B':
                m_pWindows[m_ActiveBuffer]->setCursorY(m_pWindows[m_ActiveBuffer]->getCursorY() +
                                                        ((m_Cmd.params[0]) ? m_Cmd.params[0] : 1), rect);
                m_bChangingState = false;
                break;
            case 'C':
                m_pWindows[m_ActiveBuffer]->setCursorX(m_pWindows[m_ActiveBuffer]->getCursorX() +
                                                        ((m_Cmd.params[0]) ? m_Cmd.params[0] : 1), rect);
                m_bChangingState = false;
                break;


            case 'D':
                if (m_bContainedBracket)
                {
                    // If it contained a bracket, it's a cursor command.
                    m_pWindows[m_ActiveBuffer]->setCursorX(m_pWindows[m_ActiveBuffer]->getCursorX() -
                                                          ((m_Cmd.params[0]) ? m_Cmd.params[0] : 1), rect);
                }
                else
                {
                    // Else, it's a scroll downwards command.
                    m_pWindows[m_ActiveBuffer]->scrollDown(1, rect);
                }
                m_bChangingState = false;
                break;

            case 'd':
                // Absolute row reference. (XTERM)
                m_pWindows[m_ActiveBuffer]->setCursorY((m_Cmd.params[0]) ? m_Cmd.params[0]-1 : 0, rect);
                m_bChangingState = false;
                break;

            case 'G':
                // Absolute column reference. (XTERM)
                m_pWindows[m_ActiveBuffer]->setCursorX((m_Cmd.params[0]) ? m_Cmd.params[0]-1 : 0, rect);
                m_bChangingState = false;
                break;

            case 'M':
            case 'T':
            {
                size_t nScrollLines = (m_Cmd.params[0]) ? m_Cmd.params[0] : 1;
                m_pWindows[m_ActiveBuffer]->scrollUp(nScrollLines, rect);
                m_bChangingState = false;
                break;
            }
            case 'S':
            {
                size_t nScrollLines = (m_Cmd.params[0]) ? m_Cmd.params[0] : 1;
                m_pWindows[m_ActiveBuffer]->scrollDown(nScrollLines, rect);
                m_bChangingState = false;
                break;
            }

            case 'P':
                 m_pWindows[m_ActiveBuffer]->deleteCharacters((m_Cmd.params[0]) ? m_Cmd.params[0] : 1, rect);
                m_bChangingState = false;
                break;
            case 'J':
                switch (m_Cmd.params[0])
                {
                    case 0: // Erase down.
                         m_pWindows[m_ActiveBuffer]->eraseDown(rect);
                        break;
                    case 1: // Erase up.
                        m_pWindows[m_ActiveBuffer]->eraseUp(rect);
                        break;
                    case 2: // Erase entire screen and move to home.
                        m_pWindows[m_ActiveBuffer]->eraseScreen(rect);
                        break;
                }
                m_bChangingState = false;
                break;
            case 'K':
                switch (m_Cmd.params[0])
                {
                    case 0: // Erase end of line.
                        m_pWindows[m_ActiveBuffer]->eraseEOL(rect);
                        break;
                    case 1: // Erase start of line.
                        m_pWindows[m_ActiveBuffer]->eraseSOL(rect);
                        break;
                    case 2: // Erase entire line.
                        m_pWindows[m_ActiveBuffer]->eraseLine(rect);
                        break;
                }
                m_bChangingState = false;
                break;
            case 'X': // Erase characters (XTERM)
                 m_pWindows[m_ActiveBuffer]->eraseChars((m_Cmd.params[0]) ? m_Cmd.params[0]-1 : 1, rect);
                m_bChangingState = false;
                break;

            case 'r':
                m_pWindows[m_ActiveBuffer]->setScrollRegion((m_Cmd.params[0]) ? m_Cmd.params[0]-1 : ~0,
                                                            (m_Cmd.params[1]) ? m_Cmd.params[1]-1 : ~0);
                m_bChangingState = false;
                break;

            case 'm':
            {
                // Colours, yay!
                for (int i = 0; i < m_Cmd.cur_param+1; i++)
                {
                    switch (m_Cmd.params[i])
                    {
                        case 0:
                        {
                            // Reset all attributes.
                            m_pWindows[m_ActiveBuffer]->setFlags(0);
                            m_pWindows[m_ActiveBuffer]->setForeColour(DEFAULT_FG);
                            m_pWindows[m_ActiveBuffer]->setBackColour(DEFAULT_BG);
                            break;
                        }
                        case 1:
                        {
                            // Bold
                            uint8_t flags = m_pWindows[m_ActiveBuffer]->getFlags();
                            if(!(flags & XTERM_BOLD))
                            {
                                flags |= XTERM_BOLD;
                                m_pWindows[m_ActiveBuffer]->setFlags(flags);
                            }
                            break;
                        }
                        case 7:
                        {
                            // Inverse
                            uint8_t flags = m_pWindows[m_ActiveBuffer]->getFlags();
                            if(flags & XTERM_INVERSE)
                                flags &= ~XTERM_INVERSE;
                            else
                                flags |= XTERM_INVERSE;
                            m_pWindows[m_ActiveBuffer]->setFlags(flags);
                            break;
                        }
                        case 30:
                        case 31:
                        case 32:
                        case 33:
                        case 34:
                        case 35:
                        case 36:
                        case 37:
                            // Foreground.
                            m_pWindows[m_ActiveBuffer]->setForeColour(m_Cmd.params[i]-30);
                            break;
                        case 40:
                        case 41:
                        case 42:
                        case 43:
                        case 44:
                        case 45:
                        case 46:
                        case 47:
                            // Background.
                            m_pWindows[m_ActiveBuffer]->setBackColour(m_Cmd.params[i]-40);
                            break;

                        default:
                            // Do nothing.
                            break;
                    }
                }
                m_bChangingState = false;
                break;
            }
            case '\e':
                // We received another VT100 command while expecting a terminating command - this must mean it's one of \e7 or \e8.
                switch (m_Cmd.params[0])
                {
                    case 7: // Save cursor.
                        m_SavedX = m_pWindows[m_ActiveBuffer]->getCursorX();
                        m_SavedY = m_pWindows[m_ActiveBuffer]->getCursorY();
                        m_Cmd.cur_param = 0; m_Cmd.params[0] = 0;
                        break;
                    case 8: // Restore cursor.
                        m_pWindows[m_ActiveBuffer]->setCursor(m_SavedX, m_SavedY, rect);
                        m_Cmd.cur_param = 0; m_Cmd.params[0] = 0;
                        break;
                };
                // We're still changing state, so keep m_bChangingState = true.
                break;

            default:
            {
                char tmp[128];
                sprintf(tmp, "Unhandled XTerm escape code: %d/%c.", utf32, utf32);
                log(tmp);

                m_bChangingState = false;
            }
            break;
        };
    }
    else
    {
        switch (utf32)
        {
            case '\x08':
                m_pWindows[m_ActiveBuffer]->cursorLeft(rect);
                break;

            case '\n':
                m_pWindows[m_ActiveBuffer]->cursorDownAndLeftToMargin(rect);
                break;

            case '\r':
                m_pWindows[m_ActiveBuffer]->cursorLeftToMargin(rect);
                break;

            // VT100 - command start
            case '\e':
                m_bChangingState = true;
                m_bContainedBracket = false;
                m_bContainedParen = false;
                m_Cmd.cur_param = 0;
                m_Cmd.params[0] = 0;
                m_Cmd.params[1] = 0;
                m_Cmd.params[2] = 0;
                m_Cmd.params[3] = 0;
                break;

            default:
                // Line drawing
                if(m_pWindows[m_ActiveBuffer]->getLineRenderMode())
                {
                    switch (utf32)
                    {
                        case 'j': utf32 = 0x2518; break; // Lower right corner
                        case 'k': utf32 = 0x2510; break; // Upper right corner
                        case 'l': utf32 = 0x250c; break; // Upper left corner
                        case 'm': utf32 = 0x2514; break; // Lower left corner
                        case 'n': utf32 = 0x253c; break; // Crossing lines.
                        case 'q': utf32 = 0x2500; break; // Horizontal line.
                        case 't': utf32 = 0x251c; break; // Left 'T'
                        case 'u': utf32 = 0x2524; break; // Right 'T'
                        case 'v': utf32 = 0x2534; break; // Bottom 'T'
                        case 'w': utf32 = 0x252c; break; // Top 'T'
                        case 'x': utf32 = 0x2502; break; // Vertical bar
                        default:
                        ;
                    }
                }
                if (utf32 >= 32)
                    m_pWindows[m_ActiveBuffer]->addChar(utf32, rect);
        }
    }
}

Xterm::Window::Window(size_t nRows, size_t nCols, rgb_t *pFb, size_t nMaxScrollback, size_t offsetLeft, size_t offsetTop, size_t fbWidth) :
    m_pBuffer(0), m_BufferLength(nRows*nCols), m_pFramebuffer(pFb), m_FbWidth(fbWidth), m_Width(nCols), m_Height(nRows), m_OffsetLeft(offsetLeft), m_OffsetTop(offsetTop), m_nMaxScrollback(nMaxScrollback), m_CursorX(0), m_CursorY(0), m_ScrollStart(0), m_ScrollEnd(nRows-1),
    m_pInsert(0), m_pView(0), m_Fg(DEFAULT_FG), m_Bg(DEFAULT_BG), m_Flags(0), m_bLineRender(false)
{
    // Using malloc() instead of new[] so we can use realloc()
    m_pBuffer = reinterpret_cast<TermChar*>(malloc(m_Width*m_Height*sizeof(TermChar)));

    TermChar blank;
    blank.fore = m_Fg;
    blank.back = m_Bg;
    blank.utf32 = ' ';
    blank.flags = 0;
    for (size_t i = 0; i < m_Width*m_Height; i++)
        m_pBuffer[i] = blank;

    if(m_Bg)
        Syscall::fillRect(m_pFramebuffer, 0, m_OffsetTop, m_FbWidth, nRows*g_NormalFont->getHeight(), g_Colours[m_Bg]);

    m_pInsert = m_pView = m_pBuffer;
}

Xterm::Window::~Window()
{
    free(m_pBuffer);
}

void Xterm::Window::showCursor(DirtyRectangle &rect)
{
    render(rect, XTERM_INVERSE);
}

void Xterm::Window::hideCursor(DirtyRectangle &rect)
{
    render(rect);
}

void Xterm::Window::resize(size_t nWidth, size_t nHeight, rgb_t *pBuffer)
{
    log("here3");
    size_t cols = nWidth / g_NormalFont->getWidth();
    size_t rows = nHeight / g_NormalFont->getHeight();

    g_NormalFont->setWidth(nWidth);
    g_BoldFont->setWidth(nWidth);

    TermChar *newBuf = reinterpret_cast<TermChar*>(malloc(cols*rows*sizeof(TermChar)));

    TermChar blank;
    blank.fore = m_Fg;
    blank.back = m_Bg;
    blank.utf32 = ' ';
    blank.flags = 0;
    for (size_t i = 0; i < cols*rows; i++)
        newBuf[i] = blank;
    log("here4");
    if(m_Bg)
        Syscall::fillRect(pBuffer, 0, m_OffsetTop, nWidth, rows*g_NormalFont->getHeight(), g_Colours[m_Bg]);

    for (size_t r = 0; r < m_Height; r++)
    {
        if (r >= rows) break;
        for (size_t c = 0; c < m_Width; c++)
        {
            if (c >= cols) break;

            newBuf[r*cols + c] = m_pInsert[r*m_Width + c];
        }
    }
    log("here5");
    free(m_pBuffer);
    log("here6");
    m_pInsert = m_pView = m_pBuffer = newBuf;
    m_BufferLength = rows*cols;

    m_pFramebuffer = pBuffer;
    m_FbWidth = nWidth;
    m_Width = cols;
    m_Height = rows;
    m_ScrollStart = 0;
    m_ScrollEnd = rows-1;
    if (m_CursorX >= m_Width)
        m_CursorX = 0;
    if (m_CursorY >= m_Height)
        m_CursorY = 0;
    log("here6");
    DirtyRectangle rect;
    renderAll(rect, 0);
    log("here7");
    Syscall::updateBuffer(m_pFramebuffer, rect);
    log("here8");
}

void Xterm::Window::setScrollRegion(int start, int end)
{
    if (start == -1)
        start = 0;
    if (end == -1)
        end = m_Height-1;
    m_ScrollStart = start;
    m_ScrollEnd = end;
}

void Xterm::Window::setForeColour(uint8_t fgColour)
{
    m_Fg = fgColour;
}

void Xterm::Window::setBackColour(uint8_t bgColour)
{
    m_Bg = bgColour;
}

void Xterm::Window::setFlags(uint8_t flags)
{
    m_Flags = flags;
}

uint8_t Xterm::Window::getFlags()
{
    return m_Flags;
}

void Xterm::Window::renderAll(DirtyRectangle &rect, Xterm::Window *pPrevious)
{
    TermChar *pOld = (pPrevious) ? pPrevious->m_pView : 0;
    TermChar *pNew = m_pView;

    // "Cleverer" full redraw - only redraw those glyphs that are different from the previous window.
    for (size_t y = 0; y < m_Height; y++)
    {
        for (size_t x = 0; x < m_Width; x++)
        {
            if (!pOld || (pOld[y*m_Width+x] != pNew[y*m_Width+x]))
                render(rect, 0, x, y);
        }
    }

}

void Xterm::Window::setChar(uint32_t utf32, size_t x, size_t y)
{
    TermChar *c = &m_pInsert[y*m_Width+x];

    c->fore = m_Fg;
    c->back = m_Bg;

    c->flags = m_Flags;
    c->utf32 = utf32;
}

Xterm::Window::TermChar Xterm::Window::getChar(size_t x, size_t y)
{
    if (x == ~0UL) x = m_CursorX;
    if (y == ~0UL) y = m_CursorY;
    return m_pInsert[y*m_Width+x];
}

void Xterm::Window::cursorDown(size_t n, DirtyRectangle &rect)
{
    m_CursorY += n;
    if (m_CursorY > m_ScrollEnd)
    {
        size_t amount = m_CursorY-m_ScrollEnd;
        if (m_ScrollStart == 0 && m_ScrollEnd == m_Height-1)
        {
            scrollScreenUp(amount, rect);
        }
        else
        {
            scrollRegionUp(amount, rect);
        }
        m_CursorY = m_ScrollEnd;
    }
}

void Xterm::Window::render(DirtyRectangle &rect, size_t flags, size_t x, size_t y)
{
    if (x == ~0UL) x = m_CursorX;
    if (y == ~0UL) y = m_CursorY;

    TermChar c = m_pView[y*m_Width+x];

    // If both flags and c.flags have inverse, invert the inverse by
    // unsetting it in both sets of flags.
    if ((flags & XTERM_INVERSE) && (c.flags & XTERM_INVERSE))
    {
        c.flags &= ~XTERM_INVERSE;
        flags &= ~XTERM_INVERSE;
    }

    flags = c.flags | flags;

    rgb_t fg = g_Colours[c.fore];
    if (flags & XTERM_BRIGHTFG)
        fg = g_BrightColours[c.fore];
    rgb_t bg = g_Colours[c.back];
    if (flags & XTERM_BRIGHTBG)
        bg = g_BrightColours[c.fore];

    if (flags & XTERM_INVERSE)
    {
        rgb_t tmp = fg;
        fg = bg;
        bg = tmp;
    }

    uint32_t utf32 = c.utf32;
    // char str[64];
    // sprintf(str, "Render(%d,%d,%d,%d)", m_OffsetLeft, m_OffsetTop, x, y);
    // log(str);
    // Ensure the painted area is marked dirty.
    rect.point(x*g_NormalFont->getWidth()+m_OffsetLeft, y*g_NormalFont->getHeight()+m_OffsetTop);
    rect.point((x+1)*g_NormalFont->getWidth()+m_OffsetLeft+1, (y+1)*g_NormalFont->getHeight()+m_OffsetTop);

    if (flags & XTERM_BOLD)
        g_BoldFont->render(m_pFramebuffer, utf32, x*g_BoldFont->getWidth()+m_OffsetLeft,
                           y*g_BoldFont->getHeight()+m_OffsetTop, fg, bg);
    else
        g_NormalFont->render(m_pFramebuffer, utf32, x*g_NormalFont->getWidth()+m_OffsetLeft,
                             y*g_NormalFont->getHeight()+m_OffsetTop, fg, bg);
}

void Xterm::Window::scrollRegionUp(size_t n, DirtyRectangle &rect)
{

    /*
    +----------+
    |          |
    |----------| Top2       ^
    |----------| Top1       |
    |          |
    |----------| Bottom2    ^
    |----------| Bottom1    |
    |          |
    +----------+

    */

    size_t top1_y   = (m_ScrollStart+n) * g_NormalFont->getHeight() + m_OffsetTop;
    size_t top2_y   = m_ScrollStart * g_NormalFont->getHeight() + m_OffsetTop;

    // ScrollStart and ScrollEnd are *inclusive*, so add one to make the end exclusive.
    size_t bottom1_y   = (m_ScrollEnd+1) * g_NormalFont->getHeight() + m_OffsetTop;
    size_t bottom2_y   = (m_ScrollEnd+1-n) * g_NormalFont->getHeight() + m_OffsetTop;

    rect.point(0, top2_y);
    rect.point(m_FbWidth, bottom1_y);

    // If we're bitblitting, we need to commit all changes before now.
    Syscall::updateBuffer(m_pFramebuffer, rect);
    rect.reset();log("blit0");
    Syscall::bitBlit(m_pFramebuffer, 0, top1_y, 0, top2_y, m_FbWidth, bottom2_y-top2_y);log("blit1");
    Syscall::fillRect(m_pFramebuffer, 0, bottom2_y, m_FbWidth, bottom1_y-bottom2_y, g_Colours[m_Bg]);

    memmove(&m_pBuffer[m_ScrollStart*m_Width], &m_pBuffer[(m_ScrollStart+n)*m_Width], (m_ScrollEnd+1-n-m_ScrollStart)*m_Width*sizeof(TermChar));

    TermChar blank;
    blank.fore = m_Fg;
    blank.back = m_Bg;
    blank.utf32 = ' ';
    blank.flags = 0;
    for (size_t i = m_Width*(m_ScrollEnd+1-n); i < m_Width*(m_ScrollEnd+1); i++)
        m_pBuffer[i] = blank;
}

void Xterm::Window::scrollRegionDown(size_t n, DirtyRectangle &rect)
{

    /*
    +----------+
    |          |
    |----------| Top1       |
    |----------| Top2       v
    |          |
    |----------| Bottom1    |
    |----------| Bottom2    v
    |          |
    +----------+

    */

    size_t top1_y   = m_ScrollStart * g_NormalFont->getHeight() + m_OffsetTop;
    size_t top2_y   = (m_ScrollStart+n) * g_NormalFont->getHeight() + m_OffsetTop;

    // ScrollStart and ScrollEnd are *inclusive*, so add one to make the end exclusive.
    size_t bottom1_y   = (m_ScrollEnd+1-n) * g_NormalFont->getHeight() + m_OffsetTop;
    size_t bottom2_y   = (m_ScrollEnd+1) * g_NormalFont->getHeight() + m_OffsetTop;

    rect.point(0, top1_y);
    rect.point(m_FbWidth, bottom2_y);

    // If we're bitblitting, we need to commit all changes before now.
    Syscall::updateBuffer(m_pFramebuffer, rect);
    rect.reset();log("blit2");
    Syscall::bitBlit(m_pFramebuffer, 0, top1_y, 0, top2_y, m_FbWidth, bottom2_y-top2_y);log("blit3");
    Syscall::fillRect(m_pFramebuffer, 0, top1_y, m_FbWidth, top2_y-top1_y, g_Colours[m_Bg]);

    memmove(&m_pBuffer[m_ScrollStart*m_Width], &m_pBuffer[(m_ScrollStart+n)*m_Width], (m_ScrollEnd+1-n-m_ScrollStart)*m_Width*sizeof(TermChar));

    TermChar blank;
    blank.fore = m_Fg;
    blank.back = m_Bg;
    blank.utf32 = ' ';
    blank.flags = 0;
    for (size_t i = m_Width*m_ScrollStart; i < m_Width*(m_ScrollStart+n); i++)
        m_pBuffer[i] = blank;
}

void Xterm::Window::scrollScreenUp(size_t n, DirtyRectangle &rect)
{
    size_t top_px   = m_OffsetTop;
    size_t top2_px   = m_OffsetTop+ n*g_NormalFont->getHeight();

    size_t bottom2_px = top_px + (m_Height-n)*g_NormalFont->getHeight();

    // If we're bitblitting, we need to commit all changes before now.
    Syscall::updateBuffer(m_pFramebuffer, rect);
    rect.reset();log("blit4");
    Syscall::bitBlit(m_pFramebuffer, 0, top2_px, 0, top_px, m_FbWidth, (m_Height-n)*g_NormalFont->getHeight());log("blit5");
    Syscall::fillRect(m_pFramebuffer, 0, bottom2_px, m_FbWidth, n*g_NormalFont->getHeight(), g_Colours[m_Bg]);

    if (m_pView == m_pInsert)
        m_pView += n*m_Width;

    m_pInsert += n*m_Width;

    // Have we gone off the end of the scroll area?
    if (m_pInsert + m_Height*m_Width >= (m_pBuffer+m_BufferLength))
    {
        // Either expand the buffer (if the length is less than the scrollback size) or memmove data up a bit.
        if (m_BufferLength < m_nMaxScrollback*m_Width)
        {
            // Expand.
            size_t amount = m_Width*m_Height;
            if (amount+m_BufferLength > m_nMaxScrollback*m_Width)
                amount = (m_nMaxScrollback*m_Width)-m_BufferLength;

            uintptr_t pInsOffset  = reinterpret_cast<uintptr_t>(m_pInsert) - reinterpret_cast<uintptr_t>(m_pBuffer);
            uintptr_t pViewOffset = reinterpret_cast<uintptr_t>(m_pView) - reinterpret_cast<uintptr_t>(m_pBuffer);

            m_pBuffer = reinterpret_cast<TermChar*>(realloc(m_pBuffer, (amount+m_BufferLength)*sizeof(TermChar)));

            m_pInsert = reinterpret_cast<TermChar*>(reinterpret_cast<uintptr_t>(m_pBuffer) + pInsOffset);
            m_pView   = reinterpret_cast<TermChar*>( reinterpret_cast<uintptr_t>(m_pBuffer) + pViewOffset);

            TermChar blank;
            blank.fore = m_Fg;
            blank.back = m_Bg;
            blank.utf32 = ' ';
            blank.flags = 0;
            for (size_t i = m_BufferLength; i < m_BufferLength+amount; i++)
                m_pBuffer[i] = blank;

            m_BufferLength += amount;
        }
        else
        {
            memmove(&m_pBuffer[m_Width*m_Height],
                    &m_pBuffer[0],
                    m_Width*m_Height*sizeof(TermChar));
            TermChar blank;
            blank.fore = m_Fg;
            blank.back = m_Bg;
            blank.utf32 = ' ';
            blank.flags = 0;
            for (size_t i = m_BufferLength-m_Width*m_Height; i < m_BufferLength; i++)
                m_pBuffer[i] = blank;
        }
    }
}

void Xterm::Window::setCursor(size_t x, size_t y, DirtyRectangle &rect)
{
    m_CursorX = x;
    m_CursorY = y;
}

void Xterm::Window::setCursorX(size_t x, DirtyRectangle &rect)
{
    setCursor(x, m_CursorY, rect);
}
void Xterm::Window::setCursorY(size_t y, DirtyRectangle &rect)
{
    setCursor(m_CursorX, y, rect);
}

size_t Xterm::Window::getCursorX()
{
    return m_CursorX;
}
size_t Xterm::Window::getCursorY()
{
    return m_CursorY;
}

void Xterm::Window::cursorLeft(DirtyRectangle &rect)
{
    if (m_CursorX == 0) return;
    m_CursorX --;
}

void Xterm::Window::cursorLeftNum(size_t n, DirtyRectangle &rect)
{
    if (m_CursorX == 0) return;

    if(n > m_CursorX) n = m_CursorX;

    m_CursorX -= n;
}

void Xterm::Window::cursorDownAndLeftToMargin(DirtyRectangle &rect)
{
    cursorDown(1, rect);
    m_CursorX = 0;
}

void Xterm::Window::cursorLeftToMargin(DirtyRectangle &rect)
{
    m_CursorX = 0;
}

void Xterm::Window::addChar(uint32_t utf32, DirtyRectangle &rect)
{
    TermChar tc = getChar();
    setChar(utf32, m_CursorX, m_CursorY);
    if (getChar() != tc)
        render(rect);

    m_CursorX++;
    if ((m_CursorX % m_Width) == 0)
    {
        m_CursorX = 0;
        cursorDown(1, rect);
    }
}

void Xterm::Window::scrollUp(size_t n, DirtyRectangle &rect)
{
    scrollRegionDown(n, rect);
}

void Xterm::Window::scrollDown(size_t n, DirtyRectangle &rect)
{
    scrollRegionUp(n, rect);
}

void Xterm::Window::eraseScreen(DirtyRectangle &rect)
{
    // One good fillRect should do the job nicely.
    Syscall::fillRect(m_pFramebuffer, m_OffsetLeft, m_OffsetTop, m_FbWidth, m_Height*g_NormalFont->getHeight(), g_Colours[m_Bg]);

    for(size_t row = 0; row < m_Height; row++)
    {
        for(size_t col = 0; col < m_Width; col++)
        {
            setChar(' ', col, row);
        }
    }
}

void Xterm::Window::eraseEOL(DirtyRectangle &rect)
{
    size_t l = m_OffsetLeft+m_CursorX*g_NormalFont->getWidth();

    // Again, one fillRect should do it.
    Syscall::fillRect(m_pFramebuffer, l, m_OffsetTop+m_CursorY*g_NormalFont->getHeight(), m_FbWidth-l, g_NormalFont->getHeight(), g_Colours[m_Bg]);

    size_t row = m_CursorY;
    for(size_t col = m_CursorX; col < m_Width; col++)
    {
        setChar(' ', col, row);
    }
}

void Xterm::Window::eraseSOL(DirtyRectangle &rect)
{

    int32_t row = static_cast<int32_t>(m_CursorY);
    for(int32_t col = static_cast<int32_t>(m_CursorX); col >= 0; col--)
    {
        g_NormalFont->render(m_pFramebuffer,
                             static_cast<uint32_t>(' '),
                             (col * g_NormalFont->getWidth()) + m_OffsetLeft,
                             (row * g_NormalFont->getHeight()) + m_OffsetTop,
                             g_Colours[m_Fg],
                             g_Colours[m_Bg]
        );
    }
    rect.point(m_OffsetLeft, (m_CursorY * g_NormalFont->getHeight()) + m_OffsetTop);
    rect.point((m_CursorX * g_NormalFont->getWidth()) + m_OffsetLeft, (m_CursorY * g_NormalFont->getHeight()) + m_OffsetTop);
}

void Xterm::Window::eraseLine(DirtyRectangle &rect)
{

    size_t row = m_CursorY;
    for(size_t col = 0; col < m_CursorX; col++)
    {
        g_NormalFont->render(m_pFramebuffer,
                             static_cast<uint32_t>(' '),
                             (col * g_NormalFont->getWidth()) + m_OffsetLeft,
                             (row * g_NormalFont->getHeight()) + m_OffsetTop,
                             g_Colours[m_Fg],
                             g_Colours[m_Bg]
        );
    }
    rect.point(m_OffsetLeft, (m_CursorY * g_NormalFont->getHeight()) + m_OffsetTop);
    rect.point((m_Width * g_NormalFont->getWidth()) + m_OffsetLeft, (m_CursorY * g_NormalFont->getHeight()) + m_OffsetTop);
}

void Xterm::Window::eraseChars(size_t n, DirtyRectangle &rect)
{

    size_t row = m_CursorY;
    for(size_t col = m_CursorX; col < (m_CursorX + n); col++)
    {
        g_NormalFont->render(m_pFramebuffer,
                             static_cast<uint32_t>(' '),
                             (col * g_NormalFont->getWidth()) + m_OffsetLeft,
                             (row * g_NormalFont->getHeight()) + m_OffsetTop,
                             g_Colours[m_Fg],
                             g_Colours[m_Bg]
        );
    }
    rect.point((m_CursorX * g_NormalFont->getWidth()) + m_OffsetLeft, (m_CursorY * g_NormalFont->getHeight()) + m_OffsetTop);
    rect.point(((m_CursorX + n) * g_NormalFont->getWidth()) + m_OffsetLeft, (m_CursorY * g_NormalFont->getHeight()) + m_OffsetTop);
}

void Xterm::Window::eraseUp(DirtyRectangle &rect)
{

    for(int32_t row = static_cast<int32_t>(m_CursorY); row >= 0; row--)
    {
        for(size_t col = 0; col < m_Width; col++)
        {
            g_NormalFont->render(m_pFramebuffer,
                                 static_cast<uint32_t>(' '),
                                 (col * g_NormalFont->getWidth()) + m_OffsetLeft,
                                 (row * g_NormalFont->getHeight()) + m_OffsetTop,
                                 g_Colours[m_Fg],
                                 g_Colours[m_Bg]
            );
        }
    }
    rect.point(m_OffsetLeft, m_OffsetTop);
    rect.point((m_Width * g_NormalFont->getWidth()) + m_OffsetLeft, (m_CursorY * g_NormalFont->getHeight()) + m_OffsetTop);
}

void Xterm::Window::eraseDown(DirtyRectangle &rect)
{

    for(size_t row = m_CursorY; row < m_Height; row++)
    {
        for(size_t col = 0; col < m_Width; col++)
        {
            g_NormalFont->render(m_pFramebuffer,
                                 static_cast<uint32_t>(' '),
                                 (col * g_NormalFont->getWidth()) + m_OffsetLeft,
                                 (row * g_NormalFont->getHeight()) + m_OffsetTop,
                                 g_Colours[m_Fg],
                                 g_Colours[m_Bg]
            );
        }
    }
    rect.point(m_OffsetLeft, (m_CursorY * g_NormalFont->getHeight()) + m_OffsetTop);
    rect.point((m_Width * g_NormalFont->getWidth()) + m_OffsetLeft, (m_Height * g_NormalFont->getHeight()) + m_OffsetTop);
}

void Xterm::Window::deleteCharacters(size_t n, DirtyRectangle &rect)
{

    // Start of the delete region
    size_t deleteStart = m_CursorX;

    // End of the delete region
    size_t deleteEnd = (deleteStart + n);

    // Number of characters to shift
    size_t numChars = m_Width - deleteEnd;

    // Shift all the characters across
    size_t row = m_CursorY;
    for(size_t off = 0; off < numChars; off++)
    {
        // Positions for each side of the copy
        size_t destX = deleteStart + off;
        size_t srcX = deleteEnd + off;

        // Move the character
        TermChar c = m_pView[(row * m_Width) + srcX];
        m_pView[(row * m_Width) + destX] = c;

        // Render to the framebuffer
        if(c.flags & XTERM_INVERSE)
        {
            g_NormalFont->render(m_pFramebuffer,
                                 c.utf32,
                                 (destX * g_NormalFont->getWidth()) + m_OffsetLeft,
                                 (row * g_NormalFont->getHeight()) + m_OffsetTop,
                                 g_Colours[c.back],
                                 g_Colours[c.fore]
            );
        }
        else
        {
            g_NormalFont->render(m_pFramebuffer,
                                 c.utf32,
                                 (destX * g_NormalFont->getWidth()) + m_OffsetLeft,
                                 (row * g_NormalFont->getHeight()) + m_OffsetTop,
                                 g_Colours[c.fore],
                                 g_Colours[c.back]
            );
        }
    }

    // Remove the last cells that weren't affected
    for(size_t i = (m_Width - n); i < m_Width; i++)
    {
        // Blank this cell
        TermChar c;
        c.utf32 = static_cast<uint32_t>(' ');
        c.back = c.fore = c.flags = 0;
        m_pView[(row * m_Width) + i] = c;

        // And then render to the framebuffer
        g_NormalFont->render(m_pFramebuffer,
                             c.utf32,
                             (i * g_NormalFont->getWidth()) + m_OffsetLeft,
                             (row * g_NormalFont->getHeight()) + m_OffsetTop,
                             g_Colours[c.fore],
                             g_Colours[c.back]
        );
    }

    rect.point((deleteStart * g_NormalFont->getWidth()) + m_OffsetLeft, (m_CursorY * g_NormalFont->getHeight()) + m_OffsetTop);
    rect.point((m_Width * g_NormalFont->getWidth()) + m_OffsetLeft, (m_CursorY * g_NormalFont->getHeight()) + m_OffsetTop);
}
