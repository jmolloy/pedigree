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

#include "VbeDisplay.h"
#include <Log.h>
#include <machine/x86_common/Bios.h>
#include <machine/Vga.h>
#include <machine/Machine.h>
#include <processor/PhysicalMemoryManager.h>

#include <config/Config.h>
#include <utilities/utility.h>

#include <graphics/Graphics.h>

/// \todo Put this in the config manager.
Display::ScreenMode g_ScreenMode;
Display *g_pDisplay = 0;
uintptr_t g_Framebuffer;
size_t g_FbSize;

VbeDisplay::VbeDisplay() : m_VbeVersion(), m_ModeList(), m_Mode(), m_pFramebuffer(), m_Buffers(), m_SpecialisedMode(Mode_Generic), m_Allocator()
{

}

VbeDisplay::VbeDisplay(Device *p, VbeVersion version, List<Display::ScreenMode*> &sms, size_t vidMemSz, size_t displayNum) :
    Display(p), m_VbeVersion(version), m_ModeList(sms), m_Mode(), m_pFramebuffer(0),
    m_Buffers(), m_SpecialisedMode(Mode_Generic), m_Allocator()
{
    String str;
    str.sprintf("DELETE FROM 'display_modes' where display_id = %d", displayNum);
    Config::Result *pR = Config::instance().query(str);
    if(!pR)
    {
        FATAL("VBE: Couldn't get a result.");
        return;
    }
    if (!pR->succeeded())
    {
        FATAL("VbeDisplay: Sql error: " << pR->errorMessage());
        return;
    }
    delete pR;

  uintptr_t fbAddr = 0;

  for (List<Display::ScreenMode*>::Iterator it = m_ModeList.begin();
       it != m_ModeList.end();
       it++)
  {
      str.sprintf("INSERT INTO 'display_modes' VALUES (NULL, %d,%d,%d,%d,%d,%d)", (*it)->id, displayNum, (*it)->width, (*it)->height, (*it)->pf.nBpp, (*it)->refresh);
      pR = Config::instance().query(str);

      if (!pR->succeeded())
      {
          FATAL("VbeDisplay: Sql error: " << pR->errorMessage());
          return;
      }
      
      fbAddr = (*it)->framebuffer;

      delete pR;
  }

  m_Allocator.free(0, vidMemSz);

    // Assumes the same framebuffer for all modes
    bool bFramebufferFound = false;
    for (Vector<Device::Address*>::Iterator it = m_Addresses.begin();
        it != m_Addresses.end();
        it++)
    {
        uintptr_t address = (*it)->m_Address;
        size_t size = (*it)->m_Size;
        if (address <= fbAddr && (address+size) > fbAddr)
        {
            m_pFramebuffer = static_cast<MemoryMappedIo*> ((*it)->m_Io);
            m_pFramebufferRawAddress = (*it);
            bFramebufferFound = true;
            break;
        }
    }
    
    if(!bFramebufferFound)
        ERROR("No PCI MMIO region found for the desired video mode.");
}

VbeDisplay::~VbeDisplay()
{
}

void *VbeDisplay::getFramebuffer()
{
  return reinterpret_cast<void*> (m_pFramebuffer->virtualAddress());
}

bool VbeDisplay::getPixelFormat(Display::PixelFormat *pPf)
{
  memcpy(pPf, &m_Mode.pf, sizeof(Display::PixelFormat));
  return true;
}

bool VbeDisplay::getCurrentScreenMode(Display::ScreenMode &sm)
{
  sm = m_Mode;
  return true;
}

bool VbeDisplay::getScreenModes(List<Display::ScreenMode*> &sms)
{
  sms = m_ModeList;
  return true;
}

bool VbeDisplay::setScreenMode(size_t modeId)
{
  Display::ScreenMode *pSm = 0;

  for (List<Display::ScreenMode*>::Iterator it = m_ModeList.begin();
       it != m_ModeList.end();
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

bool VbeDisplay::setScreenMode(Display::ScreenMode sm)
{
    m_Mode = sm;

    if (m_Mode.pf.nBpp == 16 &&
        m_Mode.pf.pRed == 11 &&
        m_Mode.pf.mRed == 5 &&
        m_Mode.pf.pGreen == 5 &&
        m_Mode.pf.mGreen == 6 &&
        m_Mode.pf.pBlue == 0 &&
        m_Mode.pf.mBlue == 5)
    {
        m_Mode.pf2 = Graphics::Bits16_Rgb565;
        m_SpecialisedMode = Mode_16bpp_5r6g5b;
    }
    else if (m_Mode.pf.nBpp == 16 &&
        m_Mode.pf.pRed == 10 &&
        m_Mode.pf.mRed == 5 &&
        m_Mode.pf.pGreen == 5 &&
        m_Mode.pf.mGreen == 5 &&
        m_Mode.pf.pBlue == 0 &&
        m_Mode.pf.mBlue == 5)
    {
        m_Mode.pf2 = Graphics::Bits16_Rgb555;
    }
    else if (m_Mode.pf.nBpp == 24 && m_Mode.pf.pBlue == 0)
    {
        m_Mode.pf2 = Graphics::Bits24_Rgb;
        m_SpecialisedMode = Mode_24bpp_8r8g8b;
    }
    else if (m_Mode.pf.nBpp == 24 && m_Mode.pf.pBlue > 0)
    {
        /// \todo Assumption may be wrong?
        m_Mode.pf2 = Graphics::Bits24_Bgr;
    }
    else if (m_Mode.pf.nBpp == 32)
    {
        m_Mode.pf2 = Graphics::Bits32_Argb;
    }
    else
    {
        m_Mode.pf2 = Graphics::Bits16_Argb;
        m_SpecialisedMode = Mode_Generic;
    }
    
    m_Mode.bytesPerPixel = bytesPerPixel(m_Mode.pf2);
    m_Mode.bytesPerLine = m_Mode.bytesPerPixel * m_Mode.width;

    // Invalidate all current buffers.
    for (Tree<rgb_t*,Buffer*>::Iterator it = m_Buffers.begin();
         it != m_Buffers.end();
         it++)
    {
        Buffer *pBuf = it.value();
        pBuf->valid = false;
    }

    // SET SuperVGA VIDEO MODE - AX=4F02h, BX=new mode
    Bios::instance().setAx (0x4F02);
    Bios::instance().setBx (m_Mode.id | (1<<14));
    Bios::instance().setEs (0x0000);
    Bios::instance().setDi (0x0000);
    Bios::instance().executeInterrupt (0x10);

    // Check the signature.
    if (Bios::instance().getAx() != 0x004F)
    {
        ERROR("VBE: Set mode failed! (mode " << Hex << m_Mode.id << ")");
        return false;
    }
    NOTICE("VBE: Set mode " << m_Mode.id);

    // Tell the Machine instance what VBE mode we're in, so it can set it again if we enter the debugger and return.
    Machine::instance().getVga(0)->setMode(m_Mode.id);
    
    m_pLogicalFramebuffer->setWidth(m_Mode.width);
    m_pLogicalFramebuffer->setHeight(m_Mode.height);
    m_pLogicalFramebuffer->setBytesPerPixel(m_Mode.bytesPerPixel);
    m_pLogicalFramebuffer->setBytesPerLine(m_Mode.bytesPerLine);
    m_pLogicalFramebuffer->setFormat(m_Mode.pf2);
    m_pLogicalFramebuffer->setXPos(0); m_pLogicalFramebuffer->setYPos(0);
    m_pLogicalFramebuffer->setParent(0);
    
    m_pFramebufferRawAddress->map((m_Mode.height * m_Mode.bytesPerLine) + m_Mode.width, true);
    m_pLogicalFramebuffer->setFramebuffer(reinterpret_cast<uintptr_t>(m_pFramebuffer->virtualAddress()));

    g_pDisplay = this;
    g_ScreenMode = m_Mode;
    g_Framebuffer = reinterpret_cast<uintptr_t>(m_pFramebuffer->virtualAddress());
    g_FbSize = m_pFramebuffer->size();

    return true;
}

VbeDisplay::rgb_t *VbeDisplay::newBuffer()
{
    Buffer *pBuffer = new Buffer();

    size_t pgmask = PhysicalMemoryManager::getPageSize()-1;
    size_t sz = m_Mode.width*m_Mode.height * sizeof(rgb_t);

    if (sz & pgmask) sz += PhysicalMemoryManager::getPageSize();
    sz &= ~pgmask;

    if (!PhysicalMemoryManager::instance().allocateRegion(pBuffer->mr,
                                                          sz / PhysicalMemoryManager::getPageSize(),
                                                          0,
                                                          VirtualAddressSpace::Write,
                                                          -1))
    {
        ERROR("VbeDisplay::newBuffer: allocateRegion failed!");
    }

    pBuffer->pBackbuffer = reinterpret_cast<rgb_t*>(pBuffer->mr.virtualAddress());

    sz = m_Mode.width*m_Mode.height * (m_Mode.pf.nBpp/8);
    if (sz & pgmask) sz += PhysicalMemoryManager::getPageSize();
    sz &= ~pgmask;

    if (!PhysicalMemoryManager::instance().allocateRegion(pBuffer->fbmr,
                                                          sz / PhysicalMemoryManager::getPageSize(),
                                                          0,
                                                          VirtualAddressSpace::Write,
                                                          -1))
    {
        ERROR("VbeDisplay::newBuffer: allocateRegion failed! (1)");
    }

    pBuffer->pFbBackbuffer = reinterpret_cast<uint8_t*>(pBuffer->fbmr.virtualAddress());

    m_Buffers.insert(pBuffer->pBackbuffer, pBuffer);

    return pBuffer->pBackbuffer;
}

void VbeDisplay::setCurrentBuffer(rgb_t *pBuffer)
{
    Buffer *pBuf = m_Buffers.lookup(pBuffer);
    if (!pBuf || !pBuf->valid)
    {
        ERROR("VbeDisplay: Bad buffer:" << reinterpret_cast<uintptr_t>(pBuffer));
        return;
    }

    memcpy(getFramebuffer(), pBuf->pFbBackbuffer, m_Mode.width*m_Mode.height * (m_Mode.pf.nBpp/8));
}

void VbeDisplay::updateBuffer(rgb_t *pBuffer, size_t x1, size_t y1, size_t x2,
                              size_t y2)
{
    Buffer *pBuf = m_Buffers.lookup(pBuffer);
    if (!pBuf || !pBuf->valid)
    {
        ERROR("VbeDisplay: updateBuffer: Bad buffer:" << reinterpret_cast<uintptr_t>(pBuffer));
        return;
    }

    if (x1 == ~0UL) x1 = 0;
    if (x2 == ~0UL || x2 >= m_Mode.width) x2 = m_Mode.width-1;
    if (y1 == ~0UL) y1 = 0;
    if (y2 == ~0UL || y2 >= m_Mode.height) y2 = m_Mode.height-1;

    if (m_SpecialisedMode == Mode_16bpp_5r6g5b)
    {
        unsigned int x, y;


        unsigned int pitch = m_Mode.width;

        for (y = y1; y <= y2; y++)
        {
            for (x = x1; x < x2; x++)
            {
                rgb_t *pRgb = pBuffer + pitch*y + x;
                unsigned short r = (pRgb->r >> 3);
                unsigned short g = (pRgb->g >> 2);
                unsigned short b = (pRgb->b >> 3);
                unsigned short a = (r<<11) | (g<<5) | b;
                reinterpret_cast<uint16_t*>(pBuf->pFbBackbuffer)[pitch*y + x] = a;
                reinterpret_cast<uint16_t*>(getFramebuffer())[pitch*y + x] = a;
            }
        }

        return;
    }

    if (m_SpecialisedMode == Mode_24bpp_8r8g8b)
    {
        unsigned int x, y, i;

        uint8_t *pFb = reinterpret_cast<uint8_t*>(getFramebuffer());
        uint8_t *pFb2 = pBuf->pFbBackbuffer;

        for (y = y1; y <= y2; y++)
        {
            for (x = x1; x < x2; x++)
            {
                rgb_t *pRgb = pBuffer + m_Mode.width*y + x;
                i = (y*m_Mode.width + x) * 3;
                pFb[i + 0] = pRgb->b;
                pFb[i + 1] = pRgb->g;
                pFb[i + 2] = pRgb->r;
                pFb2[i + 0] = pRgb->b;
                pFb2[i + 1] = pRgb->g;
                pFb2[i + 2] = pRgb->r;
            }
        }

        return;
    }

    size_t bytesPerPixel = m_Mode.pf.nBpp/8;

    // Unoptimised version for arbitrary pixel formats.
    for (size_t y = y1; y <= y2; y++)
    {
        for (size_t x = x1; x <= x2; x++)
        {
            size_t i = y*m_Mode.width + x;
            packColour(pBuffer[i], i, reinterpret_cast<uintptr_t>(pBuf->pFbBackbuffer));
        }

        memcpy(reinterpret_cast<uint8_t*>(getFramebuffer())+y*m_Mode.pf.nPitch + x1*bytesPerPixel,
               pBuf->pFbBackbuffer + y*m_Mode.pf.nPitch + x1*bytesPerPixel,
               (x2-x1)*bytesPerPixel);
    }
}

void VbeDisplay::killBuffer(rgb_t *pBuffer)
{
    Buffer *pBuf = m_Buffers.lookup(pBuffer);
    if (!pBuf)
    {
        ERROR("VbeDisplay: killBuffer: Bad buffer:" << reinterpret_cast<uintptr_t>(pBuffer));
        return;
    }
    pBuf->mr.free();
    pBuf->fbmr.free();

    delete pBuf;

    m_Buffers.remove(pBuffer);
}

void VbeDisplay::bitBlit(rgb_t *pBuffer, size_t fromX, size_t fromY, size_t toX,
                         size_t toY, size_t width, size_t height)
{
    Buffer *pBuf = m_Buffers.lookup(pBuffer);
    if (!pBuf || !pBuf->valid)
    {
        ERROR("VbeDisplay: bitBlit: Bad buffer:" << reinterpret_cast<uintptr_t>(pBuffer));
        return;
    }

    size_t bytesPerPixel = m_Mode.pf.nBpp/8;

    uint8_t *pFb = pBuf->pFbBackbuffer;

    // Just like memmove(), if the dest < src, copy forwards, else copy backwards.
    size_t min = 0;
    size_t max = height;
    ssize_t increment = 1;
    if (fromY < toY)
    {
        min = height;
        max = 0;
        increment = -1;
    }

    if (toX == 0 && fromX == 0 && width == m_Mode.width)
    {
        size_t to = toY*m_Mode.width;
        size_t from = fromY*m_Mode.width;
        size_t sz = width*height;
        size_t extent_s, extent_e;
        if (from > to)
        {
            extent_s = to;
            extent_e = from+sz - extent_s;
        }
        else
        {
            extent_s = from;
            extent_e = to+sz - extent_s;
        }

        memmove(&pBuffer[to],
                &pBuffer[from],
                sz*sizeof(rgb_t));
        memmove(&pFb[to * bytesPerPixel],
                &pFb[from * bytesPerPixel],
                sz*bytesPerPixel);
        memcpy(reinterpret_cast<uint8_t*>(getFramebuffer())+ extent_s*bytesPerPixel,
               pBuf->pFbBackbuffer + extent_s*bytesPerPixel,
               extent_e*bytesPerPixel);
    }
    else
    {
        // Unoptimised bitblit. This could definately be made better.
        for (size_t y = min; y < max; y += increment)
        {
            size_t to = (y+toY)*m_Mode.width + toX;
            size_t from = (y+fromY)*m_Mode.width + fromX;
            memmove(&pBuffer[to],
                    &pBuffer[from],
                    width*sizeof(rgb_t));
            memmove(&pFb[to * bytesPerPixel],
                    &pFb[from * bytesPerPixel],
                    width*bytesPerPixel);
            memcpy(reinterpret_cast<uint8_t*>(getFramebuffer())+ to*bytesPerPixel,
                   pBuf->pFbBackbuffer + to*bytesPerPixel,
                   width*bytesPerPixel);
        }
    }
}

void VbeDisplay::fillRectangle(rgb_t *pBuffer, size_t x, size_t y, size_t width, size_t height, rgb_t colour)
{
    Buffer *pBuf = m_Buffers.lookup(pBuffer);
    if (!pBuf || !pBuf->valid)
    {
        ERROR("VbeDisplay: fillRect: Bad buffer:" << reinterpret_cast<uintptr_t>(pBuffer));
        return;
    }

    uint8_t *pFb = reinterpret_cast<uint8_t*>(getFramebuffer());
    uint8_t *pFb2 = pBuf->pFbBackbuffer;

    size_t bytesPerPixel = m_Mode.pf.nBpp/8;

    size_t compiledColour = 0;
    if (m_SpecialisedMode == Mode_16bpp_5r6g5b)
    {
        compiledColour = ((colour.r >> 3) << 11) |
            ((colour.g >> 2) << 5) |
            ((colour.b >> 3) << 0);
        for (size_t i = y; i < y+height; i++)
        {
            dmemset(&pBuffer[i*m_Mode.width+x], *reinterpret_cast<uint32_t*>(&colour), width);
            wmemset(&pFb[(i*m_Mode.width+x)*2], static_cast<uint16_t>(compiledColour), width);
            wmemset(&pFb2[(i*m_Mode.width+x)*2], static_cast<uint16_t>(compiledColour), width);
        }
        return;
    }
    else
        // Bit of a dirty hack. Oh well.
        packColour(colour, 0, reinterpret_cast<uintptr_t>(&compiledColour));

    for (size_t i = y; i < y+height; i++)
    {
        for (size_t j = x; j < x+width; j++)
        {
            pBuffer[i*m_Mode.width + j] = colour;
            switch (m_Mode.pf.nBpp)
            {
                case 15:
                case 16:
                {
                    uint16_t *pFb16 = reinterpret_cast<uint16_t*> (pFb);
                    uint16_t *pFb162 = reinterpret_cast<uint16_t*> (pFb2);
                    pFb16[i*m_Mode.width + j] = compiledColour&0xFFFF;
                    pFb162[i*m_Mode.width + j] = compiledColour&0xFFFF;
                    break;
                }
                case 24:
                    pFb[ (i*m_Mode.width + j) * 3 + 0] = colour.b;
                    pFb[ (i*m_Mode.width + j) * 3 + 1] = colour.g;
                    pFb[ (i*m_Mode.width + j) * 3 + 2] = colour.r;
                    pFb2[ (i*m_Mode.width + j) * 3 + 0] = colour.b;
                    pFb2[ (i*m_Mode.width + j) * 3 + 1] = colour.g;
                    pFb2[ (i*m_Mode.width + j) * 3 + 2] = colour.r;
                    break;
                default:
                    WARNING("VbeDisplay: Pixel format not handled in fillRectangle.");
            }
        }
    }
}

void VbeDisplay::packColour(rgb_t colour, size_t idx, uintptr_t pFb)
{
    PixelFormat pf = m_Mode.pf;

    uint8_t r = colour.r;
    uint8_t g = colour.g;
    uint8_t b = colour.b;

    // Calculate the range of the Red field.
    uint8_t range = 1 << pf.mRed;

    // Clamp the red value to this range.
    r = (r * range) / 256;

    range = 1 << pf.mGreen;

    // Clamp the green value to this range.
    g = (g * range) / 256;

    range = 1 << pf.mBlue;

    // Clamp the blue value to this range.
    b = (b * range) / 256;

    // Assemble the colour.
    uint32_t c =  0 |
        (static_cast<uint32_t>(r) << pf.pRed) |
        (static_cast<uint32_t>(g) << pf.pGreen) |
        (static_cast<uint32_t>(b) << pf.pBlue);

    switch (pf.nBpp)
    {
        case 15:
        case 16:
        {
            uint16_t *pFb16 = reinterpret_cast<uint16_t*>(pFb);
            pFb16[idx] = c;
            break;
        }
        case 24:
        {
            rgb_t *pFbRgb = reinterpret_cast<rgb_t*>(pFb);
            pFbRgb[idx].r = static_cast<uint32_t>(b);
            pFbRgb[idx].g = static_cast<uint32_t>(g);
            pFbRgb[idx].b = static_cast<uint32_t>(r);
            break;
        }
        case 32:
        {
            uint32_t *pFb32 = reinterpret_cast<uint32_t*>(pFb);
            pFb32[idx] = c;
            break;
        }
    }
}
