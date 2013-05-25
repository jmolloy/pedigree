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

#include "Png.h"

#include <syslog.h>
#include <unistd.h>

Png::Png(const char *filename) :
    m_PngPtr(0), m_InfoPtr(0), m_nWidth(0), m_nHeight(0), m_pRowPointers(0)
{
    // Open the file.
    FILE *stream = fopen(filename, "rb");
    if (!stream)
    {
        syslog(LOG_ALERT, "PNG file failed to open");
        return;
    }

    // Read in some of the signature bytes.
    char buf[4];
    if (fread(buf, 1, 4, stream) != 4)
    {
        syslog(LOG_ALERT, "PNG file failed to read ident");
        return;
    }
    if (png_sig_cmp(reinterpret_cast<png_byte*>(buf), 0, 4) != 0)
    {
        syslog(LOG_ALERT, "PNG file failed IDENT check");
        return;
    }

    m_PngPtr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
                                      0, 0, 0);

    if (m_PngPtr == 0)
    {
        syslog(LOG_ALERT, "PNG file failed to initialise");
        return;
    }

    m_InfoPtr = png_create_info_struct(m_PngPtr);
    if (m_InfoPtr == 0)
    {
        syslog(LOG_ALERT, "PNG info failed to initialise");
        return;
    }

    png_init_io(m_PngPtr, stream);

    png_set_sig_bytes(m_PngPtr, 4);

    png_set_palette_to_rgb(m_PngPtr);

    png_read_png(m_PngPtr, m_InfoPtr,
                 PNG_TRANSFORM_STRIP_16 | // 16-bit-per-channel down to 8.
                 PNG_TRANSFORM_STRIP_ALPHA | // No alpha
                 PNG_TRANSFORM_PACKING , // Unpack 2 and 4 bit samples.

                 reinterpret_cast<void*>(0));

    m_pRowPointers = reinterpret_cast<uint8_t**>(png_get_rows(m_PngPtr, m_InfoPtr));

    // Grab the info header information.
    int bit_depth, color_type, interlace_type, compression_type, filter_method;
    png_uint_32 w, h;
    png_get_IHDR(m_PngPtr, m_InfoPtr, &w, &h, &bit_depth, &color_type, &interlace_type, &compression_type, &filter_method);
    m_nWidth = w;
    m_nHeight = h;

    if (bit_depth != 8)
    {
        syslog(LOG_ALERT, "PNG - invalid bit depth");
        return;
    }
    if (color_type != PNG_COLOR_TYPE_RGB)
    {
        syslog(LOG_ALERT, "PNG - invalid colour type: %d", color_type);
        return;
    }

    syslog(LOG_ALERT, "PNG loaded %ul %ul", m_nWidth, m_nHeight);
}

Png::~Png()
{
}

void Png::render(rgb_t *pFb, size_t x, size_t y, size_t width, size_t height)
{
    for (size_t r = 0; r < m_nHeight; r++)
    {
        if (r+y >= height) break;

        for (size_t c = 0; c < m_nWidth; c++)
        {
            if (c+x >= width) break;

            rgb_t rgb;
            rgb.r = m_pRowPointers[r][c*3+0];
            rgb.g = m_pRowPointers[r][c*3+1];
            rgb.b = m_pRowPointers[r][c*3+2];
            rgb.a = 255;

            pFb[(r+y)*width + (c+x)] = rgb;
        }
    }
}

uint32_t Png::compileColour(uint8_t r, uint8_t g, uint8_t b, Display::PixelFormat pf)
{
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
  return 0 |
         (static_cast<uint32_t>(r) << pf.pRed) |
         (static_cast<uint32_t>(g) << pf.pGreen) |
         (static_cast<uint32_t>(b) << pf.pBlue);
}
