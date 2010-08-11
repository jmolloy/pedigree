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
         *  same format as the screen. */
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
            
            size_t bytesPerPixel = currMode.pf.nBpp;
            size_t bytesPerLine = bytesPerPixel * currMode.width; /// \todo Not always the right calculation
            
            /// \todo Clipping
            
            // Blit across the width of the screen? How handy!
            if(UNLIKELY((!(srcx && destx)) && (width == currMode.width)))
            {
                size_t sourceBufferOffset = (srcy * bytesPerLine) + (srcx * bytesPerPixel);
                size_t frameBufferOffset = (desty * bytesPerLine) + (destx * bytesPerPixel);
                
                void *dest = reinterpret_cast<void*>(m_FramebufferBase + frameBufferOffset);
                void *src = adjust_pointer(pBuffer, sourceBufferOffset);
                
                memcpy(dest, src, width * height * bytesPerPixel);
            }
            else
            {
                // Line-by-line copy
                for(size_t y = desty; y < (desty + height); y++)
                {
                    size_t sourceBufferOffset = (y * bytesPerLine) + (srcx * bytesPerPixel);
                    size_t frameBufferOffset = (y * bytesPerLine) + (destx * bytesPerPixel);
                
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
            if(UNLIKELY(!(x && y && width && height)))
                return;
            
            Display::ScreenMode currMode;
            if(UNLIKELY(!m_pDisplay->getCurrentScreenMode(currMode)))
                return;
            
            size_t bytesPerPixel = currMode.pf.nBpp;
            size_t bytesPerLine = bytesPerPixel * currMode.width; /// \todo Not always the right calculation
            
            /// \todo Clipping
            
            // Can we just do an easy memset?
            if(UNLIKELY((!x) && (width == currMode.width)))
            {
                size_t frameBufferOffset = (y * bytesPerLine) + (x * bytesPerPixel);
                
                void *dest = reinterpret_cast<void*>(m_FramebufferBase + frameBufferOffset);
                
                /// \todo Colour conversion if the framebuffer isn't in the same
                ///       pixel format!
                dmemset(dest, colour, width * bytesPerPixel);
            }
            else
            {
                // Line-by-line fill
                for(size_t desty = y; desty < (y + height); desty++)
                {
                    size_t frameBufferOffset = (desty * bytesPerLine) + (x * bytesPerPixel);
                
                    void *dest = reinterpret_cast<void*>(m_FramebufferBase + frameBufferOffset);
                    
                    dmemset(dest, colour, width * bytesPerPixel);
                }
            }
        }
};

#endif

