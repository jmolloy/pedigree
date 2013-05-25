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

    m_pBitmap = (uint32_t *) malloc(4 * w * h);
    size_t x, y;
    for(y = 0; y < m_nHeight; ++y)
    {
      png_byte* row = m_pRowPointers[y];
      for(x = 0; x < m_nWidth; ++x)
      {
        png_byte * ptr = &(row[x*3]);
        m_pBitmap[(y * m_nWidth) + x] = (ptr[0] << 16) | (ptr[1] << 8) | (ptr[2]);
      }
    }

    syslog(LOG_ALERT, "PNG loaded %ul %ul", m_nWidth, m_nHeight);
}

Png::~Png()
{
}

void Png::render(cairo_t *cr, size_t x, size_t y, size_t width, size_t height)
{
  cairo_surface_t *surface = cairo_image_surface_create_for_data(
                                  (uint8_t*) m_pBitmap,
                                  CAIRO_FORMAT_RGB24,
                                  m_nWidth,
                                  m_nHeight,
                                  m_nWidth * 4);

  cairo_save(cr);
  cairo_identity_matrix(cr);
  cairo_translate(cr, x, y);
  cairo_scale(cr, width / (double) m_nWidth, height / (double) m_nHeight);
  cairo_set_source_surface(cr, surface, 0, 0);
  cairo_paint(cr);
  cairo_restore(cr);

  cairo_surface_destroy(surface);
}

void Png::renderPartial(
            cairo_t *cr,
            size_t atX, size_t atY,
            size_t innerX, size_t innerY,
            size_t partialWidth, size_t partialHeight,
            size_t scaleWidth, size_t scaleHeight)
{
  cairo_surface_t *surface = cairo_image_surface_create_for_data(
                                  (uint8_t*) m_pBitmap,
                                  CAIRO_FORMAT_RGB24,
                                  m_nWidth,
                                  m_nHeight,
                                  m_nWidth * 4);

  cairo_save(cr);

  cairo_rectangle(cr, atX, atY, partialWidth, partialHeight);
  cairo_clip(cr);
  cairo_new_path(cr);

  cairo_identity_matrix(cr);
  cairo_scale(cr, scaleWidth / (double) m_nWidth, scaleHeight / (double) m_nHeight);
  cairo_translate(cr, innerX, innerY);
  cairo_set_source_surface(cr, surface, 0, 0);
  cairo_paint(cr);
  cairo_restore(cr);

  cairo_surface_destroy(surface);
}
