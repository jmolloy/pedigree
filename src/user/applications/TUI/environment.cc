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

#include "environment.h"

#include <native/graphics/Graphics.h>

#include <cairo/cairo.h>

extern PedigreeGraphics::Framebuffer *g_pFramebuffer;

extern cairo_surface_t *g_Surface;

void doRedraw(DirtyRectangle &rect)
{
    if(rect.getX() == ~0UL && rect.getY() == ~0UL &&
       rect.getX2() == 0 && rect.getY2() == 0)
        return;

    if(g_pEmu && g_Surface)
    {
        PedigreeGraphics::Rect rt(rect.getX(), rect.getY(), rect.getWidth(), rect.getHeight());
        cairo_surface_flush(g_Surface);
        g_pEmu->redraw(rt);
    }
}

DirtyRectangle::DirtyRectangle() :
    m_X(~0), m_Y(~0), m_X2(0), m_Y2(0)
{
}

DirtyRectangle::~DirtyRectangle()
{
}

void DirtyRectangle::point(size_t x, size_t y)
{
    if (x < m_X)
        m_X = x;
    if (x > m_X2)
        m_X2 = x;

    if (y < m_Y)
        m_Y = y;
    if (y > m_Y2)
        m_Y2 = y;
}

rgb_t interpolateColour(rgb_t col1, rgb_t col2, uint16_t a)
{
    rgb_t ret;
    ret.r = (col1.r*a + col2.r*(256-a)) / 256;
    ret.g = (col1.g*a + col2.g*(256-a)) / 256;
    ret.b = (col1.b*a + col2.b*(256-a)) / 256;

    return ret;
}
