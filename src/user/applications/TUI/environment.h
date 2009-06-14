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

#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include <stdint.h>
#include <stdlib.h>

namespace Keyboard
{
    static const uint64_t Special = 1ULL<<63;
    static const uint64_t Alt     = 1ULL<<62;
    static const uint64_t AltGr   = 1ULL<<61;
    static const uint64_t Ctrl    = 1ULL<<60;
    static const uint64_t Shift   = 1ULL<<59;
}

namespace Display
{

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
    };
};

extern void log(const char *);

#endif
