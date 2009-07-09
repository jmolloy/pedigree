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
#ifndef XTERM_H
#define XTERM_H

#include "environment.h"

#define XTERM_BOLD      0x1
#define XTERM_UNDERLINE 0x2
#define XTERM_INVERSE   0x4
#define XTERM_BRIGHTFG  0x8
#define XTERM_BRIGHTBG  0x10

class Xterm
{
public:
    Xterm(rgb_t *pFramebuffer, size_t nWidth, size_t nHeight, size_t offsetLeft, size_t offsetTop);
    ~Xterm();

    /** Writes the given UTF-32 character to the Xterm. */
    void write(uint32_t utf32, DirtyRectangle &rect);

    /** Performs a full re-render. */
    void renderAll(DirtyRectangle &rect);

    size_t getRows()
    {
        return m_pWindows[0]->m_Height;
    }
    size_t getCols()
    {
        return m_pWindows[0]->m_Width;
    }

private:
    class Window
    {
        friend class Xterm;
        private:
            struct TermChar
            {
                uint8_t flags;
                uint8_t fore, back;
                uint32_t utf32;
            };

        public:
            Window(size_t nRows, size_t nCols, rgb_t *pFb, size_t nMaxScrollback, size_t offsetLeft, size_t offsetTop, size_t fbWidth);
            ~Window();

            void resize(size_t nRows, size_t nCols);

            void setScrollRegion(int start, int end);
            void setForeColour(uint8_t fgColour);
            void setBackColour(uint8_t bgColour);
            void setFlags(uint8_t flags);

            void renderAll(DirtyRectangle &rect);
            void setChar(uint32_t utf32, size_t x, size_t y);

            void addChar(uint32_t utf32, DirtyRectangle &rect);

            void cursorLeft(DirtyRectangle &rect);
            void cursorLeftToMargin(DirtyRectangle &rect);
            void cursorDown(size_t n, DirtyRectangle &);

            void cursorDownAndLeftToMargin(DirtyRectangle &rect);
            void cursorDownAndLeft(DirtyRectangle &rect);

            void render(DirtyRectangle &rect, size_t flags=0, size_t x=~0UL, size_t y=~0UL);

        private:
            Window(const Window &);
            Window &operator = (const Window &);

            void scrollRegionUp(size_t n, DirtyRectangle &rect);
            void scrollScreenUp(size_t n, DirtyRectangle &rect);

            TermChar *m_pBuffer;
            size_t m_BufferLength;

            rgb_t *m_pFramebuffer;

            size_t m_FbWidth;
            size_t m_Width, m_Height;

            size_t m_OffsetLeft, m_OffsetTop;

            size_t m_nMaxScrollback;
            size_t m_CursorX, m_CursorY;

            size_t m_ScrollStart, m_ScrollEnd;

            TermChar *m_pInsert;
            TermChar *m_pView;

            uint8_t m_Fg, m_Bg;
            uint8_t m_Flags;
    };

    Xterm(const Xterm &);
    Xterm &operator = (const Xterm &);

    /** Current active buffer. */
    size_t m_ActiveBuffer;

    Window *m_pWindows[2];
};

#endif
