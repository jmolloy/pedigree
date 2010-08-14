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
#include <machine/Framebuffer.h>
#include <processor/MemoryRegion.h>
#include <processor/PhysicalMemoryManager.h>
#include <processor/VirtualAddressSpace.h>
#include <machine/Display.h>

Graphics::Buffer *Framebuffer::swCreateBuffer(const void *srcData, Graphics::PixelFormat srcFormat, size_t width, size_t height)
{
    if(UNLIKELY((!m_pDisplay) || (!m_FramebufferBase)))
        return 0;
    if(UNLIKELY(!(width && height)))
        return 0;
    
    Display::ScreenMode currMode;
    if(UNLIKELY(!m_pDisplay->getCurrentScreenMode(currMode)))
        return 0;
    
    Graphics::PixelFormat destFormat = currMode.pf2;
    
    size_t sourceBytesPerPixel = bytesPerPixel(srcFormat);
    size_t sourceBytesPerLine = width * sourceBytesPerPixel;
    
    size_t destBytesPerPixel = currMode.bytesPerPixel;
    size_t destBytesPerLine = width * destBytesPerPixel;
    
    size_t fullBufferSize = width * height * destBytesPerPixel;
    
    MemoryRegion *pRegion = new MemoryRegion("sw-framebuffer-buffer");
    bool bSuccess = PhysicalMemoryManager::instance().allocateRegion(
                     *pRegion,
                     (fullBufferSize / 0x1000) + 1,
                     0,
                     VirtualAddressSpace::Write);

    if(!bSuccess)
    {
        delete pRegion;
        return false;
    }
    
    // Copy the buffer in full
    void *pAddress = pRegion->virtualAddress();
    if((sourceBytesPerPixel == destBytesPerPixel) && (srcFormat == destFormat))
    {
        // Same pixel format and same pixel size. Safe to do a conventional
        // memcpy. The pixel size check is to handle cases where a buffer may
        // be, say, 24-bit RGB, and the display driver also 24-bit RGB, but
        // the display's framebuffer reserves 32 bits for pixels (even though
        // the actual depth is 24 bits).
        memcpy(pAddress, srcData, fullBufferSize);
    }
    else
    {
        // Have to convert and pack each pixel, much slower than memcpy.
        size_t x, y;
        for(y = 0; y < height; y++)
        {
            for(x = 0; x < width; x++)
            {
                size_t sourceOffset = (y * sourceBytesPerLine) + (x * sourceBytesPerPixel);
                size_t destOffset = (y * destBytesPerLine) + (x * destBytesPerPixel);
                
                // We'll always access the beginning of a pixel, which makes
                // things here much simpler.
                const uint32_t *pSource = reinterpret_cast<const uint32_t*>(adjust_pointer(srcData, sourceOffset));
                
                uint32_t transform = 0;
                Graphics::convertPixel(*pSource, srcFormat, transform, destFormat);
                
                if(destBytesPerPixel == 4)
                {
                    uint32_t *pDest = reinterpret_cast<uint32_t*>(adjust_pointer(pAddress, destOffset));
                    *pDest = transform;
                }
                else if(destBytesPerPixel == 3)
                {
                    // Handle existing data after this byte if it exists
                    uint32_t *pDest = reinterpret_cast<uint32_t*>(adjust_pointer(pAddress, destOffset));
                    *pDest = (*pDest & 0xFF000000) | (transform & 0xFFFFFF);
                }
                else if(destBytesPerPixel == 2)
                {
                    uint16_t *pDest = reinterpret_cast<uint16_t*>(adjust_pointer(pAddress, destOffset));
                    *pDest = transform & 0xFFFF;
                }
                else if(destBytesPerPixel == 1)
                {
                    /// \todo 8-bit
                }
            }
        }
    }
    
    Graphics::Buffer *pBuffer = new Graphics::Buffer;
    pBuffer->base = reinterpret_cast<uintptr_t>(pRegion->virtualAddress());
    pBuffer->width = width;
    pBuffer->height = height;
    pBuffer->format = currMode.pf2;
    pBuffer->bufferId = 0;
    pBuffer->pBacking = reinterpret_cast<void*>(pRegion);
    
    return pBuffer;
}

void Framebuffer::swDestroyBuffer(Graphics::Buffer *pBuffer)
{
    if(pBuffer && pBuffer->base)
    {
        MemoryRegion *pRegion = reinterpret_cast<MemoryRegion*>(pBuffer->pBacking);
        delete pRegion; // Unmaps the memory as well
        
        delete pBuffer;
    }
}

void Framebuffer::swBlit(Graphics::Buffer *pBuffer, size_t srcx, size_t srcy,
                         size_t destx, size_t desty, size_t width, size_t height)
{
    if(UNLIKELY((!m_pDisplay) || (!m_FramebufferBase)))
        return;
    if(UNLIKELY(!(width && height)))
        return;
    
    Display::ScreenMode currMode;
    if(UNLIKELY(!m_pDisplay->getCurrentScreenMode(currMode)))
        return;
    
    size_t bytesPerLine = currMode.bytesPerLine;
    size_t destBytesPerPixel = currMode.bytesPerPixel;
    size_t sourceBytesPerLine = pBuffer->width * destBytesPerPixel;
    
    // Sanity check and clip
    if((srcx > pBuffer->width) || (srcy > pBuffer->height))
        return;
    if((destx > currMode.width) || (desty > currMode.height))
        return;
    if((srcx + width) > pBuffer->width)
        width = (srcx + width) - pBuffer->width;
    if((srcy + height) > pBuffer->height)
        height = (srcy + height) - pBuffer->height;
    if((destx + width) > currMode.width)
        width = (destx + width) - currMode.width;
    if((desty + height) > currMode.height)
        height = (desty + height) - currMode.height;
    
    void *pSrc = reinterpret_cast<void*>(pBuffer->base);
    
    // Blit across the width of the screen? How handy!
    if(UNLIKELY((!(srcx && destx)) && (width == currMode.width)))
    {
        size_t sourceBufferOffset = (srcy * sourceBytesPerLine) + (srcx * destBytesPerPixel);
        size_t frameBufferOffset = (desty * bytesPerLine) + (destx * destBytesPerPixel);
        
        void *dest = reinterpret_cast<void*>(m_FramebufferBase + frameBufferOffset);
        void *src = adjust_pointer(pSrc, sourceBufferOffset);
        
        memcpy(dest, src, bytesPerLine * height);
    }
    else
    {
        // Line-by-line copy
        for(size_t y1 = desty, y2 = srcy; y1 < (desty + height); y1++, y2++)
        {
            size_t sourceBufferOffset = (y2 * sourceBytesPerLine) + (srcx * destBytesPerPixel);
            size_t frameBufferOffset = (y1 * bytesPerLine) + (destx * destBytesPerPixel);
        
            void *dest = reinterpret_cast<void*>(m_FramebufferBase + frameBufferOffset);
            void *src = adjust_pointer(pSrc, sourceBufferOffset);
            
            memcpy(dest, src, width * destBytesPerPixel);
        }
    }
}

void Framebuffer::swRect(size_t x, size_t y, size_t width, size_t height, uint32_t colour, Graphics::PixelFormat format)
{
    if(UNLIKELY((!m_pDisplay) || (!m_FramebufferBase)))
        return;
    if(UNLIKELY(!(width && height)))
        return;
    
    Display::ScreenMode currMode;
    if(UNLIKELY(!m_pDisplay->getCurrentScreenMode(currMode)))
        return;

    // Sanity check and clip
    if((x > currMode.width) || (y > currMode.height))
        return;
    if((x + width) > currMode.width)
        width = (x + width) - currMode.width;
    if((y + height) > currMode.height)
        height = (y + height) - currMode.height;
    
    uint32_t transformColour = 0;
    Graphics::convertPixel(colour, format, transformColour, currMode.pf2);
    
    size_t bytesPerPixel = currMode.bytesPerPixel;
    size_t bytesPerLine = currMode.bytesPerLine;
    
    // Can we just do an easy memset?
    if(UNLIKELY((!x) && (width == currMode.width)))
    {
        size_t frameBufferOffset = (y * bytesPerLine) + (x * bytesPerPixel);
        
        void *dest = reinterpret_cast<void*>(m_FramebufferBase + frameBufferOffset);
        
        size_t copySize = (bytesPerLine * height);
        if((bytesPerPixel == 4) || (bytesPerPixel == 3)) /// \todo Handle 24-bit properly
        {
            if((copySize % 8) == 0) // QWORD boundary
            {
                uint64_t val = (static_cast<uint64_t>(transformColour) << 32) | transformColour;
                qmemset(dest, val, copySize / 8);
            }
            else
                dmemset(dest, transformColour, copySize / 4);
        }
        else if(bytesPerPixel == 2)
        {
            if((copySize % 8) == 0) // QWORD boundary
            {
                uint64_t val = (static_cast<uint64_t>(transformColour) << 48) | (static_cast<uint64_t>(transformColour) << 32) | (transformColour << 16) | transformColour;
                qmemset(dest, val, copySize / 8);
            }
            else if((copySize % 4) == 0) // DWORD boundary
            {
                uint32_t val = (transformColour << 16) | transformColour;
                dmemset(dest, val, copySize / 4);
            }
            else
                wmemset(dest, transformColour, copySize / 2);
        }
        else
            memset(dest, transformColour, (bytesPerLine * height));
    }
    else
    {
        // Line-by-line fill
        for(size_t desty = y; desty < (y + height); desty++)
        {
            size_t frameBufferOffset = (desty * bytesPerLine) + (x * bytesPerPixel);
        
            void *dest = reinterpret_cast<void*>(m_FramebufferBase + frameBufferOffset);
            
            size_t copySize = width * bytesPerPixel;
            if((bytesPerPixel == 4) || (bytesPerPixel == 3)) /// \todo Handle 24-bit properly
            {
                if((copySize % 8) == 0) // QWORD boundary
                {
                    uint64_t val = (static_cast<uint64_t>(transformColour) << 32) | transformColour;
                    qmemset(dest, val, copySize / 8);
                }
                else
                    dmemset(dest, transformColour, copySize / 4);
            }
            else if(bytesPerPixel == 2)
            {
                if((copySize % 8) == 0) // QWORD boundary
                {
                    uint64_t val = (static_cast<uint64_t>(transformColour) << 48) | (static_cast<uint64_t>(transformColour) << 32) | (transformColour << 16) | transformColour;
                    qmemset(dest, val, copySize / 8);
                }
                else if((copySize % 4) == 0) // DWORD boundary
                {
                    uint32_t val = (transformColour << 16) | transformColour;
                    dmemset(dest, val, copySize / 4);
                }
                else
                    wmemset(dest, transformColour, copySize / 2);
            }
            else
                memset(dest, transformColour, (width * bytesPerPixel));
        }
    }
}

void Framebuffer::swCopy(size_t srcx, size_t srcy, size_t destx, size_t desty, size_t w, size_t h)
{
    if(UNLIKELY((!m_pDisplay) || (!m_FramebufferBase)))
        return;
    if(UNLIKELY(!(w && h)))
        return;
    if(UNLIKELY((srcx == destx) && (srcy == desty)))
        return;
    
    Display::ScreenMode currMode;
    if(UNLIKELY(!m_pDisplay->getCurrentScreenMode(currMode)))
        return;

    // Sanity check and clip
    if((srcx > currMode.width) || (srcy > currMode.height))
        return;
    if((destx > currMode.width) || (desty > currMode.height))
        return;
    if((srcx + w) > currMode.width)
        w = (srcx + w) - currMode.width;
    if((srcy + h) > currMode.height)
        h = (srcy + h) - currMode.height;
    if((destx + w) > currMode.width)
        w = (destx + w) - currMode.width;
    if((desty + h) > currMode.height)
        h = (desty + h) - currMode.height;
    
    if((srcx == destx) && (srcy == desty))
        return;
    if(!(w && h))
        return;
    
    size_t bytesPerLine = currMode.bytesPerLine;
    size_t bytesPerPixel = currMode.bytesPerPixel;
    size_t sourceBytesPerLine = w * bytesPerPixel;
    
    // Easy memcpy?
    if(UNLIKELY(((!srcx) && (!destx)) && (w == currMode.width)))
    {
        size_t frameBufferOffsetSrc = (srcy * bytesPerLine) + (srcx * bytesPerPixel);
        size_t frameBufferOffsetDest = (desty * bytesPerLine) + (destx * bytesPerPixel);
        
        void *dest = reinterpret_cast<void*>(m_FramebufferBase + frameBufferOffsetDest);
        void *src = reinterpret_cast<void*>(m_FramebufferBase + frameBufferOffsetSrc);
        
        memmove(dest, src, h * bytesPerLine);
    }
    else
    {
        // Not so easy, but still not too hard
        for(size_t yoff = 0; yoff < h; yoff++)
        {
            size_t frameBufferOffsetSrc = ((srcy + yoff) * bytesPerLine) + (srcx * bytesPerPixel);
            size_t frameBufferOffsetDest = ((desty + yoff) * bytesPerLine) + (destx * bytesPerPixel);
            
            void *dest = reinterpret_cast<void*>(m_FramebufferBase + frameBufferOffsetDest);
            void *src = reinterpret_cast<void*>(m_FramebufferBase + frameBufferOffsetSrc);
            
            memmove(dest, src, w * bytesPerPixel);
        }
    }
}

void Framebuffer::swLine(size_t x1, size_t y1, size_t x2, size_t y2, uint32_t colour, Graphics::PixelFormat format)
{
    if(UNLIKELY((!m_pDisplay) || (!m_FramebufferBase)))
        return;
    
    Display::ScreenMode currMode;
    if(UNLIKELY(!m_pDisplay->getCurrentScreenMode(currMode)))
        return;

    // Clip co-ordinates where necessary
    if(x1 > currMode.width)
        x1 = currMode.width;
    if(x2 > currMode.width)
        x2 = currMode.width;
    if(y1 > currMode.height)
        y1 = currMode.height;
    if(y2 > currMode.height)
        y2 = currMode.height;

    if(UNLIKELY((x1 == x2) && (y1 == y2)))
        return;
    
    size_t bytesPerPixel = currMode.bytesPerPixel;
    size_t bytesPerLine = currMode.bytesPerLine;
    
    uint32_t transformColour = 0;
    Graphics::convertPixel(colour, format, transformColour, currMode.pf2);

    // Bresenham's algorithm, referred to Computer Graphics, C Version (2nd Edition)
    // from 1997, by D. Hearn and M. Pauline Baker (page 88)
    // http://www.amazon.com/Computer-Graphics-C-Version-2nd/dp/0135309247

    ssize_t dx = static_cast<ssize_t>(x2 - x1);
    ssize_t dy = static_cast<ssize_t>(y2 - y1);
    bool bSteep = dy > dx;
    if(bSteep)
    {
        swap(x1, y1);
        swap(x2, y2);
    }
    if(x1 > x2)
    {
        swap(x1, x2);
        swap(y1, y2);
    }

    dx = x2 - x1;
    dy = y2 - y1;

    ssize_t error = dx / 2;

    size_t y = y1;

    for(size_t x = x1; x < x2; x++)
    {
        if(bSteep)
            setPixel(y, x, colour, format);
        else
            setPixel(x, y, colour, format);

        error -= dy;
        if(error < 0)
        {
            y++;
            error += dx;
        }
    }
}

void Framebuffer::setPixel(size_t x, size_t y, uint32_t colour, Graphics::PixelFormat format)
{
    if(UNLIKELY((!m_pDisplay) || (!m_FramebufferBase)))
        return;
    
    Display::ScreenMode currMode;
    if(UNLIKELY(!m_pDisplay->getCurrentScreenMode(currMode)))
        return;

    if(x > currMode.width)
        x = currMode.width;
    if(y > currMode.height)
        y = currMode.height;
    
    size_t bytesPerPixel = currMode.bytesPerPixel;
    size_t bytesPerLine = currMode.bytesPerLine;

    uint32_t transformColour = 0;
    Graphics::convertPixel(colour, format, transformColour, currMode.pf2);

    size_t frameBufferOffset = (y * bytesPerLine) + (x * bytesPerPixel);

    if(bytesPerPixel == 4)
    {
        uint32_t *pDest = reinterpret_cast<uint32_t*>(m_FramebufferBase + frameBufferOffset);
        *pDest = transformColour;
    }
    else if(bytesPerPixel == 3)
    {
        // Handle existing data after this byte if it exists
        uint32_t *pDest = reinterpret_cast<uint32_t*>(m_FramebufferBase + frameBufferOffset);
        *pDest = (*pDest & 0xFF000000) | (transformColour & 0xFFFFFF);
    }
    else if(bytesPerPixel == 2)
    {
        uint16_t *pDest = reinterpret_cast<uint16_t*>(m_FramebufferBase + frameBufferOffset);
        *pDest = transformColour & 0xFFFF;
    }
    else if(bytesPerPixel == 1)
    {
        /// \todo 8-bit
    }
}

