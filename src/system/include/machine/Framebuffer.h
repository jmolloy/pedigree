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
#include <graphics/Graphics.h>
#include <Log.h>

class Display;

/** This class provides a generic interface for interfacing with a framebuffer.
 *  Each display driver specialises this class to define the "base address" of
 *  the framebuffer in its own way (eg, allocate memory, or use a DMA region).
 *  There are a variety of default software-only operations, which are used by
 *  default if the main operational methods are not overridden. */
class Framebuffer
{
    public:
    
        Framebuffer() : m_pParent(0), m_FramebufferBase(0)
        {
        }
        
        virtual ~Framebuffer()
        {
        }
        
        inline size_t getWidth()
        {
            return m_nWidth;
        }
        
        inline size_t getHeight()
        {
            return m_nHeight;
        }
        
        inline Graphics::PixelFormat getFormat()
        {
            return m_PixelFormat;
        }
        
        /// Sets the palette used for palette-based colour formats. Takes an
        /// array of pixels in Bits32_Argb format.
        inline void setPalette(uint32_t *palette, size_t nEntries)
        {
            /// \todo Write
        }
        
        /** Gets a raw pointer to the framebuffer itself. There is no way to
         *  know if this pointer points to an MMIO region or real RAM, so it
         *  cannot be guaranteed to be safe. */
        virtual void *getRawBuffer()
        {
            if(m_pParent)
                return m_pParent->getRawBuffer();
            return reinterpret_cast<void*>(m_FramebufferBase);
        }
        
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
        virtual Graphics::Buffer *createBuffer(const void *srcData, Graphics::PixelFormat srcFormat,
                                               size_t width, size_t height)
        {
            if(m_pParent)
                return m_pParent->createBuffer(srcData, srcFormat, width, height);
            return swCreateBuffer(srcData, srcFormat, width, height);
        }
        
        /** Destroys a created buffer. Frees its memory in both the system RAM
         *  and any references still in VRAM. */
        virtual void destroyBuffer(Graphics::Buffer *pBuffer)
        {
            if(m_pParent)
                m_pParent->destroyBuffer(pBuffer);
            else
                swDestroyBuffer(pBuffer);
        }
        
        /** Performs an update of a region of this framebuffer. This function
         *  can be used by drivers to request an area of the framebuffer be
         *  redrawn, but is useless for non-hardware-accelerated devices.
         *  \param x leftmost x co-ordinate of the redraw area, ~0 for "invalid"
         *  \param y topmost y co-ordinate of the redraw area, ~0 for "invalid"
         *  \param w width of the redraw area, ~0 for "invalid"
         *  \param h height of the redraw area, ~0 for "invalid"
         *  \param bChild non-zero if a child began the redraw, zero otherwise
         */
        void redraw(size_t x = ~0UL, size_t y = ~0UL,
                    size_t w = ~0UL, size_t h = ~0UL,
                    bool bChild = false)
        {
            if(m_pParent)
            {
                // Redraw with parent:
                // 1. Draw our framebuffer. This will go to the top without
                //    changing intermediate framebuffers.
                // 2. Pass a redraw up the chain. This will reach the top level
                //    (with modified x/y) and cause our screen region to be redrawn.

                /// \note If none of the above makes sense, you need to read the
                ///       Pedigree graphics design doc:
                ///       http://pedigree-project.org/issues/114

                // If the redraw was not caused by a child, make sure our
                // framebuffer has precedence over any children.
                /// \todo nChildren parameter - this is not necessary if no children!
                if(!bChild)
                    m_pParent->draw(reinterpret_cast<void*>(m_FramebufferBase), x, y, m_XPos + x, m_YPos + y, w, h, m_PixelFormat);

                // Now we are a child requesting a redraw, so the parent will not
                // have precedence over us.
                m_pParent->redraw(m_XPos + x, m_YPos + y, w, h, true);
            }
            else
                hwRedraw(x, y, w, h);
        }
        
        /** Blits a given buffer to the screen. See createBuffer.
         *  \param bLowestCall whether this is the lowest level call for the
         *                     chain or a call being passed up from a child
         *                     somewhere in the chain.
         */
        virtual inline void blit(Graphics::Buffer *pBuffer, size_t srcx, size_t srcy,
                                 size_t destx, size_t desty, size_t width, size_t height,
                                 bool bLowestCall = true)
        {
            if(m_pParent)
                m_pParent->blit(pBuffer, srcx, srcy, m_XPos + destx, m_YPos + desty, width, height, false);
            if(bLowestCall || !m_pParent)
                swBlit(pBuffer, srcx, srcy, destx, desty, width, height);
        }

        /** Draws given raw pixel data to the screen. Used for framebuffer
         *  chains and applications which need to render constantly changing
         *  pixel buffers. */
        virtual inline void draw(void *pBuffer, size_t srcx, size_t srcy,
                                 size_t destx, size_t desty, size_t width, size_t height,
                                 Graphics::PixelFormat format = Graphics::Bits32_Argb)
        {
            // Draw is implemented as a "create buffer and blit"... so we can
            // avoid checking for parent here as we don't want to poison the
            // parent's buffer.
            swDraw(pBuffer, srcx, srcy, destx, desty, width, height, format);
        }
        
        /** Draws a single rectangle to the screen with the given colour. */
        virtual inline void rect(size_t x, size_t y, size_t width, size_t height,
                                 uint32_t colour, Graphics::PixelFormat format = Graphics::Bits32_Argb,
                                 bool bLowestCall = true)
        {
            if(m_pParent)
                m_pParent->rect(m_XPos + x, m_YPos + y, width, height, colour, format, false);
            if(bLowestCall || !m_pParent)
                swRect(x, y, width, height, colour, format);
        }
        
        /** Copies a rectangle already on the framebuffer to a new location */
        virtual inline void copy(size_t srcx, size_t srcy,
                                 size_t destx, size_t desty,
                                 size_t w, size_t h,
                                 bool bLowestCall = true)
        {
            if(m_pParent)
                m_pParent->copy(m_XPos + srcx, m_YPos + srcy, m_XPos + destx, m_YPos + desty, w, h, false);
            if(bLowestCall || !m_pParent)
                swCopy(srcx, srcy, destx, desty, w, h);
        }

        /** Draws a line one pixel wide between two points on the screen */
        virtual inline void line(size_t x1, size_t y1, size_t x2, size_t y2,
                                 uint32_t colour, Graphics::PixelFormat format = Graphics::Bits32_Argb,
                                 bool bLowestCall = true)
        {
            if(m_pParent)
                m_pParent->line(m_XPos + x1, m_YPos + y1, m_YPos + x2, y2, colour, format, false);
            if(bLowestCall || !m_pParent)
                swLine(x1, y1, x2, y2, colour, format);
        }

        /** Sets an individual pixel on the framebuffer. Not inheritable. */
        void setPixel(size_t x, size_t y, uint32_t colour,
                      Graphics::PixelFormat format = Graphics::Bits32_Argb,
                      bool bLowestCall = true)
        {
            if(m_pParent)
                m_pParent->setPixel(m_XPos + x, m_YPos + y, colour, format, false);
            if(bLowestCall || !m_pParent)
                swSetPixel(x, y, colour, format);
        }
        
        /** Class friendship isn't inheritable, so these have to be public for
         *  graphics drivers to use. They shouldn't be touched by anything that
         *  isn't a graphics driver. */
        
        /// X position on our parent's framebuffer
        size_t m_XPos;
        inline void setXPos(size_t x)
        {
            m_XPos = x;
        }
        
        /// Y position on our parent's framebuffer
        size_t m_YPos;
        inline void setYPos(size_t y)
        {
            m_YPos = y;
        }
        
        /// Width of the framebuffer in pixels
        size_t m_nWidth;
        inline void setWidth(size_t w)
        {
            m_nWidth = w;
        }
        
        /// Height of the framebuffer in pixels
        size_t m_nHeight;
        inline void setHeight(size_t h)
        {
            m_nHeight = h;
        }
        
        /// Framebuffer pixel format
        Graphics::PixelFormat m_PixelFormat;
        inline void setFormat(Graphics::PixelFormat pf)
        {
            m_PixelFormat = pf;
        }
        
        /// Bytes per pixel in this framebuffer
        size_t m_nBytesPerPixel;
        inline void setBytesPerPixel(size_t b)
        {
            m_nBytesPerPixel = b;
        }
        
        /// Bytes per line in this framebuffer
        size_t m_nBytesPerLine;
        inline void setBytesPerLine(size_t b)
        {
            m_nBytesPerLine = b;
        }
        
        /// Parent of this framebuffer
        Framebuffer *m_pParent;
        inline void setParent(Framebuffer *p)
        {
            m_pParent = p;
        }
        inline Framebuffer *getParent()
        {
            return m_pParent;
        }
        
        void setFramebuffer(uintptr_t p)
        {
            m_FramebufferBase = p;
        }
        
    private:

        /** Sets an individual pixel on the framebuffer. Not inheritable. */
        void swSetPixel(size_t x, size_t y, uint32_t colour,
                        Graphics::PixelFormat format = Graphics::Bits32_Argb);
    
    protected:
    
        // Base address of this framebuffer, set by whatever code inherits this
        // class, ideally in the constructor.
        uintptr_t m_FramebufferBase;
        
        void swBlit(Graphics::Buffer *pBuffer, size_t srcx, size_t srcy,
                    size_t destx, size_t desty, size_t width, size_t height);
        
        void swRect(size_t x, size_t y, size_t width, size_t height,
                    uint32_t colour, Graphics::PixelFormat format);
        
        void swCopy(size_t srcx, size_t srcy, size_t destx, size_t desty,
                    size_t w, size_t h);

        void swLine(size_t x1, size_t y1, size_t x2, size_t y2,
                    uint32_t colour, Graphics::PixelFormat format);

        void swDraw(void *pBuffer, size_t srcx, size_t srcy,
                    size_t destx, size_t desty, size_t width, size_t height,
                    Graphics::PixelFormat format = Graphics::Bits32_Argb);
                    
        Graphics::Buffer *swCreateBuffer(const void *srcData, Graphics::PixelFormat srcFormat,
                                         size_t width, size_t height);
        
        void swDestroyBuffer(Graphics::Buffer *pBuffer);
        
        /// Inherited by drivers that provide a hardware redraw function
        virtual void hwRedraw(size_t x = ~0UL, size_t y = ~0UL,
                              size_t w = ~0UL, size_t h = ~0UL) {}
};

#endif

