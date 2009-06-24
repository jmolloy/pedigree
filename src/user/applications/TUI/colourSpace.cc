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

#include "environment.h"
#include "colourSpace.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#if 0
void packColour(Display::PixelFormat pf, rgb_t colour, void *pFb, size_t idx)
{
    uint8_t r = colour.r;
    uint8_t g = colour.g;
    uint8_t b = colour.b;

    // Calculate the range of the Red field.
    uint8_t range = 1 << pf.mRed;

    // Clamp the red value to this range.
    r = (r * range) / 256;

    range = 1 << pf.mGreen;

    // Clamp the green value to this range.
    g = (g * range) / 256;

    range = 1 << pf.mBlue;

    // Clamp the blue value to this range.
    b = (b * range) / 256;

    // Assemble the colour.
    uint32_t c =  0 |
        (static_cast<uint32_t>(r) << pf.pRed) |
        (static_cast<uint32_t>(g) << pf.pGreen) |
        (static_cast<uint32_t>(b) << pf.pBlue);

    switch (pf.nBpp)
    {
        case 16:
        {
            uint16_t *pFb16 = reinterpret_cast<uint16_t*>(pFb);
            pFb16[idx] = c;
            break;
        }
        case 24:
        {
            rgb_t *pFbRgb = reinterpret_cast<rgb_t*>(pFb);
            pFbRgb[idx].r = static_cast<uint32_t>(r) << pf.pRed;
            pFbRgb[idx].g = static_cast<uint32_t>(g) << pf.pGreen;
            pFbRgb[idx].b = static_cast<uint32_t>(b) << pf.pBlue;
            break;
        }
        case 32:
        {
            uint32_t *pFb32 = reinterpret_cast<uint32_t*>(pFb);
            pFb32[idx] = c;
            break;
        }
    }
}

void swapBuffers(void *pDest, rgb_t *pSrc, rgb_t *pPrevState, size_t nWidth, Display::PixelFormat pf, DirtyRectangle &rect)
{
    char str[128];
    sprintf(str, "Dirty: (%d -> %d, %d -> %d)", rect.getX(), rect.getX()+rect.getWidth(), rect.getY(), rect.getY()+rect.getHeight());
    log(str);
    for (size_t x = rect.getX(); x < rect.getX()+rect.getWidth(); x++)
    {
        for (size_t y = rect.getY(); y < rect.getY()+rect.getHeight(); y++)
        {
            size_t i = y*nWidth + x;
            if (pPrevState[i].r != pSrc[i].r ||
                pPrevState[i].g != pSrc[i].g ||
                pPrevState[i].b != pSrc[i].b)
            {
                pPrevState[i] = pSrc[i];
                packColour(pf, pSrc[i], pDest, i);
            }
        }
    }
}

rgb_t interpolateColour(rgb_t col1, rgb_t col2, uint16_t a)
{
    rgb_t ret;
    ret.r = (col1.r*a + col2.r*(256-a)) / 256;
    ret.g = (col1.g*a + col2.g*(256-a)) / 256;
    ret.b = (col1.b*a + col2.b*(256-a)) / 256;

    return ret;
}
#endif
