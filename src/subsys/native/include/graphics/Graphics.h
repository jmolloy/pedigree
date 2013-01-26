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

#include <types.h>

namespace PedigreeGraphics
{
    class Rect
    {
        public:

            Rect() : x(0), y(0), w(0), h(0) {};
            Rect(size_t x, size_t y, size_t width, size_t height) :
                x(x), y(y), w(width), h(height)
            {};

            virtual ~Rect() {};

            void update(size_t x, size_t y, size_t w, size_t h)
            {
                x = x; y = y;
                w = w; h = h;
            }

            inline size_t getX()
            {
                return x;
            }

            inline size_t getY()
            {
                return y;
            }

            inline size_t getW()
            {
                return w;
            }

            inline size_t getH()
            {
                return h;
            }

        private:

            size_t x, y;
            size_t w, h;
    };

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

        Bits8_Idx,          // Index into a palette
        Bits8_Rgb332,
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

    /// Creates a 24-bit RGB value (Bits24_Rgb)
    inline uint32_t createRgb(uint32_t r, uint32_t g, uint32_t b)
    {
        return (r << 16) | (g << 8) | b;
    }

    struct Buffer
    {
        uint32_t empty;
    };

    class Framebuffer;

    struct GraphicsProvider
    {
        /// \todo Provide the current graphics mode via a watered-down Display
        ///       class.
        void *pDisplay;

        /* Some form of hardware caps here... */
        bool bHardwareAccel;

        Framebuffer *pFramebuffer;

        size_t maxWidth;
        size_t maxHeight;
        size_t maxDepth;

        bool bReserved1;
    };

    /** This class provides a generic interface for interfacing with a framebuffer.
     *  Each display driver specialises this class to define the "base address" of
     *  the framebuffer in its own way (eg, allocate memory, or use a DMA region).
     *  There are a variety of default software-only operations, which are used by
     *  default if the main operational methods are not overridden. */
    class Framebuffer
    {
        public:
        
            Framebuffer();
            virtual ~Framebuffer();

            /** Gets the framebuffer width */
            size_t getWidth();

            /** Gets the framebuffer height */
            size_t getHeight();

            /** Gets the framebuffer's native format */
            PedigreeGraphics::PixelFormat getFormat();

            /** Gets the number of bytes per pixel for the framebuffer */
            size_t getBytesPerPixel();

            /** Sets a new palette for use with indexed colour formats. The
             *  palette should be an array of uint32_t's, all of which will be
             *  interpreted as 24-bit RGB. */
            void setPalette(uint32_t *palette, size_t entries);

            /** Gets a raw pointer to the framebuffer itself. There is no way to
             *  know if this pointer points to an MMIO region or real RAM, so it
             *  cannot be guaranteed to be safe.
             *  There is no way to determine if an application can safely use
             *  this buffer without segfaulting. */
            void *getRawBuffer();

            /** Creates a new child of this framebuffer with the given semantics.
             *  Do a normal "delete" to destroy memory consumed by this new buffer. */
            Framebuffer *createChild(size_t x, size_t y, size_t w, size_t h);

            /** Converts a given pixel from one pixel format to another. */
            bool convertPixel(uint32_t source, PedigreeGraphics::PixelFormat srcFormat,
                              uint32_t &dest, PedigreeGraphics::PixelFormat destFormat);

            /** Creates a new buffer to be used for blits from the given raw pixel
             *  data. Performs automatic conversion of the pixel format to the
             *  pixel format of the current display mode.
             *  Do not modify any of the members of the buffer structure, or attempt
             *  to inject your own pixels into the buffer.
             *  Once a buffer is created, it is only used for blitting to the screen
             *  and cannot be modified.
             *  It is expected that the buffer has been packed to its bit depth, and
             *  does not have any padding on each scanline at all.
             *  Do not delete the returned buffer yourself, pass it to destroyBuffer
             *  which performs a proper cleanup of all resources related to the
             *  buffer.
             *  The buffer should be padded to finish on a DWORD boundary. This is
             *  not padding per scanline but rather padding per buffer. */
            PedigreeGraphics::Buffer *createBuffer(const void *srcData, PedigreeGraphics::PixelFormat srcFormat,
                                           size_t width, size_t height);

            /** Destroys a created buffer. Frees its memory in both the system RAM
             *  and any references still in VRAM. */
            void destroyBuffer(PedigreeGraphics::Buffer *pBuffer);

            /** Performs an update of a region of this framebuffer. This function
             *  can be used by drivers to request an area of the framebuffer be
             *  redrawn, but is useless for non-hardware-accelerated devices.
             *  \param x leftmost x co-ordinate of the redraw area, ~0 for "invalid"
             *  \param y topmost y co-ordinate of the redraw area, ~0 for "invalid"
             *  \param w width of the redraw area, ~0 for "invalid"
             *  \param h height of the redraw area, ~0 for "invalid"
             *  \param bChild non-zero if a child began the redraw, zero otherwise */
            void redraw(size_t x = ~0UL, size_t y = ~0UL,
                        size_t w = ~0UL, size_t h = ~0UL,
                        bool bChild = false);

            /** Blits a given buffer to the screen. See createBuffer. */
            void blit(PedigreeGraphics::Buffer *pBuffer, size_t srcx, size_t srcy,
                      size_t destx, size_t desty, size_t width, size_t height);

            /** Draws given raw pixel data to the screen. Used for framebuffer
             *  chains and applications which need to render constantly changing
             *  pixel buffers. */
            void draw(void *pBuffer, size_t srcx, size_t srcy,
                      size_t destx, size_t desty, size_t width, size_t height,
                      PedigreeGraphics::PixelFormat format = PedigreeGraphics::Bits32_Argb);

            /** Draws a single rectangle to the screen with the given colour. */
            void rect(size_t x, size_t y, size_t width, size_t height,
                      uint32_t colour, PedigreeGraphics::PixelFormat format = PedigreeGraphics::Bits32_Argb);

            /** Copies a rectangle already on the framebuffer to a new location */
            void copy(size_t srcx, size_t srcy,
                      size_t destx, size_t desty,
                      size_t w, size_t h);

            /** Draws a line one pixel wide between two points on the screen */
            void line(size_t x1, size_t y1, size_t x2, size_t y2,
                      uint32_t colour, PedigreeGraphics::PixelFormat format = PedigreeGraphics::Bits32_Argb);

            /** Sets an individual pixel on the framebuffer. Not inheritable. */
            void setPixel(size_t x, size_t y, uint32_t colour,
                          PedigreeGraphics::PixelFormat format = PedigreeGraphics::Bits32_Argb);

            /** Creates a framebuffer given a GraphicsProvider. Sanity checked
             *  and should barely be used (as the information itself only ever
             *  comes from the kernel; it cannot be fabricated). */
            Framebuffer(GraphicsProvider &gfx);

        private:

            GraphicsProvider m_Provider;

            bool m_bProviderValid;

            bool m_bIsChild;
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
        else if(srcFormat == Bits24_Bgr)
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
};

#endif

