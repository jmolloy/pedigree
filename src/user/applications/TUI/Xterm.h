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
    Xterm(rgb_t *pFramebuffer, size_t nWidth, size_t nHeight, size_t offsetLeft, size_t offsetTop, class Terminal *pT);
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

    void showCursor(DirtyRectangle &rect)
    {
        m_pWindows[m_ActiveBuffer]->showCursor(rect);
    }
    void hideCursor(DirtyRectangle &rect)
    {
        m_pWindows[m_ActiveBuffer]->hideCursor(rect);
    }

    void resize(size_t w, size_t h, rgb_t *b)
    {
        m_pWindows[0]->resize(w, h, b);
        m_pWindows[1]->resize(w, h, b);
    }

private:
    class Window
    {
        friend class Xterm;
        private:
            class TermChar
            {
            public:
                bool operator == (const TermChar &other) const
                {
                    return (flags == other.flags &&
                            fore == other.fore &&
                            back == other.back &&
                            utf32 == other.utf32);
                }
                bool operator != (const TermChar &other) const
                {
                    return !(flags == other.flags &&
                            fore == other.fore &&
                            back == other.back &&
                            utf32 == other.utf32);
                }
                uint8_t flags;
                uint8_t fore, back;
                uint32_t utf32;
            };

        public:
            Window(size_t nRows, size_t nCols, rgb_t *pFb, size_t nMaxScrollback, size_t offsetLeft, size_t offsetTop, size_t fbWidth);
            ~Window();

            void showCursor(DirtyRectangle &rect);
            void hideCursor(DirtyRectangle &rect);

            void resize(size_t nRows, size_t nCols, rgb_t *pBuffer);

            void setScrollRegion(int start, int end);
            void setForeColour(uint8_t fgColour);
            void setBackColour(uint8_t bgColour);
            void setFlags(uint8_t flags);
            uint8_t getFlags();

            void renderAll(DirtyRectangle &rect, Window *pPrevious);
            void setChar(uint32_t utf32, size_t x, size_t y);
            TermChar getChar(size_t x=~0UL, size_t y=~0UL);

            void addChar(uint32_t utf32, DirtyRectangle &rect);

            void setCursor(size_t x, size_t y, DirtyRectangle &rect);
            void setCursorX(size_t x, DirtyRectangle &rect);
            void setCursorY(size_t y, DirtyRectangle &rect);
            size_t getCursorX();
            size_t getCursorY();

            void cursorLeft(DirtyRectangle &rect);
            void cursorLeftNum(size_t n, DirtyRectangle &rect);
            void cursorLeftToMargin(DirtyRectangle &rect);
            void cursorDown(size_t n, DirtyRectangle &);

            void cursorDownAndLeftToMargin(DirtyRectangle &rect);
            void cursorDownAndLeft(DirtyRectangle &rect);
            void cursorTab(DirtyRectangle &rect);

            void render(DirtyRectangle &rect, size_t flags=0, size_t x=~0UL, size_t y=~0UL);

            void scrollRegionUp(size_t n, DirtyRectangle &rect);
            void scrollRegionDown(size_t n, DirtyRectangle &rect);

            void scrollScreenUp(size_t n, DirtyRectangle &rect);

            void scrollUp(size_t n, DirtyRectangle &rect);
            void scrollDown(size_t n, DirtyRectangle &rect);

            /** Erase to end of line. */
            void eraseEOL(DirtyRectangle &rect);
            /** Erase to start of line. */
            void eraseSOL(DirtyRectangle &rect);
            /** Erase entire line. */
            void eraseLine(DirtyRectangle &rect);
            /** Erase n characters. */
            void eraseChars(size_t n, DirtyRectangle &rect);
            /** Erase from the current line up. */
            void eraseUp(DirtyRectangle &rect);
            /** Erase from the current line down. */
            void eraseDown(DirtyRectangle &rect);
            /** Erase the entire screen. */
            void eraseScreen(DirtyRectangle &rect);

            /** Deletes n characters */
            void deleteCharacters(size_t n, DirtyRectangle &rect);

            void setLineRenderMode(bool b)
            {
                m_bLineRender = b;
            }

            bool getLineRenderMode()
            {
                return m_bLineRender;
            }
            
            void lineRender(uint32_t utf32, DirtyRectangle &rect);

        private:
            Window(const Window &);
            Window &operator = (const Window &);

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

            bool m_bLineRender;
    };

    Xterm(const Xterm &);
    Xterm &operator = (const Xterm &);

    /** Current active buffer. */
    size_t m_ActiveBuffer;

    Window *m_pWindows[2];

    /// Set of parameters for the XTerm commands
    typedef struct
    {
        int params[4];
        int cur_param;
    } XtermCmd;
    XtermCmd m_Cmd;

    /// Are we currently interpreting a state change?
    bool m_bChangingState;

    /// Did this state include a '['? This changes the way some commands are interpreted.
    bool m_bContainedBracket;
    bool m_bContainedParen;

    /// Saved cursor position.
    uint32_t m_SavedX, m_SavedY;

    class Terminal *m_pT;

    bool m_bFbMode;
};

#endif
