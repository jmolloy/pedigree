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

bool Framebuffer::convertPixel(uint32_t source, PixelFormat srcFormat,
                               uint32_t &dest, PixelFormat destFormat)
{
    if((srcFormat == destFormat) || (!source))
    {
        dest = source;
        return true;
    }
    
    // Amount of red/green/blue
    size_t amtRed = 0, amtGreen = 0, amtBlue = 0, amtAlpha = 0;
    if((srcFormat == Bits32_Argb) ||
       (srcFormat == Bits32_Rgb) ||
       (srcFormat == Bits24_Rgb))
    {
        amtAlpha = (source & 0xff000000) >> 24;
        amtRed = (source & 0xff0000) >> 16;
        amtGreen = (source & 0xff00) >> 8;
        amtBlue = (source & 0xff);
    }
    else if(srcFormat == Bits16_Argb)
    {
        amtAlpha = (source & 0xF000) >> 12;
        amtRed = (source & 0xF00) >> 8;
        amtGreen = (source & 0xF0) >> 4;
        amtBlue = (source & 0xF);
    }
    else if(srcFormat == Bits16_Rgb565)
    {
        amtRed = (source & 0xF800) >> 11;
        amtGreen = (source & 0x7E0) >> 5;
        amtBlue = (source & 0x1F);
    }
    else if(srcFormat == Bits16_Rgb555)
    {
        amtRed = (source & 0xF800) >> 10;
        amtGreen = (source & 0x3E0) >> 5;
        amtBlue = (source & 0x1F);
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
                return true;
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
                
                dest = (amtRed << 11) | (amtGreen << 5) | (amtBlue);
                return true;
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
                if(amtAlpha)
                    amtAlpha = ((amtAlpha / 255) * 0xF) & 0xF;
                
                dest = (amtAlpha << 12) | (amtRed << 8) | (amtGreen << 4) | (amtBlue);
                return true;
            }
            break;
        case Bits16_Argb:
            // 555 -> 32/24 bit RGB
            if((destFormat == Bits32_Rgb) ||
               (destFormat == Bits32_Argb) ||
               (destFormat == Bits32_Rgb) ||
               (destFormat == Bits24_Rgb))
            {
                if(amtRed)
                    amtRed = ((amtRed / 0x1F) * 0xF) & 0xFF;
                if(amtGreen)
                    amtGreen = ((amtGreen / 0x1F) * 0xF) & 0xFF;
                if(amtBlue)
                    amtBlue = ((amtBlue / 0x1F) * 0xF) & 0xFF;
                
                dest = (amtRed << 16) | (amtGreen << 8) | (amtBlue);
                
                if(destFormat == Bits32_Argb)
                    dest |= 0xFF000000;
                
                return true;
            }
            break;
        case Bits16_Rgb565:
            // 555 -> 32/24 bit RGB
            if((destFormat == Bits32_Rgb) ||
               (destFormat == Bits32_Argb) ||
               (destFormat == Bits32_Rgb) ||
               (destFormat == Bits24_Rgb))
            {
                if(amtRed)
                    amtRed = ((amtRed / 0x1F) * 0xFF) & 0xFF;
                if(amtGreen)
                    amtGreen = ((amtGreen / 0x3F) * 0xFF) & 0xFF;
                if(amtBlue)
                    amtBlue = ((amtBlue / 0x1F) * 0xFF) & 0xFF;
                
                dest = (amtRed << 16) | (amtGreen << 8) | (amtBlue);
                
                if(destFormat == Bits32_Argb)
                    dest |= 0xFF000000;
                
                return true;
            }
            break;
        case Bits16_Rgb555:
            // 555 -> 32/24 bit RGB
            if((destFormat == Bits32_Rgb) ||
               (destFormat == Bits32_Argb) ||
               (destFormat == Bits32_Rgb) ||
               (destFormat == Bits24_Rgb))
            {
                if(amtRed)
                    amtRed = ((amtRed / 0x1F) * 0xFF) & 0xFF;
                if(amtGreen)
                    amtGreen = ((amtGreen / 0x1F) * 0xFF) & 0xFF;
                if(amtBlue)
                    amtBlue = ((amtBlue / 0x1F) * 0xFF) & 0xFF;
                
                dest = (amtRed << 16) | (amtGreen << 8) | (amtBlue);
                
                if(destFormat == Bits32_Argb)
                    dest |= 0xFF000000;
                
                return true;
            }
            else if(destFormat == Bits16_Rgb565)
            {
                if(amtRed)
                    amtRed = ((amtRed / 0x1F) * 0x1F) & 0x1F;
                if(amtGreen)
                    amtGreen = ((amtGreen / 0x3F) * 0x3F) & 0x3F;
                if(amtBlue)
                    amtBlue = ((amtBlue / 0x1F) * 0x1F) & 0x1F;
                
                dest = (amtRed << 11) | (amtGreen << 5) | (amtBlue);
                
                return true;
            }
            else if(destFormat == Bits16_Argb)
            {
                if(amtRed)
                    amtRed = ((amtRed / 0xF) * 0xF) & 0xF;
                if(amtGreen)
                    amtGreen = ((amtGreen / 0xF) * 0xF) & 0xF;
                if(amtBlue)
                    amtBlue = ((amtBlue / 0xF) * 0xF) & 0xF;
                
                dest = (0xF << 12) | (amtRed << 8) | (amtGreen << 4) | (amtBlue);
                
                return true;
            }
            break;
        case Bits8_Idx:
            break;
        default:
            break;
    }
    
    return false;
}

void Framebuffer::swBlit(void *pBuffer, size_t srcx, size_t srcy, size_t destx,
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
    
    // Sanity check and clip
    /// \todo Buffer dimension check
    if((destx > currMode.width) || (desty > currMode.height))
        return;
    if((destx + width) > currMode.width)
        width = (destx + width) - currMode.width;
    if((desty + height) > currMode.height)
        height = (desty + height) - currMode.height;
    
    // Blit across the width of the screen? How handy!
    if(UNLIKELY((!(srcx && destx)) && (width == currMode.width)))
    {
        size_t sourceBufferOffset = (srcy * sourceBytesPerLine) + (srcx * bytesPerPixel);
        size_t frameBufferOffset = (desty * bytesPerLine) + (destx * bytesPerPixel);
        
        void *dest = reinterpret_cast<void*>(m_FramebufferBase + frameBufferOffset);
        void *src = adjust_pointer(pBuffer, sourceBufferOffset);
        
        memmove(dest, src, width * bytesPerPixel);
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
            
            memmove(dest, src, width * bytesPerPixel);
        }
    }
}

void Framebuffer::swRect(size_t x, size_t y, size_t width, size_t height, uint32_t colour, PixelFormat format)
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
    convertPixel(colour, format, transformColour, Bits32_Argb);
    
    size_t bytesPerPixel = currMode.pf.nBpp / 8;
    size_t bytesPerLine = bytesPerPixel * currMode.width; /// \todo Not always the right calculation
    
    // Can we just do an easy memset?
    if(UNLIKELY((!x) && (width == currMode.width)))
    {
        size_t frameBufferOffset = (y * bytesPerLine) + (x * bytesPerPixel);
        
        void *dest = reinterpret_cast<void*>(m_FramebufferBase + frameBufferOffset);
        
        dmemset(dest, transformColour, (width * bytesPerPixel) / 4);
    }
    else
    {
        // Line-by-line fill
        for(size_t desty = y; desty < (y + height); desty++)
        {
            size_t frameBufferOffset = (desty * bytesPerLine) + (x * bytesPerPixel);
        
            void *dest = reinterpret_cast<void*>(m_FramebufferBase + frameBufferOffset);
            
            dmemset(dest, transformColour, (width * bytesPerPixel) / 4);
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
    
    size_t bytesPerPixel = currMode.pf.nBpp / 8;
    size_t bytesPerLine = bytesPerPixel * currMode.width; /// \todo Not always the right calculation
    size_t sourceBytesPerLine = w * bytesPerPixel; /// \todo Pass a struct for the buffer with full buffer width
    
    // Easy memcpy?
    if(UNLIKELY(((!srcx) && (!destx)) && (w == currMode.width)))
    {
        size_t frameBufferOffsetSrc = (srcy * bytesPerLine) + (srcx * bytesPerPixel);
        size_t frameBufferOffsetDest = (desty * bytesPerLine) + (destx * bytesPerPixel);
        
        void *dest = reinterpret_cast<void*>(m_FramebufferBase + frameBufferOffsetDest);
        void *src = reinterpret_cast<void*>(m_FramebufferBase + frameBufferOffsetSrc);
        
        memmove(dest, src, (w * h) * bytesPerPixel);
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

