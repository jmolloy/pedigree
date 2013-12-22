/*
 * Copyright (c) 2013 Matthew Iselin
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

#ifndef TEXTIO_H
#define TEXTIO_H

#include <processor/types.h>

class Vga;

/**
 * Provides exceptionally simple VT100 emulation to the Vga class, if
 * one exists. Note that this is NOT xterm emulation.
 */
class TextIO
{
private:
    enum VgaColour
    {
        Black       =0,
        Blue        =1,
        Green       =2,
        Cyan        =3,
        Red         =4,
        Magenta     =5,
        Orange      =6,
        LightGrey   =7,
        DarkGrey    =8,
        LightBlue   =9,
        LightGreen  =10,
        LightCyan   =11,
        LightRed    =12,
        LightMagenta=13,
        Yellow      =14,
        White       =15
    };

public:  
    TextIO();
    ~TextIO();

    /**
     * Initialise and prepare for rendering.
     * Clears the screen as a side-effect, unless false is passed in
     * for bClear.
     * \return True if we can now write, false otherwise.
     */
    bool initialise(bool bClear = true);
  
    /**
     * Write a string to the screen, handling any VT100 control sequences
     * embedded in the string along the way.
     * \param str The string to write.
     */
    void write(const char *s, size_t len);

private:
    void setColour(VgaColour *which, size_t param, bool bBright = false);

    bool m_bInitialised;
    bool m_bControlSeq;
    bool m_bBracket;
    bool m_bParams;
    size_t m_CursorX, m_CursorY;
    size_t m_SavedCursorX, m_SavedCursorY;
    size_t m_ScrollStart, m_ScrollEnd;
    size_t m_CurrentParam;
    size_t m_Params[4];

    VgaColour m_Fore, m_Back;

    uint16_t *m_pFramebuffer;
    Vga *m_pVga;
};

#endif
