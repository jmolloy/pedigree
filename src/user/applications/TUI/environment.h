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

#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include <stdint.h>
#include <stdlib.h>

#include <Widget.h>

struct rgb_t
{
    uint8_t b;
    uint8_t g;
    uint8_t r;
    uint8_t a;
} __attribute__((packed));

namespace Keyboard
{
    enum KeyFlags
    {
        Special = 1ULL << 63,
        Ctrl    = 1ULL << 62,
        Shift   = 1ULL << 61,
        Alt     = 1ULL << 60,
        AltGr   = 1ULL << 59
    };
}

class PedigreeTerminalEmulator : public Widget
{
    public:
        PedigreeTerminalEmulator() : Widget(), m_nWidth(0), m_nHeight(0)
        {};

        virtual ~PedigreeTerminalEmulator()
        {};

        virtual bool render(PedigreeGraphics::Rect &rt, PedigreeGraphics::Rect &dirty)
        {
            return true;
        }

        void handleReposition(const PedigreeGraphics::Rect &rt)
        {
            m_nWidth = rt.getW();
            m_nHeight = rt.getH();
        }

        size_t getWidth() const
        {
            return m_nWidth;
        }

        size_t getHeight() const
        {
            return m_nHeight;
        }

    private:
        size_t m_nWidth;
        size_t m_nHeight;
};

extern PedigreeTerminalEmulator *g_pEmu;

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
}

class DirtyRectangle
{
public:
    DirtyRectangle();
    ~DirtyRectangle();

    void point(size_t x, size_t y);

    size_t getX() {return m_X;}
    size_t getY() {return m_Y;}
    size_t getX2() {return m_X2;}
    size_t getY2() {return m_Y2;}
    size_t getWidth() {return m_X2-m_X+1;}
    size_t getHeight() {return m_Y2-m_Y+1;}

    void reset()
    {
        m_X = 0; m_Y = 0; m_X2 = 0; m_X2 = 0;
    }

private:
    size_t m_X, m_Y, m_X2, m_Y2;
};

void doRedraw(DirtyRectangle &rect);

rgb_t interpolateColour(rgb_t col1, rgb_t col2, uint16_t a);

extern void log(const char *);

#endif
