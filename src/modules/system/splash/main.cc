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
#include <machine/InputManager.h>
#include <processor/Processor.h>
#include <config/Config.h>

#include <LockGuard.h>

#include "image.h"
#include "font.h"

#ifdef X86_COMMON
extern Display *g_pDisplay;
extern Display::ScreenMode g_ScreenMode;
#endif

static Display::rgb_t *g_pBuffer;
static size_t g_Width = 0;
static size_t g_Height = 0;

static Display::rgb_t g_Bg = {0x00,0x00,0x00,0x00};
static Display::rgb_t g_Fg = {0xFF,0xFF,0xFF,0xFF};

static Display::rgb_t g_ProgressBorderCol = {150,80,0,0x00};
static Display::rgb_t g_ProgressCol = {150,100,0,0x00};

static size_t g_ProgressX, g_ProgressY;
static size_t g_ProgressW, g_ProgressH;
static size_t g_LogBoxX, g_LogBoxY;
static size_t g_LogX, g_LogY;
static size_t g_LogW, g_LogH;

static size_t g_Previous = 0;
static bool g_LogMode = false;

Mutex g_PrintLock(false);

void printChar(char c)
{
    LockGuard<Mutex> guard(g_PrintLock);

    if(!c)
        return;
    else if(c == '\t')
        g_LogX = (g_LogX + 8) & ~7;
    else if(c == '\r')
        g_LogX = 0;
    else if(c == '\n')
    {
        g_LogX = 0;
        g_LogY++;
    }
    else if(c >= ' ')
    {
        for(size_t i = 0;i<FONT_HEIGHT;i++)
        {
            for(size_t j = 0;j<FONT_WIDTH;j++)
            {
                if(font_data[(c*FONT_HEIGHT)+i] & (1<<(FONT_WIDTH-j)))
                {
                    g_pBuffer[((g_LogBoxY+g_LogY*FONT_HEIGHT + i)*g_Width) + ((g_LogBoxX+g_LogX*FONT_WIDTH) + j)].r = g_Fg.r;
                    g_pBuffer[((g_LogBoxY+g_LogY*FONT_HEIGHT + i)*g_Width) + ((g_LogBoxX+g_LogX*FONT_WIDTH) + j)].g = g_Fg.g;
                    g_pBuffer[((g_LogBoxY+g_LogY*FONT_HEIGHT + i)*g_Width) + ((g_LogBoxX+g_LogX*FONT_WIDTH) + j)].b = g_Fg.b;
                }
            }
        }
        g_pDisplay->updateBuffer(g_pBuffer, g_LogBoxX+g_LogX*FONT_WIDTH, g_LogBoxY+g_LogY*FONT_HEIGHT, g_LogBoxX+g_LogX*FONT_WIDTH+FONT_WIDTH, g_LogBoxY+g_LogY*FONT_HEIGHT+FONT_HEIGHT);
        g_LogX++;
    }

    if(g_LogX >= g_LogW/FONT_WIDTH)
    {
        g_LogX = 0;
        g_LogY++;
    }

    // Overflowed the view?
    if(g_LogY >= (g_LogH/FONT_HEIGHT))
    {
        // By how much?
        size_t diff = g_LogY - g_LogH/FONT_HEIGHT + 1;

        g_pDisplay->bitBlit(g_pBuffer, g_LogBoxX, g_LogBoxY + (diff*FONT_HEIGHT), g_LogBoxX, g_LogBoxY, g_LogW, ((g_LogH/FONT_HEIGHT)-diff)*FONT_HEIGHT);
        g_pDisplay->fillRectangle(g_pBuffer, g_LogBoxX, g_LogBoxY + ((g_LogH/FONT_HEIGHT)-diff)*FONT_HEIGHT, g_LogW - g_LogBoxX, diff*FONT_HEIGHT, g_Bg);
        g_LogY = g_LogH/FONT_HEIGHT-diff;
    }
}

void printString(const char *str)
{
    for(size_t i = 0;i<strlen(str);i++)
        printChar(str[i]);
}

class StreamingScreenLogger : public Log::LogCallback
{
    public:
        /// printString is used directly as well as in this callback object,
        /// therefore we simply redirect to it.
        void callback(const char *str)
        {
            printString(str);
        }
};

static StreamingScreenLogger g_StreamLogger;

void keyCallback(uint64_t key)
{
    if(key == '\e' && !g_LogMode)
    {
        // Because we edit the dimensions of the screen, we can't let a print
        // continue while we run here.
        LockGuard<Mutex> guard(g_PrintLock);

        g_LogMode = true;

        g_LogY += (g_LogBoxY / FONT_HEIGHT);
        g_LogX = (g_LogBoxX / FONT_WIDTH);
        g_LogBoxX = g_LogBoxY = 0;
        g_LogW = g_Width;
        g_LogH = g_Height;
    }
}

void progress(const char *text)
{
    // Calculate percentage.
    if(g_BootProgressTotal == 0)
        return;

    if((g_BootProgressCurrent + 1) >= g_BootProgressTotal)
    {
        Log::instance().removeCallback(&g_StreamLogger);
        InputManager::instance().removeCallback(InputManager::Key, keyCallback);
    }

    // Check if we should refresh screen
    if(!strcmp(text, "unload"))
        g_pDisplay->setCurrentBuffer(g_pBuffer);

    if(g_LogMode)
        return;

    size_t w = (g_ProgressW * g_BootProgressCurrent) / g_BootProgressTotal;
    if(g_Previous <= g_BootProgressCurrent)
        g_pDisplay->fillRectangle(g_pBuffer, g_ProgressX, g_ProgressY, w, g_ProgressH, g_ProgressCol);
    else
        g_pDisplay->fillRectangle(g_pBuffer, g_ProgressX+w, g_ProgressY, g_ProgressW-w, g_ProgressH, g_Bg);
    g_Previous = g_BootProgressCurrent;
}

static void init()
{
    if (!g_pDisplay)
    {
        FATAL("No display");
    }
    g_pBuffer = g_pDisplay->newBuffer();
    assert(g_pBuffer);

    g_Width   = g_ScreenMode.width;
    g_Height  = g_ScreenMode.height;

    Config::Result *pResult = Config::instance().query("select r,g,b from 'colour-scheme' where name='splash-background';");
    if (!pResult)
    {
        ERROR("Error looking up background colour.");
    }
    else
    {
        g_Bg.r = pResult->getNum(0, static_cast<size_t>(0));
        g_Bg.g = pResult->getNum(0, 1);
        g_Bg.b = pResult->getNum(0, 2);
        delete pResult;
    }

    pResult = Config::instance().query("select r,g,b from 'colour-scheme' where name='splash-foreground';");
    if (!pResult)
    {
        ERROR("Error looking up foreground colour.");
    }
    else
    {
        g_Fg.r = pResult->getNum(0, static_cast<size_t>(0));
        g_Fg.g = pResult->getNum(0, 1);
        g_Fg.b = pResult->getNum(0, 2);
        delete pResult;
    }

    pResult = Config::instance().query("select r,g,b from 'colour-scheme' where name='border';");
    if (!pResult)
    {
        ERROR("Error looking up border colour.");
    }
    else
    {
        g_ProgressBorderCol.r = pResult->getNum(0, static_cast<size_t>(0));
        g_ProgressBorderCol.g = pResult->getNum(0, 1);
        g_ProgressBorderCol.b = pResult->getNum(0, 2);
        delete pResult;
    }

    pResult = Config::instance().query("select r,g,b from 'colour-scheme' where name='fill';");
    if (!pResult)
    {
        ERROR("Error looking up border colour.");
    }
    else
    {
        g_ProgressCol.r = pResult->getNum(0, static_cast<size_t>(0));
        g_ProgressCol.g = pResult->getNum(0, 1);
        g_ProgressCol.b = pResult->getNum(0, 2);
        delete pResult;
    }

    g_pDisplay->setCurrentBuffer(g_pBuffer);
    g_pDisplay->fillRectangle(g_pBuffer, 0, 0, g_Width, g_Height, g_Bg);

    size_t x, y;
    size_t origx = x = (g_Width - width) / 2;
    size_t origy = y = (g_Height - height) / 3;

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

    g_LogBoxX = 0;
    g_LogBoxY = g_ProgressY + (g_ProgressH * 2);
    g_LogW = g_Width;
    g_LogH = g_Height - g_LogBoxY;
    g_LogX = g_LogY = 0;

    // Draw a border around the log area
    g_pDisplay->fillRectangle(g_pBuffer, g_LogBoxX, g_LogBoxY - 2, g_LogW, g_LogH - 2, g_Fg);
    g_pDisplay->fillRectangle(g_pBuffer, g_LogBoxX, g_LogBoxY - 1, g_LogW, g_LogH - 1, g_Bg);

    // Draw empty progress bar. Easiest way to draw a nonfilled rect?
    // draw two filled rects.
    g_pDisplay->fillRectangle(g_pBuffer, g_ProgressX-2, g_ProgressY-2, g_ProgressW+4, g_ProgressH+4, g_ProgressBorderCol);
    g_pDisplay->fillRectangle(g_pBuffer, g_ProgressX-1, g_ProgressY-1, g_ProgressW+2, g_ProgressH+2, g_Bg);

    Log::instance().installCallback(&g_StreamLogger, true);

    g_BootProgressUpdate = &progress;
    InputManager::instance().installCallback(InputManager::Key, keyCallback);
}

static void destroy()
{
}

MODULE_INFO("splash", &init, &destroy, "vbe", "config");

