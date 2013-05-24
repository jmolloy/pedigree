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

class Framebuffer;
 
namespace Graphics
{
    /// Creates a 24-bit RGB value (Bits24_Rgb)
    inline uint32_t createRgb(uint32_t r, uint32_t g, uint32_t b)
    {
        return (r << 16) | (g << 8) | b;
    }

    enum PixelFormat
    {
        Bits32_Argb,        // Alpha + RGB, with alpha in the highest byte
        Bits32_Rgba,        // RGB + alpha, with alpha in the lowest byte
        Bits32_Rgb,         // RGB, no alpha, essentially the same as above
        Bits32_Bgr,         // BGR, no alpha
        
        Bits24_Rgb,         // RGB in a 24-bit pack
        Bits24_Bgr,         // R and B bytes swapped
        
        Bits16_Argb,        // 4:4:4:4 ARGB, alpha most significant nibble
        Bits16_Rgb565,      // 5:6:5 RGB
        Bits16_Rgb555,      // 5:5:5 RGB
        
        Bits8_Idx,          // Index into a palette
        Bits8_Rgb332,       // Intensity values
    };
    
    inline size_t bitsPerPixel(PixelFormat format)
    {
        switch(format)
        {
            case Bits32_Argb:
            case Bits32_Rgba:
            case Bits32_Rgb:
            case Bits32_Bgr:
                return 32;
            case Bits24_Rgb:
            case Bits24_Bgr:
                return 24;
            case Bits16_Argb:
            case Bits16_Rgb565:
            case Bits16_Rgb555:
                return 16;
            case Bits8_Idx:
            case Bits8_Rgb332:
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

        /// Number of bytes per pixel (as it may be different to the format).
        size_t bytesPerPixel;
        
        /// Buffer ID, for easy identification within drivers
        size_t bufferId;
        
        /// Backing pointer, for drivers. Typically holds a MemoryRegion
        /// for software-only framebuffers to identify the memory's location.
        void *pBacking;
    };

    inline bool convertPixel(uint32_t source, PixelFormat srcFormat,
                             uint32_t &dest, PixelFormat destFormat)
    {
        if((srcFormat == destFormat) || (!source))
        {
            dest = source;
            return true;
        }
        
        // Amount of red/green/blue, in 8-bit intensity values
        uint8_t amtRed = 0, amtGreen = 0, amtBlue = 0, amtAlpha = 0;
        
        // Unpack the pixel as necessary
        if((srcFormat == Bits32_Argb) ||
           (srcFormat == Bits32_Rgb) ||
           (srcFormat == Bits24_Rgb))
        {
            if(srcFormat != Bits24_Rgb)
                amtAlpha = (source & 0xff000000) >> 24;
            amtRed = (source & 0xff0000) >> 16;
            amtGreen = (source & 0xff00) >> 8;
            amtBlue = (source & 0xff);
        }
        else if(srcFormat == Bits32_Rgba)
        {
            amtRed = (source & 0xff000000) >> 24;
            amtGreen = (source & 0xff0000) >> 16;
            amtBlue = (source & 0xff00) >> 8;
            amtAlpha = (source & 0xff);
        }
        else if((srcFormat == Bits32_Bgr) || (srcFormat == Bits24_Bgr))
        {
            amtBlue = (source & 0xff0000) >> 16;
            amtGreen = (source & 0xff00) >> 8;
            amtRed = (source & 0xff);
        }
        else if(srcFormat == Bits16_Argb)
        {
            amtAlpha = (((source & 0xF000) >> 12) / 0xF) * 0xFF;
            amtRed = (((source & 0xF00) >> 8) / 0xF) * 0xFF;
            amtGreen = (((source & 0xF0) >> 4) / 0xF) * 0xFF;
            amtBlue = ((source & 0xF) / 0xF) * 0xFF;
        }
        else if(srcFormat == Bits16_Rgb565)
        {
            amtRed = (((source & 0xF800) >> 11) / 0x1F) * 0xFF;
            amtGreen = (((source & 0x7E0) >> 5) / 0x3F) * 0xFF;
            amtBlue = ((source & 0x1F) / 0x1F) * 0xFF;
        }
        else if(srcFormat == Bits16_Rgb555)
        {
            amtRed = (((source & 0xF800) >> 10) / 0x1F) * 0xFF;
            amtGreen = (((source & 0x3E0) >> 5) / 0x1F) * 0xFF;
            amtBlue = ((source & 0x1F) / 0x1F) * 0xFF;
        }
        else if(srcFormat == Bits8_Rgb332)
        {
            amtRed = (((source & 0xE0) >> 5) / 0x7) * 0xFF;
            amtGreen = (((source & 0x1C) >> 2) / 0x7) * 0xFF;
            amtBlue = ((source & 0x3) / 0x3) * 0xFF;
        }
        
        // Conversion code. Complicated and ugly. :(
        switch(destFormat)
        {
            case Bits32_Argb:
                dest = (amtAlpha << 24) | (amtRed << 16) | (amtGreen << 8) | amtBlue;
                return true;
            case Bits32_Rgba:
                dest = (amtRed << 24) | (amtGreen << 16) | (amtBlue << 8) | amtAlpha;
                return true;
            case Bits32_Rgb:
                dest = (amtRed << 16) | (amtGreen << 8) | amtBlue;
                return true;
            case Bits24_Rgb:
                dest = (amtRed << 16) | (amtGreen << 8) | amtBlue;
                return true;
            case Bits24_Bgr:
                dest = (amtBlue << 16) | (amtGreen << 8) | amtRed;
                return true;
            case Bits16_Rgb555:
                // 8-bit to 5-bit scaling. Lossy.
                amtRed >>= 3;
                amtGreen >>= 3;
                amtBlue >>= 3;
                    
                dest = (amtRed << 10) | (amtGreen << 5) | (amtBlue);
                return true;
            case Bits16_Rgb565:
                // 8-bit to 5 and 6 -bit scaling. Lossy.
                amtRed >>= 3;
                amtGreen >>= 2;
                amtBlue >>= 3;
                    
                dest = (amtRed << 11) | (amtGreen << 5) | (amtBlue);
                return true;
            case Bits16_Argb:
                // 8-bit to 4-bit scaling. Lossy.
                amtRed >>= 4;
                amtGreen >>= 4;
                amtBlue >>= 4;
                amtAlpha >>= 4;
                
                dest = (amtAlpha << 12) | (amtRed << 8) | (amtGreen << 4) | (amtBlue);
                return true;
            case Bits8_Idx:
                /// \todo Palette conversion
                break;
            case Bits8_Rgb332:
                amtRed >>= 5;
                amtGreen >>= 5;
                amtBlue >>= 6;
                
                dest = (amtRed << 5) | (amtGreen << 2) | amtBlue;
                return true;
            default:
                break;
        }
        
        return false;
    }

    /// Creates a new framebuffer as a child of the current framebuffer
    /// \param pFbOverride If non-null, this specifies the memory region to use
    ///                    as the framebuffer for the new object
    Framebuffer *createFramebuffer(Framebuffer *pParent,
                                   size_t x, size_t y,
                                   size_t w, size_t h,
                                   void *pFbOverride = 0);

    /// Destroys a given framebuffer.
    void destroyFramebuffer(Framebuffer *pFramebuffer);
};

#endif

