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

/// \todo move implementation into Framebuffer.cc
/// \todo Pixel format conversion

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
                                  uint32_t &dest, PixelFormat destFormat)
        {
            if((srcFormat == destFormat) || (!source))
            {
                dest = source;
                return true;
            }
            
            // Amount of red/green/blue
            size_t amtRed = 0, amtGreen = 0, amtBlue = 0;
            if((srcFormat == Bits32_Argb) ||
               (srcFormat == Bits32_Rgb) ||
               (srcFormat == Bits24_Rgb))
            {
                amtRed = (source & 0xff0000) >> 16;
                amtGreen = (source & 0xff00) >> 8;
                amtBlue = (source & 0xff);
            }
            
            // Conversion code. Complicated and ugly. :(
            switch(srcFormat)
            {
                case Bits32_Argb:
                case Bits32_Rgb:
                case Bits24_Rgb:
                    // Dead simple conversion from ARGB -> RGB, just lose alpha
                    if(destFormat == Bits32_Rgb)
                    {
                        dest = HOST_TO_LITTLE32(source) & 0xFFFFFF;
                        return true;
                    }
                    // Dead simple conversion from RGB -> ARGB, add 100% alpha
                    else if(destFormat == Bits32_Argb)
                    {
                        dest = HOST_TO_LITTLE32(source) | 0xFF000000;
                        return true;
                    }
                    // Only able to hit this on 24-bit due to initial checks
                    else if(destFormat == Bits32_Rgb)
                    {
                        // Keep alpha out in case dest already has it
                        dest = source & 0xFFFFFF;
                        return true;
                    }
                    // More involved conversion to 16-bit 555
                    else if(destFormat == Bits16_Rgb555)
                    {
                        size_t amtRed = (source & 0xff0000) >> 16;
                        size_t amtGreen = (source & 0xff00) >> 8;
                        size_t amtBlue = (source & 0xff);
                        if(amtRed)
                            amtRed = ((amtRed / 255) * 0x1F) & 0x1F;
                        if(amtGreen)
                            amtGreen = ((amtGreen / 255) * 0x1F) & 0x1F;
                        if(amtBlue)
                            amtBlue = ((amtBlue / 255) * 0x1F) & 0x1F;
                        
                        dest = (amtRed << 10) | (amtGreen << 5) | (amtBlue);
                    }
                    // About the same for 16-bit 565
                    else if(destFormat == Bits16_Rgb565)
                    {
                        size_t amtRed = (source & 0xff0000) >> 16;
                        size_t amtGreen = (source & 0xff00) >> 8;
                        size_t amtBlue = (source & 0xff);
                        if(amtRed)
                            amtRed = ((amtRed / 255) * 0x1F) & 0x1F;
                        if(amtGreen)
                            amtGreen = ((amtGreen / 255) * 0x3F) & 0x3F;
                        if(amtBlue)
                            amtBlue = ((amtBlue / 255) * 0x1F) & 0x1F;
                        
                        dest = (amtRed << 10) | (amtGreen << 5) | (amtBlue);
                    }
                    // About the same for 16-bit ARGB
                    else if(destFormat == Bits16_Argb)
                    {
                        if(amtRed)
                            amtRed = ((amtRed / 255) * 0xF) & 0xF;
                        if(amtGreen)
                            amtGreen = ((amtGreen / 255) * 0xF) & 0xF;
                        if(amtBlue)
                            amtBlue = ((amtBlue / 255) * 0xF) & 0xF;
                        
                        dest = (0xF << 12) | (amtRed << 8) | (amtGreen << 4) | (amtBlue);
                    }
                    break;
                case Bits16_Argb:
                    break;
                case Bits16_Rgb565:
                    break;
                case Bits16_Rgb555:
                    break;
                case Bits8_Idx:
                    break;
                default:
                    break;
            }
            
            return false;
        }
        
        /** Performs an update of a region of this framebuffer. This function
         *  can be used by drivers to request an area of the framebuffer be
         *  redrawn, but is useless for non-hardware-accelerated devices.
         *  \param x leftmost x co-ordinate of the redraw area, ~0 for "invalid"
         *  \param y topmost y co-ordinate of the redraw area, ~0 for "invalid"
         *  \param w width of the redraw area, ~0 for "invalid"
         *  \param h height of the redraw area, ~0 for "invalid" */
        virtual void redraw(size_t x = ~0UL, size_t y = ~0UL,
                            size_t w = ~0UL, size_t h = ~0UL)
        {
        }
        
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
        
        /** Draws a single rectangle to the screen with the given 32-bit
         *  colour in RGBA format (where ALPHA is in the MSB). */
        virtual inline void rect(size_t x, size_t y, size_t width, size_t height,
                                 uint32_t colour)
        {
            swRect(x, y, width, height, colour);
        }
        
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
        
        inline void swBlit(void *pBuffer, size_t srcx, size_t srcy, size_t destx,
                           size_t desty, size_t width, size_t height)
        {
            if(UNLIKELY((!m_pDisplay) || (!m_FramebufferBase)))
                return;
            if(UNLIKELY(!(width && height)))
                return;
            
            Display::ScreenMode currMode;
            if(UNLIKELY(!m_pDisplay->getCurrentScreenMode(currMode)))
                return;
            
            size_t bytesPerPixel = currMode.pf.nBpp / 8;
            size_t bytesPerLine = bytesPerPixel * currMode.width; /// \todo Not always the right calculation
            size_t sourceBytesPerLine = width * bytesPerPixel; /// \todo Pass a struct for the buffer with full buffer width
            
            /// \todo Clipping
            
            // Blit across the width of the screen? How handy!
            if(UNLIKELY((!(srcx && destx)) && (width == currMode.width)))
            {
                size_t sourceBufferOffset = (srcy * sourceBytesPerLine) + (srcx * bytesPerPixel);
                size_t frameBufferOffset = (desty * bytesPerLine) + (destx * bytesPerPixel);
                
                void *dest = reinterpret_cast<void*>(m_FramebufferBase + frameBufferOffset);
                void *src = adjust_pointer(pBuffer, sourceBufferOffset);
                
                memcpy(dest, src, width * bytesPerPixel);
            }
            else
            {
                // Line-by-line copy
                for(size_t y1 = desty, y2 = srcy; y1 < (desty + height); y1++, y2++)
                {
                    size_t sourceBufferOffset = (y2 * sourceBytesPerLine) + (srcx * bytesPerPixel);
                    size_t frameBufferOffset = (y1 * bytesPerLine) + (destx * bytesPerPixel);
                
                    void *dest = reinterpret_cast<void*>(m_FramebufferBase + frameBufferOffset);
                    void *src = adjust_pointer(pBuffer, sourceBufferOffset);
                    
                    memcpy(dest, src, width * bytesPerPixel);
                }
            }
        }
        
        inline void swRect(size_t x, size_t y, size_t width, size_t height, uint32_t colour)
        {
            if(UNLIKELY((!m_pDisplay) || (!m_FramebufferBase)))
                return;
            if(UNLIKELY(!(width && height)))
                return;
            
            Display::ScreenMode currMode;
            if(UNLIKELY(!m_pDisplay->getCurrentScreenMode(currMode)))
                return;
            
            size_t bytesPerPixel = currMode.pf.nBpp / 8;
            size_t bytesPerLine = bytesPerPixel * currMode.width; /// \todo Not always the right calculation
            
            /// \todo Clipping
            
            // Can we just do an easy memset?
            if(UNLIKELY((!x) && (width == currMode.width)))
            {
                size_t frameBufferOffset = (y * bytesPerLine) + (x * bytesPerPixel);
                
                void *dest = reinterpret_cast<void*>(m_FramebufferBase + frameBufferOffset);
                
                /// \todo Colour conversion if the framebuffer isn't in the same
                ///       pixel format!
                dmemset(dest, colour, (width * bytesPerPixel) / 4);
            }
            else
            {
                // Line-by-line fill
                for(size_t desty = y; desty < (y + height); desty++)
                {
                    size_t frameBufferOffset = (desty * bytesPerLine) + (x * bytesPerPixel);
                
                    void *dest = reinterpret_cast<void*>(m_FramebufferBase + frameBufferOffset);
                    
                    dmemset(dest, colour, (width * bytesPerPixel) / 4);
                }
            }
        }
};

#endif

