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

#ifndef XTERM_H
#define XTERM_H

#include "environment.h"

#include <string>

#include <native/graphics/Graphics.h>

#define XTERM_MAX_PARAMS        16

// We must always support at least 132 columns. Normally, the margins would be
// set to whatever the screen supports, but for DECCOLM support we will need to
// be able to expand to 132 columns and only render the visible portion.
#define XTERM_WIDE              132
#define XTERM_STANDARD          80
#define XTERM_MIN_WIDTH         132

// #define XTERM_DEBUG
// #define XTERM_DEBUG_EXTRA

class Xterm
{
public:
    Xterm(PedigreeGraphics::Framebuffer *pFramebuffer, size_t nWidth, size_t nHeight, size_t offsetLeft, size_t offsetTop, class Terminal *pT);
    ~Xterm();

    /** Writes the given UTF-32 character to the Xterm. */
    void write(uint32_t utf32, DirtyRectangle &rect);

    /** Performs a full re-render. */
    void renderAll(DirtyRectangle &rect);

    size_t getRows() const
    {
        return m_pWindows[0]->m_Height;
    }
    size_t getCols() const
    {
        return m_pWindows[0]->m_Width;
    }
    size_t getStride() const
    {
        return m_pWindows[0]->m_Stride;
    }

    void showCursor(DirtyRectangle &rect)
    {
        m_pWindows[m_ActiveBuffer]->showCursor(rect);
    }
    void hideCursor(DirtyRectangle &rect)
    {
        m_pWindows[m_ActiveBuffer]->hideCursor(rect);
    }

    size_t getModes() const
    {
        return m_Modes;
    }

    void resize(size_t w, size_t h, PedigreeGraphics::Framebuffer *pFb)
    {
        m_pWindows[(m_ActiveBuffer + 1) % 2]->resize(w, h, false);
        m_pWindows[m_ActiveBuffer]->resize(w, h, true);
    }

    void setCursorStyle(bool bFilled)
    {
        m_pWindows[m_ActiveBuffer]->setCursorStyle(bFilled);
    }

    /** Processes the given key and injects to the terminal, based on xterm modes. */
    void processKey(uint64_t key);

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
            Window(size_t nRows, size_t nCols, PedigreeGraphics::Framebuffer *pFb, size_t nMaxScrollback, size_t offsetLeft, size_t offsetTop, size_t fbWidth, Xterm *parent);
            ~Window();

            void showCursor(DirtyRectangle &rect);
            void hideCursor(DirtyRectangle &rect);

            void resize(size_t nRows, size_t nCols, bool bActive);

            void setScrollRegion(int start, int end);
            void setForeColour(uint8_t fgColour);
            void setBackColour(uint8_t bgColour);
            void setFlags(uint8_t flags);
            uint8_t getFlags();

            void setMargins(size_t left, size_t right);

            void setChar(uint32_t utf32, size_t x, size_t y);
            TermChar getChar(size_t x=~0UL, size_t y=~0UL);

            void fillChar(uint32_t utf32, DirtyRectangle &rect);
            void addChar(uint32_t utf32, DirtyRectangle &rect);

            void setCursorRelOrigin(size_t x, size_t y, DirtyRectangle &rect);
            void setCursor(size_t x, size_t y, DirtyRectangle &rect);
            void setCursorX(size_t x, DirtyRectangle &rect);
            void setCursorY(size_t y, DirtyRectangle &rect);
            ssize_t getCursorX() const;
            ssize_t getCursorY() const;
            /** The RelOrigin versions retrieve relative when inside the
              * page area, and the normal x/y if not. */
            ssize_t getCursorXRelOrigin() const;
            ssize_t getCursorYRelOrigin() const;
            void setCursorStyle(bool bFilled)
            {
                m_bCursorFilled = bFilled;
            }

            void cursorToOrigin();

            void cursorLeft(DirtyRectangle &rect);
            void cursorLeftNum(size_t n, DirtyRectangle &rect);
            void cursorLeftToMargin(DirtyRectangle &rect);
            void cursorDown(size_t n, DirtyRectangle &);
            void cursorUp(size_t n, DirtyRectangle &);

            void backspace(DirtyRectangle &rect);

            void cursorUpWithinMargin(size_t n, DirtyRectangle &);
            void cursorLeftWithinMargin(size_t n, DirtyRectangle &);
            void cursorRightWithinMargin(size_t n, DirtyRectangle &);
            void cursorDownWithinMargin(size_t n, DirtyRectangle &);

            void cursorDownAndLeftToMargin(DirtyRectangle &rect);
            void cursorDownAndLeft(DirtyRectangle &rect);
            void cursorTab(DirtyRectangle &rect);
            void cursorTabBack(DirtyRectangle &rect);

            void renderAll(DirtyRectangle &rect, Window *pPrevious);
            void render(DirtyRectangle &rect, size_t flags=0, size_t x=~0UL, size_t y=~0UL);
            void renderArea(DirtyRectangle &rect, size_t x=~0UL, size_t y=~0UL, size_t w=~0UL, size_t h=~0UL);

            void scrollRegionUp(size_t n, DirtyRectangle &rect);
            void scrollRegionDown(size_t n, DirtyRectangle &rect);

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
            /** Inserts n blank characters */
            void insertCharacters(size_t n, DirtyRectangle &rect);

            /** Inserts n blank lines */
            void insertLines(size_t n, DirtyRectangle &rect);
            /** Deletes n lines */
            void deleteLines(size_t n, DirtyRectangle &rect);

            /** Sets a tab stop at the current X position */
            void setTabStop();
            /** Clears a tab stop at the current X position */
            void clearTabStop();
            /** Clears all tab stops */
            void clearAllTabStops();

            /** Checks for wrapping and wraps if needed. */
            void checkWrap(DirtyRectangle &rect);

            /** Checks for scrolling and scrolls if needed. */
            void checkScroll(DirtyRectangle &rect);

            /* Inverts the display, if the character has the default colours. */
            void invert(DirtyRectangle &rect);

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

            PedigreeGraphics::Framebuffer *m_pFramebuffer;

            size_t m_FbWidth;
            size_t m_Width, m_Height, m_Stride;

            size_t m_OffsetLeft, m_OffsetTop;

            size_t m_nMaxScrollback;
            ssize_t m_CursorX, m_CursorY;

            ssize_t m_ScrollStart, m_ScrollEnd;
            ssize_t m_LeftMargin, m_RightMargin;

            TermChar *m_pInsert;
            TermChar *m_pView;

            uint8_t m_Fg, m_Bg;
            uint8_t m_Flags;

            bool m_bCursorFilled;

            bool m_bLineRender;

            Xterm *m_pParentXterm;
    };

    Xterm(const Xterm &);
    Xterm &operator = (const Xterm &);

    /** Current active buffer. */
    size_t m_ActiveBuffer;

    Window *m_pWindows[2];

    /// Set of parameters for the XTerm commands
    typedef struct
    {
        int params[XTERM_MAX_PARAMS];
        int cur_param;
        bool has_param;
    } XtermCmd;
    XtermCmd m_Cmd;

    typedef struct
    {
        std::string params[XTERM_MAX_PARAMS];
        int cur_param;
        bool has_param;
    } XtermOsControl;
    XtermOsControl m_OsCtl;

    /// Are we currently interpreting a state change?
    bool m_bChangingState;

    /// Did this state include a '['? This changes the way some commands are interpreted.
    bool m_bContainedBracket;
    bool m_bContainedParen;
    bool m_bIsOsControl;

    enum TerminalModes
    {
        LineFeedNewLine = 0x1,
        CursorKey       = 0x2,
        AnsiVt52        = 0x4,  // ANSI vs VT52 mode toggle.
        Column          = 0x8,
        Scrolling       = 0x10,
        Screen          = 0x20,
        Origin          = 0x40,
        AutoWrap        = 0x80,
        AutoRepeat      = 0x100,
        Interlace       = 0x200,
        AppKeypad       = 0x400,
        Margin          = 0x800,
        Insert          = 0x1000,
    };

    enum TerminalFlags
    {
        /// Seen a '?' character.
        Question        = 0x1,

        /// Seen a '<' character.
        LeftAngle       = 0x2,

        /// Seen a '>' character.
        RightAngle      = 0x4,

        /// Seen a '[' character.
        LeftSquare      = 0x8,

        /// Seen a ']' character.
        RightSquare     = 0x10,

        /// Seen a '(' character.
        LeftRound       = 0x20,

        /// Seen a ')' character.
        RightRound      = 0x40,

        /// Seen a '$' character.
        Dollar          = 0x80,

        /// Seen a '\'' character.
        Apostrophe      = 0x100,

        /// Seen a '"' character.
        Quote           = 0x200,

        /// Seen a ' ' character.
        Space           = 0x400,

        /// Seen a '!' character.
        Bang            = 0x800,

        /// Seen a '\e' (ESC) character.
        Escape          = 0x1000,

        /// Seen a '%' character.
        Percent         = 0x2000,

        /// Seen a '*' character.
        Asterisk        = 0x4000,

        /// Seen a '+' character.
        Plus            = 0x8000,

        /// Seen a '-' character.
        Minus           = 0x10000,

        /// Seen a '.' character.
        Period          = 0x20000,

        /// Seen a '/' character.
        Slash           = 0x40000,

        /// Seen a '#' character.
        Hash            = 0x80000,

        /// Seen a '\' character.
        Backslash       = 0x100000,

        /// Seen a '_' character.
        Underscore      = 0x200000,

        /// VT52: set cursor, waiting for Y.
        Vt52SetCursorWaitY = 0x400000,

        /// VT52: set cursor, waiting for X.
        Vt52SetCursorWaitX = 0x800000,
    };

    bool setFlagsForUtf32(uint32_t utf32);

    /// Flags for this particular sequence.
    size_t m_Flags;

    /// Currently active modes.
    size_t m_Modes;

    /// Tab stops.
    char *m_TabStops;

    /// Saved cursor position.
    uint32_t m_SavedX, m_SavedY;

    class Terminal *m_pT;

    bool m_bFbMode;
};

#endif
