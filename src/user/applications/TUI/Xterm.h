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

#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>

#define C_BLACK  0
#define C_RED    1
#define C_GREEN  2
#define C_YELLOW 3
#define C_BLUE   4
#define C_MAGENTA 5
#define C_CYAN   6
#define C_WHITE  7

#define C_BRIGHT 8

namespace Display
{

    /** Describes the format of a pixel in a buffer. */
    struct PixelFormat
    {
        uint8_t mRed;       ///< Red mask.
        uint8_t pRed;       ///< Position of red field.
        uint8_t mGreen;     ///< Green mask.
        uint8_t pGreen;     ///< Position of green field.
        uint8_t mBlue;      ///< Blue mask.
        uint8_t pBlue;      ///< Position of blue field.
        uint8_t mAlpha;     ///< Alpha mask.
        uint8_t pAlpha;     ///< Position of the alpha field.
        uint8_t nBpp;       ///< Bits per pixel (total).
        uint32_t nPitch;    ///< Bytes per scanline.
    };

    /** Describes a screen mode / resolution */
    struct ScreenMode
    {
        uint32_t id;
        uint32_t width;
        uint32_t height;
        uint32_t refresh;
        uintptr_t framebuffer;
        PixelFormat pf;
    };
};

/** \brief An Xterm compatible terminal emulator. */
class Xterm
{
public:
    Xterm(Display::ScreenMode mode, void *pFramebuffer);
    virtual ~Xterm();

    void write(char c);
    void write(char *str);

    /** Returns the width of the screen in characters. */
    uint32_t getWidth() {return m_nWidth;}
    /** Returns the height of the screen in characters. */
    uint32_t getHeight() {return m_nHeight;}

protected:

    /** Creates a colour integer suitable for inserting into the framebuffer from an R, G and B list. */
    uint32_t compileColour(uint8_t r, uint8_t g, uint8_t b);

    /** Writes a character out to the framebuffer. Takes a preformatted integer for the foreground and background colours.
        x and y are in characters, not pixels. */
    virtual void putCharFb(uint32_t c, int x, int y, uint32_t f, uint32_t b);

    /** Screen mode, pixel format etc */
    Display::ScreenMode m_Mode;

    /** Framebuffer address. */
    uint8_t *m_pFramebuffer;

    /** Colour list. This is the list of precompiled colour values for the current pixel format. */
    uint32_t m_pColours[16];

    uint32_t m_nWidth, m_nHeight;

    /** Copy constructor is private. */
    Xterm(const Xterm &);
    Xterm &operator = (const Xterm &);

    class Window
    {
    public:
        Window(uint32_t nWidth, uint32_t nHeight, Xterm *pParent);
        ~Window();

        /** Writes a character to the screen, incrementing the X cursor and wrapping if nessecary. */
        void writeChar(unsigned char c);

        /** Gets/sets the cursor coordinates. */
        uint32_t getCursorX() {return m_CursorX;}
        uint32_t getCursorY() {return m_CursorY-m_View;}
        void setCursorX(uint32_t x);
        void setCursorY(uint32_t y);
        void setScrollRegion(uint32_t start, uint32_t end);

        /** Erase to end of line. */
        void eraseEOL();
        /** Erase to start of line. */
        void eraseSOL();
        /** Erase entire line. */
        void eraseLine();
        /** Erase n characters. */
        void eraseChars(size_t n);
        /** Erase from the current line up. */
        void eraseUp();
        /** Erase from the current line down. */
        void eraseDown();
        /** Erase the entire screen. */
        void eraseScreen();
        /** Scroll one line down */
        void scrollDown();
        /** Scroll one line up */
        void scrollUp();
        /** Sets the (foreground) colour. */
        void setFore(uint8_t c);
        /** Sets the (background) colour. */
        void setBack(uint8_t c);
        /** Gets the (foreground) colour. */
        uint8_t getFore() {return m_Foreground;}
        /** Get the (background) colour. */
        uint8_t getBack() {return m_Background;}
        /** Sets whether colours are bold or not. */
        void setBold(bool b);
        /** Deletes n characters */
        void deleteCharacters(uint32_t n);

        /** Perform a full-on screen refresh. */
        void refresh();

        void setLineDrawingMode(bool b);
        bool getLineDrawingMode();

    private:
        Window(const Window &);
        Window &operator = (const Window &);

        //
        // Cursor position.
        //
        uint32_t m_CursorX, m_CursorY;

        //
        // General properties
        //
        uint32_t m_nWidth, m_nHeight;
        uint32_t m_nScrollMin, m_nScrollMax;
        uint8_t m_Foreground, m_Background;
        bool m_bBold;
        bool m_bLineDrawingMode;
        Xterm *m_pParent;

        //
        // Window properties
        //
        uint32_t m_View;

        //
        // Actual data.
        //
        uint16_t *m_pData;
    };

    /// Two windows - one normal and one alternate.
    Window *m_pWindows[2];
    uint8_t m_CurrentWindow;

    /// Are we currently interpreting a state change?
    bool m_bChangingState;

    /// Did this state include a '['? This changes the way some commands are interpreted.
    bool m_bContainedBracket;
    bool m_bContainedParen;

    /// Don't refresh - a string is being processed and a full screen refresh will take place at the end of it.
    bool m_bDontRefresh;

    /// Saved cursor position.
    uint32_t m_SavedX, m_SavedY;
  
    typedef struct
    {
        int params[4];
        int cur_param;
    } XtermCmd;
    XtermCmd m_Cmd;
};

#endif
