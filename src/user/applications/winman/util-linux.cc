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

#ifdef TARGET_LINUX

#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <syslog.h>
#include <errno.h>

#include <SDL/SDL.h>

#include "util.h"

Framebuffer::Framebuffer() : m_pFramebuffer(0), m_FramebufferSize(0),
    m_Format(), m_Width(0), m_Height(0), m_pScreen(0), m_pBackbuffer(0)
{
}

Framebuffer::~Framebuffer()
{
    // ...
}

bool Framebuffer::initialise()
{
    return true;
}

void Framebuffer::storeMode()
{
    // no-op for SDL
}

void Framebuffer::restoreMode()
{
    // no-op for SDL
}

int Framebuffer::enterMode(size_t desiredW, size_t desiredH, size_t desiredBpp)
{
    m_pScreen = SDL_SetVideoMode(desiredW, desiredH, desiredBpp,
        SDL_DOUBLEBUF | SDL_SWSURFACE);
    /// \todo handle SDL_SetVideoMode failure.

    m_pBackbuffer = SDL_CreateRGBSurface(
        SDL_DOUBLEBUF | SDL_SWSURFACE, desiredW, desiredH, 32,
        0x00FF0000,
        0x0000FF00,
        0x000000FF,
        0
    );

    m_pFramebuffer = (void *) m_pBackbuffer->pixels;

    m_Width = m_pScreen->w;
    m_Height = m_pScreen->h;
    m_Format = CAIRO_FORMAT_RGB24;

    return 0;
}

void Framebuffer::flush(size_t x, size_t y, size_t w, size_t h)
{
    // Don't bother doing a dirty rectangle flush just yet.
    SDL_BlitSurface(m_pBackbuffer, NULL, m_pScreen, NULL);
    SDL_Flip(m_pScreen);
}

SharedBuffer::SharedBuffer(size_t size) : m_ShmName(), m_ShmFd(-1),
    m_pBuffer(0), m_Size(size)
{
    memset(m_ShmName, 0, sizeof m_ShmName);
    strcpy(m_ShmName, "wm_x");

    m_ShmFd = shm_open("wm_x", O_RDWR | O_CREAT, 0777);
    fprintf(stderr, "shm fd=%d [%s]\n", m_ShmFd, strerror(errno));
    ftruncate(m_ShmFd, size);

    m_pBuffer = mmap(
        0,
        size,
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        m_ShmFd,
        0);
}

SharedBuffer::SharedBuffer(size_t size, void *handle) : m_ShmName(),
    m_ShmFd(-1), m_pBuffer(0), m_Size(size)
{
    /// \todo force null-termination
    memcpy(m_ShmName, &handle, 8);

    m_ShmFd = shm_open(m_ShmName, O_RDWR, 0777);

    m_pBuffer = mmap(
        0,
        size,
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        m_ShmFd,
        0);
}

SharedBuffer::~SharedBuffer()
{
    munmap(m_pBuffer, m_Size);
    close(m_ShmFd);
    shm_unlink(m_ShmName);
}

void *SharedBuffer::getHandle()
{
    uint64_t v = 0;
    memcpy(&v, m_ShmName, 8);
    return (void *) v;
}

void *SharedBuffer::getBuffer()
{
    return m_pBuffer;
}

#endif
