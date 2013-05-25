/*
 * Copyright (c) 2008 James Molloy, Jörg Pfähler, Matthew Iselin
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
#ifndef MACHINE_DISPLAY_H
#define MACHINE_DISPLAY_H

#include <compiler.h>
#include <machine/Device.h>
#include <utilities/List.h>

#include <machine/Framebuffer.h>

/**
 * A display is either a dumb framebuffer or something more accelerated.
 *
 * The general use case for writing to the screen:
 *   * Create a buffer with "newBuffer()".
 *   * Set the buffer as the main on-screen buffer with "setCurrentBuffer()".
 *   * Write to it, then update the changes to the screen with "updateBuffer()". Optionally here
 *     you can define a rectangle to be updated, usually referred to as a "dirty rectangle".
 *   * Optionally, you can use the faster acceleration functions "fillRectangle" and "bitBlit".
 *     Note that these functions may not actually be accelerated on-chip, this is up to the driver;
 *     however they are likely to be much faster than updating the buffer manually and calling updateBuffer().
 */
class Display : public Device
{
public:

    /** Describes a single RGBA 32-bit pixel. */
    struct rgb_t
    {
        uint8_t r, g, b, a;
    } PACKED;

    /** Describes the format of a pixel in a buffer. */
    struct PixelFormat
    {
        uint8_t mRed;       ///< Red mask.
        uint8_t pRed;       ///< Position of red field.
        uint8_t mGreen;     ///< Green mask.
        uint8_t pGreen;     ///< Position of green field.
        uint8_t mBlue;      ///< Blue mask.
        uint8_t pBlue;      ///< Position of blue field.
        uint8_t mAlpha;     ///< Alpha mask.
        uint8_t pAlpha;     ///< Position of the alpha field.
        uint8_t nBpp;       ///< Bits per pixel (total).
        uint32_t nPitch;    ///< Bytes per scanline.
    };

    /** Describes a screen mode / resolution */
    struct ScreenMode
    {
        uint32_t id;
        uint32_t width;
        uint32_t height;
        uint32_t refresh;
        uintptr_t framebuffer;
        PixelFormat pf;
        
        Graphics::PixelFormat pf2;
        uint32_t bytesPerLine;
        uint32_t bytesPerPixel;
    };

    Display()
    {
        m_SpecificType = "Generic Display";
    }
    Display(Device *p) :
        Device(p)
    {
    }
    virtual ~Display()
    {
    }

    virtual Type getType()
    {return Device::Display;}

    virtual void getName(String &str)
    {str = "Generic Display";}

    virtual void dump(String &str)
    {str = "Generic Display";}

    /** Returns a pointer to a linear framebuffer.
        Returns 0 if none available.
        \warning Not intended to be used - use the buffer functions instead. */
    virtual void *getFramebuffer()
    {return 0;}

    /** Returns a new back buffer. This is the primary method for drawing to the screen.
        The buffer returned is in RGB 24-bit format and is the width and height of the screen. */
    virtual rgb_t *newBuffer()
    {return 0;}

    /** Sets the currently viewed buffer to pBuffer. */
    virtual void setCurrentBuffer(rgb_t *pBuffer)
    {}

    /** Updates the contents of pBuffer to the screen, or more specifically to video memory. If this buffer
        is not current (i.e. via a call to setCurrentBuffer) the screen content will obviously not change.

        Optionally a rectangle can be provided - only this region will be updated. */
    virtual void updateBuffer(rgb_t *pBuffer, size_t x1=~0UL, size_t y1=~0UL, size_t x2=~0UL, size_t y2=~0UL)
    {}

    /** If a buffer is no longer being used, it can be deleted using this function. */
    virtual void killBuffer(rgb_t *pBuffer)
    {}

    /** Blits data from [fromX,fromY] to [toX,toY], with the given width and height. */
    virtual void bitBlit(rgb_t *pBuffer, size_t fromX, size_t fromY, size_t toX, size_t toY, size_t width, size_t height)
    {}

    /** Fills the rectangle from [x,y] extending by [width,height] with the given colour. */
    virtual void fillRectangle(rgb_t *pBuffer, size_t x, size_t y, size_t width, size_t height, rgb_t colour)
    {}

    /** Returns the format of each pixel in the framebuffer, along with the bits-per-pixel.
        \return True if operation succeeded, false otherwise. */
    virtual bool getPixelFormat(PixelFormat &pf)
    {return false;}

    /** Returns the current screen mode.
        \return True if operation succeeded, false otherwise. */
    virtual bool getCurrentScreenMode(ScreenMode &sm)
    {return false;}

    /** Fills the given List with all of the available screen modes.
        \return True if operation succeeded, false otherwise. */
    virtual bool getScreenModes(List<ScreenMode*> &sms)
    {return false;}

    /** Sets the current screen mode.
        \return True if operation succeeded, false otherwise. */
    virtual bool setScreenMode(ScreenMode sm)
    {return false;}
    
    /** Sets the current screen mode using a set of well-defined VBE IDs.
     *  Example: 0x117 = 1024x768x16. */
    virtual bool setScreenMode(size_t modeId)
    {
        Display::ScreenMode *pSm = 0;
        
        List<Display::ScreenMode*> modes;
        if(!getScreenModes(modes))
            return false;
        for(List<Display::ScreenMode*>::Iterator it = modes.begin();
            it != modes.end();
            it++)
        {
            if ((*it)->id == modeId)
            {
                pSm = *it;
                break;
            }
        }
        if (pSm == 0)
        {
            ERROR("Screenmode not found: " << modeId);
            return false;
        }

        return setScreenMode(*pSm);
    }
    
    /** Sets the current screen mode using a width, height, and pixel depth */
    virtual bool setScreenMode(size_t nWidth, size_t nHeight, size_t nBpp)
    {
        // This default implementation is enough for VBE
        /// \todo "Closest match": allow a threshold for a match in case the
        ///       specific mode specified cannot be set.
        
        Display::ScreenMode *pSm = 0;
        
        List<Display::ScreenMode*> modes;
        if(!getScreenModes(modes))
            return false;
        for(List<Display::ScreenMode*>::Iterator it = modes.begin();
            it != modes.end();
            it++)
        {
            if (((*it)->width == nWidth) &&
                ((*it)->height == nHeight))
            {
                if(/*(Graphics::bitsPerPixel((*it)->pf2) == nBpp) ||*/ (((*it)->pf.nBpp) == nBpp))
                {
                    pSm = *it;
                    break;
                }
            }
        }
        if (pSm == 0)
        {
            ERROR("Screenmode not found: " << Dec << nWidth << "x" << nHeight << "x" << nBpp << Hex);
            return false;
        }

        return setScreenMode(*pSm);
    }
};

#endif
