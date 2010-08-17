/*
 * Copyright (c) 2010 James Molloy, Matthew Iselin
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
#include <graphics/Graphics.h>
#include <types.h>

#include <pedigree-c/pedigree-syscalls.h>

#define PACKED __attribute__((packed)) /// \todo Move to a header

using namespace PedigreeGraphics;

struct blitargs
{
    Buffer *pBuffer;
    uint32_t srcx, srcy, destx, desty, width, height;
} PACKED;

struct drawargs
{
    void *a;
    uint32_t b, c, d, e, f, g, h;
} PACKED;

struct fourargs
{
    uint32_t a, b, c, d;
} PACKED;

struct sixargs
{
    uint32_t a, b, c, d, e, f;
} PACKED;

Framebuffer::Framebuffer() : m_Provider(), m_bProviderValid(false)
{
    int ret = pedigree_gfx_get_provider(&m_Provider);
    if(ret >= 0)
        m_bProviderValid = true;
}

void *Framebuffer::getRawBuffer()
{
    if(!m_bProviderValid)
        return 0;
    
    return reinterpret_cast<void*>(pedigree_gfx_get_raw_buffer(&m_Provider));
}

Buffer *Framebuffer::createBuffer(const void *srcData, PixelFormat srcFormat,
                                  size_t width, size_t height)
{
    if(!m_bProviderValid)
        return 0;
    
    Buffer *ret = 0;
    fourargs args;
    args.a = reinterpret_cast<uint32_t>(srcData); /// \todo 64-bit caveat
    args.b = static_cast<uint32_t>(srcFormat);
    args.c = width;
    args.d = height;
    int ok = pedigree_gfx_create_buffer(&m_Provider, reinterpret_cast<void**>(&ret), &args);
    
    if(ok >= 0)
        return ret;
    else
    {
        delete ret;
        return 0;
    }
}

void Framebuffer::destroyBuffer(Buffer *pBuffer)
{
    if(!m_bProviderValid)
        return;
    
    pedigree_gfx_destroy_buffer(&m_Provider, pBuffer);
}

void Framebuffer::redraw(size_t x, size_t y,
                         size_t w, size_t h)
{
    if(!m_bProviderValid)
        return;
    
    pedigree_gfx_redraw(&m_Provider, x, y, w, h);
}

void Framebuffer::blit(Buffer *pBuffer, size_t srcx, size_t srcy,
                       size_t destx, size_t desty, size_t width, size_t height)
{
    blitargs args;
    args.pBuffer = pBuffer;
    args.srcx = srcx;
    args.srcy = srcy;
    args.destx = destx;
    args.desty = desty;
    args.width = width;
    args.height = height;
    
    pedigree_gfx_blit(&m_Provider, &args);
}

void Framebuffer::draw(void *pBuffer, size_t srcx, size_t srcy,
                       size_t destx, size_t desty, size_t width, size_t height,
                       PixelFormat format)
{
    if(!m_bProviderValid)
        return;
    
    drawargs args;
    args.a = pBuffer;
    args.b = srcx;
    args.c = srcy;
    args.d = destx;
    args.e = desty;
    args.f = width;
    args.g = height;
    args.h = static_cast<uint32_t>(format);
    
    pedigree_gfx_draw(&m_Provider, &args);
}

void Framebuffer::rect(size_t x, size_t y, size_t width, size_t height,
                       uint32_t colour, PixelFormat format)
{
    if(!m_bProviderValid)
        return;
    
    sixargs args;
    args.a = x;
    args.b = y;
    args.c = width;
    args.d = height;
    args.e = colour;
    args.f = static_cast<uint32_t>(format);
    
    pedigree_gfx_rect(&m_Provider, &args);
}

void Framebuffer::copy(size_t srcx, size_t srcy,
                       size_t destx, size_t desty,
                       size_t w, size_t h)
{
    if(!m_bProviderValid)
        return;
    
    sixargs args;
    args.a = srcx;
    args.b = srcy;
    args.c = destx;
    args.d = desty;
    args.e = w;
    args.f = h;
    
    pedigree_gfx_copy(&m_Provider, &args);
}

void Framebuffer::line(size_t x1, size_t y1, size_t x2, size_t y2,
                       uint32_t colour, PixelFormat format)
{
    if(!m_bProviderValid)
        return;
    
    sixargs args;
    args.a = x1;
    args.b = y1;
    args.c = x2;
    args.d = y2;
    args.e = colour;
    args.f = static_cast<uint32_t>(format);
    
    pedigree_gfx_line(&m_Provider, &args);
}

void Framebuffer::setPixel(size_t x, size_t y, uint32_t colour,
                           PixelFormat format)
{
    if(!m_bProviderValid)
        return;
    
    pedigree_gfx_set_pixel(&m_Provider, x, y, colour, static_cast<uint32_t>(format));
}

