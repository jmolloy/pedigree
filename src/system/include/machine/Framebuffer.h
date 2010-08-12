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
#ifndef _MACHINE_FRAMEBUFFER_H
#define _MACHINE_FRAMEBUFFER_H

#include <processor/types.h>
#include <utilities/utility.h>
#include <machine/Display.h>

/** This class provides a generic interface for interfacing with a framebuffer.
 *  Each display driver specialises this class to define the "base address" of
 *  the framebuffer in its own way (eg, allocate memory, or use a DMA region).
 *  There are a variety of default software-only operations, which are used by
 *  default if the main operational methods are not overridden. */
class Framebuffer
{
    public:
    
        enum PixelFormat
        {
            Bits32_Argb,        // Alpha + RGB, with alpha in the highest byte
            Bits32_Rgb,         // RGB, no alpha, essentially the same as above
            
            Bits24_Rgb,         // RGB in a 24-bit pack
            
            Bits16_Argb,        // 4:4:4:4 ARGB, alpha most significant nibble
            Bits16_Rgb565,      // 5:6:5 RGB
            Bits16_Rgb555,      // 5:5:5 RGB
            
            Bits8_Idx           // Index into a palette
        };
        
        struct Buffer
        {
            uintptr_t base;
            size_t width;
            size_t height;
            size_t bpp;
            
            PixelFormat format;
        };
    
        Framebuffer() : m_pDisplay(0), m_FramebufferBase(0)
        {
        }
        
        virtual ~Framebuffer()
        {
        }
        
        /** Gets a raw pointer to the framebuffer itself. There is no way to
         *  know if this pointer points to an MMIO region or real RAM, so it
         *  cannot be guaranteed to be safe. */
        virtual void *getRawBuffer()
        {
            return reinterpret_cast<void*>(m_FramebufferBase);
        }
        
        /** Converts a given pixel from one pixel format to another. */
        virtual bool convertPixel(uint32_t source, PixelFormat srcFormat,
                                         uint32_t &dest, PixelFormat destFormat);
        
        /** Performs an update of a region of this framebuffer. This function
         *  can be used by drivers to request an area of the framebuffer be
         *  redrawn, but is useless for non-hardware-accelerated devices.
         *  \param x leftmost x co-ordinate of the redraw area, ~0 for "invalid"
         *  \param y topmost y co-ordinate of the redraw area, ~0 for "invalid"
         *  \param w width of the redraw area, ~0 for "invalid"
         *  \param h height of the redraw area, ~0 for "invalid" */
        virtual void redraw(size_t x = ~0UL, size_t y = ~0UL,
                            size_t w = ~0UL, size_t h = ~0UL) {}
        
        /** Blits a given buffer to the screen. Assumes the buffer is in the
         *  same format as the screen.
         *  \todo The buffer should be "created" somehow, external to this blit.
         *        This works better for hardware accelerated devices which need
         *        to have pixmaps constructed in video memory for accelerated
         *        blits.
         */
        virtual inline void blit(void *pBuffer, size_t srcx, size_t srcy, size_t destx,
                                 size_t desty, size_t width, size_t height)
        {
            swBlit(pBuffer, srcx, srcy, destx, desty, width, height);
        }
        
        /** Draws a single rectangle to the screen with the given colour. */
        virtual inline void rect(size_t x, size_t y, size_t width, size_t height,
                                 uint32_t colour, PixelFormat format = Bits32_Argb)
        {
            swRect(x, y, width, height, colour, format);
        }
        
        /** Copies a rectangle already on the framebuffer to a new location */
        virtual inline void copy(size_t srcx, size_t srcy,
                                 size_t destx, size_t desty,
                                 size_t w, size_t h)
        {
            swCopy(srcx, srcy, destx, desty, w, h);
        }

        /** Draws a line one pixel wide between two points on the screen */
        virtual inline void line(size_t x1, size_t y1, size_t x2, size_t y2,
                                 uint32_t colour, PixelFormat format = Bits32_Argb)
        {
            swLine(x1, y1, x2, y2, colour, format);
        }

        /** Sets an individual pixel on the framebuffer. Not inheritable. */
        void setPixel(size_t x, size_t y, uint32_t colour, PixelFormat format = Bits32_Argb);
        
    private:
    
    protected:
        
        // Linked Display, used to get mode information for software routines
        Display *m_pDisplay;
    
        // Base address of this framebuffer, set by whatever code inherits this
        // class, ideally in the constructor.
        uintptr_t m_FramebufferBase;
        
        void setFramebuffer(uintptr_t p)
        {
            m_FramebufferBase = p;
        }
        void setDisplay(Display *p)
        {
            m_pDisplay = p;
        }
        
        void swBlit(void *pBuffer, size_t srcx, size_t srcy, size_t destx,
                    size_t desty, size_t width, size_t height);
        
        void swRect(size_t x, size_t y, size_t width, size_t height, uint32_t colour, PixelFormat format);
        
        void swCopy(size_t srcx, size_t srcy, size_t destx, size_t desty, size_t w, size_t h);

        void swLine(size_t x1, size_t y1, size_t x2, size_t y2, uint32_t colour, PixelFormat format);
};

#endif

