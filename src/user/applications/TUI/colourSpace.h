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

#ifndef COLOURSPACE_H
#define COLOURSPACE_H

#if 0
#include "environment.h"

struct rgb_t
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
} __attribute__((packed));

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

void packColour(Display::PixelFormat pf, rgb_t colour,
                void *pBuffer, size_t idx);

void swapBuffers(void *pDest, rgb_t *pSrc, rgb_t *pPrevState, size_t nPixels, Display::PixelFormat pf, DirtyRectangle &rect);

rgb_t interpolateColour(rgb_t col1, rgb_t col2, uint16_t a);

#endif

#endif
