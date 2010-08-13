/*
 * Copyright (c) 2010 Matthew Iselin
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
#ifndef _GRAPHICS_H
#define _GRAPHICS_H

#include <processor/types.h>
 
namespace Graphics
{
    enum PixelFormat
    {
        Bits32_Argb,        // Alpha + RGB, with alpha in the highest byte
        Bits32_Rgba,        // RGB + alpha, with alpha in the lowest byte
        Bits32_Rgb,         // RGB, no alpha, essentially the same as above
        
        Bits24_Rgb,         // RGB in a 24-bit pack
        Bits24_Bgr,         // R and B bytes swapped
        
        Bits16_Argb,        // 4:4:4:4 ARGB, alpha most significant nibble
        Bits16_Rgb565,      // 5:6:5 RGB
        Bits16_Rgb555,      // 5:5:5 RGB
        
        Bits8_Idx           // Index into a palette
    };
        
    inline size_t bitsPerPixel(PixelFormat format)
    {
        switch(format)
        {
            case Bits32_Argb:
            case Bits32_Rgba:
            case Bits32_Rgb:
                return 32;
            case Bits24_Rgb:
            case Bits24_Bgr:
                return 24;
            case Bits16_Argb:
            case Bits16_Rgb565:
            case Bits16_Rgb555:
                return 16;
            case Bits8_Idx:
                return 8;
            default:
                return 4;
        }
    }
    
    inline size_t bytesPerPixel(PixelFormat format)
    {
        return bitsPerPixel(format) / 8;
    }
    
    struct Buffer
    {
        /// Base of this buffer in memory. For internal use only.
        uintptr_t base;
        
        /// Width of the buffer in pixels
        size_t width;
        
        /// Height of the buffer in pixels
        size_t height;
        
        /// Output format of the buffer. NOT the input format. Used for
        /// byte-per-pixel calculations.
        PixelFormat format;
        
        /// Buffer ID, for easy identification within drivers
        size_t bufferId;
        
        /// Backing pointer, for drivers. Typically holds a MemoryRegion
        /// for software-only framebuffers to identify the memory's location.
        void *pBacking;
    };
};

#endif

