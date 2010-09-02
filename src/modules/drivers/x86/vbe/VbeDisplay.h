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
#ifndef VBE_DISPLAY_H
#define VBE_DISPLAY_H

#include <machine/Device.h>
#include <machine/Display.h>
#include <utilities/List.h>
#include <utilities/MemoryAllocator.h>
#include <processor/MemoryMappedIo.h>
#include <processor/PhysicalMemoryManager.h>

#include <machine/Framebuffer.h>

class VbeDisplay : public Display
{
public:
    /** VBE versions, in order. */
    enum VbeVersion
    {
        Vbe1_2,
        Vbe2_0,
        Vbe3_0
    };

    VbeDisplay();
    VbeDisplay(Device *p, VbeVersion version, List<Display::ScreenMode*> &sms, size_t vidMemSz, size_t displayNum);

    virtual ~VbeDisplay();

    virtual void *getFramebuffer();
    virtual bool getPixelFormat(Display::PixelFormat *pPf);
    virtual bool getCurrentScreenMode(Display::ScreenMode &sm);
    virtual bool getScreenModes(List<Display::ScreenMode*> &sms);
    virtual bool setScreenMode(Display::ScreenMode sm);
    virtual bool setScreenMode(size_t modeId);

    virtual rgb_t *newBuffer();
    virtual void setCurrentBuffer(rgb_t *pBuffer);
    virtual void updateBuffer(rgb_t *pBuffer, size_t x1=~0UL, size_t y1=~0UL,
                              size_t x2=~0UL, size_t y2=~0UL);
    virtual void killBuffer(rgb_t *pBuffer);
    virtual void bitBlit(rgb_t *pBuffer, size_t fromX, size_t fromY, size_t toX, size_t toY, size_t width, size_t height);
    virtual void fillRectangle(rgb_t *pBuffer, size_t x, size_t y, size_t width, size_t height, rgb_t colour);

    size_t getModeId()
    {
        return m_Mode.id;
    }
    
    virtual void setLogicalFramebuffer(Framebuffer *p)
    {
        m_pLogicalFramebuffer = p;
    }

private:
    /** Copy constructor is private. */
    VbeDisplay(const VbeDisplay &);
    VbeDisplay &operator = (const VbeDisplay &);

    void packColour(rgb_t colour, size_t idx, uintptr_t pFb);

    /** VBE version. */
    VbeVersion m_VbeVersion;

    /** Screen modes. */
    List<Display::ScreenMode*> m_ModeList;

    /** Current mode. */
    Display::ScreenMode m_Mode;

    MemoryMappedIo *m_pFramebuffer;
    Framebuffer *m_pLogicalFramebuffer;
    
    Device::Address *m_pFramebufferRawAddress;

    /** Possible display modes that we have specialised code for. */
    enum ModeType
    {
        Mode_16bpp_5r6g5b,
        Mode_24bpp_8r8g8b,
        Mode_Generic
    };

    /** Buffer format. */
    struct Buffer
    {
        Buffer() : pBackbuffer(0), pFbBackbuffer(0), mr("Buffer"), fbmr("Fb buffer"), valid(true) {}
        rgb_t        *pBackbuffer;
        uint8_t      *pFbBackbuffer;
        MemoryRegion  mr,fbmr;

        bool valid;

        private:
            Buffer(const Buffer&);
            const Buffer& operator = (const Buffer&);
    };
    /** Buffers. */
    Tree<rgb_t*, Buffer*> m_Buffers;

    /** Mode. */
    ModeType m_SpecialisedMode;

    /** Memory allocator for video memory. */
    MemoryAllocator m_Allocator;
};

#endif
