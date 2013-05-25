/*
 * Copyright (c) 2010 Matthew Iselin
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
#include <graphics/Graphics.h>
#include <machine/Framebuffer.h>

Framebuffer *Graphics::createFramebuffer(Framebuffer *pParent,
                                         size_t x, size_t y,
                                         size_t w, size_t h,
                                         void *pFbOverride)
{
    if(!(w && h))
        return 0;

    // Don't allow insane placement, but do allow "offscreen" buffers.
    // They may be moved later, at which point they will be "onscreen"
    // buffers.
    if((x > pParent->getWidth()) || (y > pParent->getHeight()))
        return 0;
    
    // Every framebuffer in the system uses the same framebuffer format - that
    // of the graphics card itself.
    Graphics::PixelFormat format = pParent->getFormat();

    size_t bytesPerPixel = pParent->getBytesPerPixel();
    size_t bytesPerLine = bytesPerPixel * w;

    // Allocate space for the buffer
    uint8_t *pMem = 0;
    if(!pFbOverride)
        pMem = new uint8_t[bytesPerLine * h];
    else
        pMem = reinterpret_cast<uint8_t*>(pFbOverride);
    NOTICE("pMem: " << ((uintptr_t) pMem) << " ov=" << (pFbOverride ? "yes" : "no"));

    // Create the framebuffer
    Framebuffer *pRet = new Framebuffer();
    pRet->setFramebuffer(reinterpret_cast<uintptr_t>(pMem));
    pRet->setWidth(w);
    pRet->setHeight(h);
    pRet->setFormat(format);
    pRet->setBytesPerPixel(bytesPerPixel);
    pRet->setBytesPerLine(bytesPerLine);
    pRet->setXPos(x);
    pRet->setYPos(y);
    pRet->m_pParent = pParent;
    return pRet;
}

void Graphics::destroyFramebuffer(Framebuffer *pFramebuffer)
{
    if((!pFramebuffer) || (!pFramebuffer->getRawBuffer()))
        return;

    // Let the parent redraw itself onto this region
    if(pFramebuffer->getParent())
        pFramebuffer->redraw(0, 0, pFramebuffer->getWidth(), pFramebuffer->getHeight(), false);

    // Delete our framebuffer memory
    /// \todo Need a flag to know that this address isn't on the heap!
    // delete [] reinterpret_cast<uint8_t*>(pFramebuffer->getRawBuffer());

    // Finally delete the framebuffer object
    delete pFramebuffer;
}

