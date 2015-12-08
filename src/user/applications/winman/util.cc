/*
 * Copyright (c) 2008-2014, Pedigree Developers
 *
 * Please see the CONTRIB file in the root of the source tree for a full
 * list of contributors.
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

#include <fcntl.h>
#include <syslog.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <native/graphics/Graphics.h>

#include "util.h"

Framebuffer::Framebuffer() : m_pFramebuffer(0), m_FramebufferSize(0),
    m_Format(), m_Width(0), m_Height(0), m_Fb(-1),
    m_bStoredMode(false), m_StoredMode()
{
}

Framebuffer::~Framebuffer()
{
    if (m_pFramebuffer)
    {
        munmap(m_pFramebuffer, m_FramebufferSize);
        m_pFramebuffer = 0;
        m_FramebufferSize = 0;
    }

    if (m_Fb >= 0)
    {
        close(m_Fb);
        m_Fb = -1;
    }
}

bool Framebuffer::initialise()
{
    // Grab a framebuffer to use.
    m_Fb = open("/dev/fb", O_RDWR);
    if(m_Fb < 0)
    {
        syslog(LOG_INFO, "winman: no framebuffer device");
        fprintf(stderr, "winman: couldn't open framebuffer device");
        return false;
    }

    return true;
}

void Framebuffer::storeMode()
{
    if (m_bStoredMode)
        return;

    // Grab the current mode so we can restore it if we die.
    pedigree_fb_mode current_mode;
    int result = ioctl(m_Fb, PEDIGREE_FB_GETMODE, &current_mode);
    if(result == 0)
    {
        m_bStoredMode = true;
        m_StoredMode = current_mode;
    }
}

void Framebuffer::restoreMode()
{
    if (!m_bStoredMode)
        return;

    // Restore old graphics mode.
    pedigree_fb_modeset old_mode = {m_StoredMode.width,
                                    m_StoredMode.height,
                                    m_StoredMode.depth};
    ioctl(m_Fb, PEDIGREE_FB_SETMODE, &old_mode);

    m_bStoredMode = false;
}

int Framebuffer::enterMode(size_t desiredW, size_t desiredH, size_t desiredBpp)
{
    // All good, framebuffer already exists.
    if (m_pFramebuffer)
        return 0;

    // Can we set the graphics mode we want?
    pedigree_fb_modeset mode = {desiredW, desiredH, desiredBpp};
    int result = ioctl(m_Fb, PEDIGREE_FB_SETMODE, &mode);
    if(result < 0)
    {
        // No! Bad!
        /// \note Mode set logic will try and find a mode in a lower colour depth
        ///       if the desired one cannot be set.
        syslog(LOG_INFO, "winman: can't set the desired mode");
        fprintf(stderr, "winman: could not set desired mode (%dx%d) in any colour depth.\n", mode.width, mode.height);
        return EXIT_FAILURE;
    }

    pedigree_fb_mode set_mode;
    result = ioctl(m_Fb, PEDIGREE_FB_GETMODE, &set_mode);
    if(result < 0)
    {
        syslog(LOG_INFO, "winman: can't get mode info");
        fprintf(stderr, "winman: could not get mode information after setting mode.\n");

        // Back to text.
        memset(&mode, 0, sizeof(mode));
        ioctl(m_Fb, PEDIGREE_FB_SETMODE, &mode);
        return EXIT_FAILURE;
    }

    m_Width = set_mode.width;
    m_Height = set_mode.height;

    m_Format = CAIRO_FORMAT_ARGB32;
    if(set_mode.format == PedigreeGraphics::Bits24_Rgb)
    {
        if(set_mode.bytes_per_pixel != 4)
        {
            fprintf(stderr, "winman: error: incompatible framebuffer format (bytes per pixel)\n");
            return EXIT_FAILURE;
        }
    }
    else if(set_mode.format == PedigreeGraphics::Bits16_Rgb565)
    {
        m_Format = CAIRO_FORMAT_RGB16_565;
    }
    else if(set_mode.format > PedigreeGraphics::Bits32_Rgb)
    {
        fprintf(stderr, "winman: error: incompatible framebuffer format (possibly BGR or similar)\n");
        return EXIT_FAILURE;
    }

    int stride = cairo_format_stride_for_width(m_Format, set_mode.width);

    // Map the framebuffer in to our address space.
    syslog(LOG_INFO, "Mapping /dev/fb in (sz=%x)...", stride * set_mode.height);
    m_pFramebuffer = mmap(
        0,
        stride * set_mode.height,
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        m_Fb,
        0);
    syslog(LOG_INFO, "Got %p...", m_pFramebuffer);

    if(m_pFramebuffer == MAP_FAILED)
    {
        syslog(LOG_CRIT, "winman: couldn't map framebuffer into address space");
        return EXIT_FAILURE;
    }
    else
    {
        syslog(LOG_INFO, "winman: mapped framebuffer at %p", m_pFramebuffer);
    }

    m_FramebufferSize = stride * set_mode.height;

    return 0;
}

void Framebuffer::flush(size_t x, size_t y, size_t w, size_t h)
{
    // Submit a redraw to the graphics card.
    pedigree_fb_rect fbdirty = {x, y, w, h};
    ioctl(m_Fb, PEDIGREE_FB_REDRAW, &fbdirty);
}

SharedBuffer::SharedBuffer(size_t size) : m_pFramebuffer(0)
{
    m_pFramebuffer = new PedigreeIpc::SharedIpcMessage(size, 0);
    m_pFramebuffer->initialise();
}

SharedBuffer::SharedBuffer(size_t size, void *handle) : m_pFramebuffer(0)
{
    m_pFramebuffer = new PedigreeIpc::SharedIpcMessage(size, handle);
    m_pFramebuffer->initialise();
}

SharedBuffer::~SharedBuffer()
{
    /// \todo Need a way to destroy the old framebuffer without freeing the
    ///       shared region. Refcount on the region perhaps?
    // delete m_Framebuffer;
}

void *SharedBuffer::getHandle()
{
    return m_pFramebuffer->getHandle();
}

void *SharedBuffer::getBuffer()
{
    return m_pFramebuffer->getBuffer();
}
