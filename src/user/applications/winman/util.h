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

#ifndef _WINMAN_UTIL_H
#define _WINMAN_UTIL_H

#ifndef TARGET_LINUX
#include <sys/fb.h>

#include <ipc/Ipc.h>
#else
#include <SDL/SDL.h>
#endif

#include <cairo/cairo.h>

/**
 * \brief Abstracts the system's framebuffer offering.
 *
 * This is needed to abstract out the Pedigree-specific framebuffer parts so
 * the window manager can be run under SDL on Linux for rapid development.
 */
class Framebuffer
{
    public:
        Framebuffer();
        virtual ~Framebuffer();

        /** General system-specific initialisation. */
        bool initialise();

        /** Store current mode (before switching). */
        void storeMode();

        /** Restore saved mode. */
        void restoreMode();

        /** Create framebuffer, enter desired mode. */
        int enterMode(size_t desiredW, size_t desiredH, size_t desiredBpp);

        /** Retrieve base address of the framebuffer. */
        void *getFramebuffer() const
        {
            return m_pFramebuffer;
        }

        /** Flush framebuffer in the given region. */
        void flush(size_t x, size_t y, size_t w, size_t h);

        /** Get framebuffer colour format. */
        cairo_format_t getFormat() const
        {
            return m_Format;
        }

        /** Get framebuffer dimensions. */
        size_t getWidth() const
        {
            return m_Width;
        }
        size_t getHeight() const
        {
            return m_Height;
        }

    private:
        void *m_pFramebuffer;
        size_t m_FramebufferSize;

        cairo_format_t m_Format;

        size_t m_Width;
        size_t m_Height;

#ifdef TARGET_LINUX
        SDL_Surface *m_pScreen;
        SDL_Surface *m_pBackbuffer;
#else
        int m_Fb;

        bool m_bStoredMode;
        pedigree_fb_mode m_StoredMode;
#endif
};

/**
 * \brief Abstracts a buffer shared between multiple processes.
 *
 * Pedigree uses shared IPC messages for this purpose, while Linux uses
 * actual Linux shmem regions and passes file descriptors around.
 */
class SharedBuffer
{
    public:
        SharedBuffer(size_t size);
        SharedBuffer(size_t size, void *handle);

        virtual ~SharedBuffer();

        /** Retrieve the memory address of the buffer. */
        void *getBuffer();

        /** Retrieve a handle that can used create a matching SharedBuffer */
        void *getHandle();

#ifdef TARGET_LINUX
        char m_ShmName[8];
        int m_ShmFd;

        void *m_pBuffer;
        size_t m_Size;

        static size_t m_NextId;
#else
        PedigreeIpc::SharedIpcMessage *m_pFramebuffer;
#endif
};

#endif
