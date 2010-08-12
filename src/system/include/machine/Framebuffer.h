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

class Display;

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

/** This class provides a generic interface for interfacing with a framebuffer.
 *  Each display driver specialises this class to define the "base address" of
 *  the framebuffer in its own way (eg, allocate memory, or use a DMA region).
 *  There are a variety of default software-only operations, which are used by
 *  default if the main operational methods are not overridden. */
class Framebuffer
{
    public:
    
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
        virtual bool convertPixel(uint32_t source, Graphics::PixelFormat srcFormat,
                                         uint32_t &dest, Graphics::PixelFormat destFormat);
         
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
        Graphics::Buffer *createBuffer(const void *srcData, Graphics::PixelFormat srcFormat,
                                       size_t width, size_t height)
        {
            return swCreateBuffer(srcData,srcFormat, width, height);
        }
        
        /** Destroys a created buffer. Frees its memory in both the system RAM
         *  and any references still in VRAM. */
        void destroyBuffer(Graphics::Buffer *pBuffer)
        {
            swDestroyBuffer(pBuffer);
        }
        
        /** Performs an update of a region of this framebuffer. This function
         *  can be used by drivers to request an area of the framebuffer be
         *  redrawn, but is useless for non-hardware-accelerated devices.
         *  \param x leftmost x co-ordinate of the redraw area, ~0 for "invalid"
         *  \param y topmost y co-ordinate of the redraw area, ~0 for "invalid"
         *  \param w width of the redraw area, ~0 for "invalid"
         *  \param h height of the redraw area, ~0 for "invalid" */
        virtual void redraw(size_t x = ~0UL, size_t y = ~0UL,
                            size_t w = ~0UL, size_t h = ~0UL) {}
        
        /** Blits a given buffer to the screen. See createBuffer. */
        virtual inline void blit(Graphics::Buffer *pBuffer, size_t srcx, size_t srcy,
                                 size_t destx, size_t desty, size_t width, size_t height)
        {
            swBlit(pBuffer, srcx, srcy, destx, desty, width, height);
        }
        
        /** Draws a single rectangle to the screen with the given colour. */
        virtual inline void rect(size_t x, size_t y, size_t width, size_t height,
                                 uint32_t colour, Graphics::PixelFormat format = Graphics::Bits32_Argb)
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
                                 uint32_t colour, Graphics::PixelFormat format = Graphics::Bits32_Argb)
        {
            swLine(x1, y1, x2, y2, colour, format);
        }

        /** Sets an individual pixel on the framebuffer. Not inheritable. */
        void setPixel(size_t x, size_t y, uint32_t colour,
                      Graphics::PixelFormat format = Graphics::Bits32_Argb);
        
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
        
        void swBlit(Graphics::Buffer *pBuffer, size_t srcx, size_t srcy,
                    size_t destx, size_t desty, size_t width, size_t height);
        
        void swRect(size_t x, size_t y, size_t width, size_t height,
                    uint32_t colour, Graphics::PixelFormat format);
        
        void swCopy(size_t srcx, size_t srcy, size_t destx, size_t desty,
                    size_t w, size_t h);

        void swLine(size_t x1, size_t y1, size_t x2, size_t y2,
                    uint32_t colour, Graphics::PixelFormat format);
                    
        Graphics::Buffer *swCreateBuffer(const void *srcData, Graphics::PixelFormat srcFormat,
                                         size_t width, size_t height);
        
        void swDestroyBuffer(Graphics::Buffer *pBuffer);
};

#endif

