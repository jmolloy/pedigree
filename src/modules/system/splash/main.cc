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

#include <Module.h>
#include <Log.h>

#include <utilities/assert.h>
#include <machine/Display.h>
#include <processor/Processor.h>

#include "image.h"

#ifdef X86_COMMON
extern Display *g_pDisplay;
extern Display::ScreenMode g_ScreenMode;
#endif

static Display::rgb_t *g_pBuffer;
static size_t g_Width = 0;
static size_t g_Height = 0;

static Display::rgb_t g_Bg = {0x00,0x00,0x00};

static Display::rgb_t g_ProgressBorderCol = {150,80,0};
static Display::rgb_t g_ProgressCol = {150,100,0};

static size_t g_ProgressX, g_ProgressY;
static size_t g_ProgressW, g_ProgressH;

static size_t g_Total = 0;

void total(uintptr_t total)
{
    g_Total = total;
}

void progress(const char *text, uintptr_t progress)
{
    // Calculate percentage.
    if (g_Total == 0)
        return;

    size_t w = (g_ProgressW * progress) / g_Total;
    g_pDisplay->fillRectangle(g_pBuffer, g_ProgressX, g_ProgressY, w, g_ProgressH, g_ProgressCol);
    g_pDisplay->fillRectangle(g_pBuffer, g_ProgressX+w, g_ProgressY, g_ProgressW-w, g_ProgressH, g_Bg);
}

void init()
{
    if (!g_pDisplay)
    {
        FATAL("No display");
    }
    g_pBuffer = g_pDisplay->newBuffer();
    assert(g_pBuffer);

    g_Width   = g_ScreenMode.width;
    g_Height  = g_ScreenMode.height;

    g_pDisplay->setCurrentBuffer(g_pBuffer);
    g_pDisplay->fillRectangle(g_pBuffer, 0, 0, g_Width, g_Height, g_Bg);

    size_t x, y;
    size_t origx = x = (g_Width - width) / 2;
    size_t origy = y = (g_Height - height) / 4;

    char *data = header_data;
    for (size_t i = 0; i < width*height; i++)
    {
        if (i != 0 && (i % width) == 0)
        {
            x = origx;
            y++;
        }
        uint8_t pixel[3];
        HEADER_PIXEL(data, pixel);
        g_pBuffer[y*g_Width + x].r = pixel[0];
        g_pBuffer[y*g_Width + x].g = pixel[1];
        g_pBuffer[y*g_Width + x].b = pixel[2];
        x++;
    }
    g_pDisplay->updateBuffer(g_pBuffer, origx, origy, origx+width, origy+height);

    g_ProgressX = (g_Width / 2) - 200;
    g_ProgressW = 400;
    g_ProgressY = (g_Height / 4) * 3;
    g_ProgressH = 15;

    // Draw empty progress bar. Easiest way to draw a nonfilled rect?
    // draw two filled rects.
    g_pDisplay->fillRectangle(g_pBuffer, g_ProgressX-2, g_ProgressY-2, g_ProgressW+4, g_ProgressH+4, g_ProgressBorderCol);
    g_pDisplay->fillRectangle(g_pBuffer, g_ProgressX-1, g_ProgressY-1, g_ProgressW+2, g_ProgressH+2, g_Bg);

    g_BootProgress = &progress;
    g_BootProgressTotal = &total;
}

void destroy()
{
}

MODULE_NAME("splash");
MODULE_ENTRY(&init);
MODULE_EXIT(&destroy);
#ifdef X86_COMMON
MODULE_DEPENDS("vbe");
#endif
