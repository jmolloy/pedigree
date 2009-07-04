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
    Xterm(rgb_t *pFramebuffer, size_t nWidth, size_t nHeight, size_t offsetLeft, size_t offsetTop, rgb_t *pBackground);
    ~Xterm();

    /** Writes the given UTF-32 character to the Xterm. */
    void write(uint32_t utf32, DirtyRectangle &rect);

    /** Sets the Xterm as "active". In this mode, the Xterm will update the
        framebuffer. With it disabled, all updates are performed merely on the
        back buffer. */
    void setActive(bool active)
    {m_bActive = active;}

    /** Performs a full re-render. */
    void renderAll(DirtyRectangle &rect);

    size_t getRows()
    {
        return m_Height;
    }
    size_t getCols()
    {
        return m_Width;
    }

private:
    void render(size_t x, size_t y, DirtyRectangle &rect, size_t flags=0);
    void set(size_t x, size_t y, uint32_t utf32);

    void cursorDownScrollIfNeeded(DirtyRectangle &rect);

    void scrollUp(size_t n, DirtyRectangle &rect);

    /** Framebuffer. */
    rgb_t *m_pFramebuffer;

    /** Backbuffer. */
    rgb_t *m_pAltBuffer;

    /** Background image. */
    rgb_t *m_pBackground;

    /** Terminal state, for updating cursor position correctly. */
    struct TermChar
    {
        size_t flags;
        rgb_t fore, back;
        uint32_t utf32;
    };
    TermChar *m_pStatebuffer;
    TermChar *m_pAltStatebuffer;

    /** Current active buffer. */
    size_t m_ActiveBuffer;

    /** Current cursor position. */
    size_t m_CursorX, m_CursorY;

    /** Current foreground colour. */
    uint8_t m_FgColour;
    /** Current background colour. */
    uint8_t m_BgColour;
    /** Current flags. */
    uint8_t m_Flags;

    /** Scroll region, zero-based, inclusive. */
    size_t m_ScrollStart, m_ScrollEnd;

    /** Width of the framebuffer, in pixels. */
    size_t m_FbWidth;
    /** Width and height of the terminal, in characters. */
    size_t m_Width, m_Height;

    /** Offset in pixels of the terminal window from the start of the framebuffer. */
    size_t m_OffsetLeft, m_OffsetTop;

    /** True if we're currently the active terminal. */
    bool m_bActive;
};

#endif
