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
#include <graphics/Graphics.h>
#include <graphics/GraphicsService.h>

#include "image.h"
#include "font.h"

Framebuffer *g_pFramebuffer = 0;

static uint8_t *g_pBuffer = 0;
static Graphics::Buffer *g_pFont = 0;
static size_t g_Width = 0;
static size_t g_Height = 0;

static Display::rgb_t g_Bg = {0x00,0x00,0x00,0x00};
static Display::rgb_t g_Fg = {0xFF,0xFF,0xFF,0xFF};

static uint32_t g_BackgroundColour = 0;
static uint32_t g_ForegroundColour = 0xFFFFFF;

static uint32_t g_ProgressBorderColour = 0x965000;
static uint32_t g_ProgressColour       = 0x966400;

static Graphics::PixelFormat g_BgFgFormat = Graphics::Bits24_Rgb;

static Display::rgb_t g_ProgressBorderCol = {150,80,0,0x00};
static Display::rgb_t g_ProgressCol = {150,100,0,0x00};

static size_t g_ProgressX, g_ProgressY;
static size_t g_ProgressW, g_ProgressH;
static size_t g_LogBoxX, g_LogBoxY;
static size_t g_LogX, g_LogY;
static size_t g_LogW, g_LogH;
    
static GraphicsService::GraphicsProvider pProvider;

static size_t g_Previous = 0;
static bool g_LogMode = false;

Mutex g_PrintLock(false);

void printChar(char c, size_t x, size_t y)
{
    if(!g_pFramebuffer)
        return;

    g_pFramebuffer->blit(g_pFont, 0, c * FONT_HEIGHT, x, y, FONT_WIDTH, FONT_HEIGHT);
}

void printChar(char c)
{
    if(!g_pFramebuffer)
        return;

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
    
        g_pFramebuffer->redraw(g_LogBoxX, g_LogBoxY, g_LogW, g_LogH, true);
    }
    else if(c >= ' ')
    {
        g_pFramebuffer->blit(g_pFont, 0, c * FONT_HEIGHT, g_LogBoxX + (g_LogX * FONT_WIDTH), g_LogBoxY + (g_LogY * FONT_HEIGHT), FONT_WIDTH, FONT_HEIGHT);
        g_LogX++;
    }

    if(g_LogX >= g_LogW/FONT_WIDTH)
    {
        g_LogX = 0;
        g_LogY++;
    
        g_pFramebuffer->redraw(g_LogBoxX, g_LogBoxY, g_LogW, g_LogH, true);
    }

    // Overflowed the view?
    if(g_LogY >= (g_LogH / FONT_HEIGHT))
    {
        // By how much?
        size_t diff = g_LogY - (g_LogH / FONT_HEIGHT) + 1;
        
        // Scroll up
        g_pFramebuffer->copy(g_LogBoxX, g_LogBoxY + (diff * FONT_HEIGHT), g_LogBoxX, g_LogBoxY, g_LogW - g_LogBoxX, ((g_LogH / FONT_HEIGHT) - diff) * FONT_HEIGHT);
        g_pFramebuffer->rect(g_LogBoxX, g_LogBoxY + ((g_LogH / FONT_HEIGHT) - diff) * FONT_HEIGHT, g_LogW - g_LogBoxX, diff * FONT_HEIGHT, g_BackgroundColour, g_BgFgFormat);
        
        g_LogY = (g_LogH / FONT_HEIGHT) - diff;
    
        g_pFramebuffer->redraw(g_LogBoxX, g_LogBoxY, g_LogW, g_LogH, true);
    }
}

void printString(const char *str)
{
    for(size_t i = 0;i<strlen(str);i++)
        printChar(str[i]);
}

void printStringAt(const char *str, size_t x, size_t y)
{
    /// \todo Handle overflows
    for(size_t i = 0; i < strlen(str); i++)
    {
        printChar(str[i], x, y);
        x += FONT_WIDTH;
    }
}

void centerStringAt(const char *str, size_t x, size_t y)
{
    /// \todo Handle overflows
    size_t pxWidth = strlen(str) * FONT_WIDTH;
    
    size_t startX = x - (pxWidth / 2);
    for(size_t i = 0; i < strlen(str); i++)
    {
        printChar(str[i], startX, y);
        startX += FONT_WIDTH;
    }
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
    if(!g_pFramebuffer)
        return;

    bool bFinished = false;
    if((g_BootProgressCurrent + 1) >= g_BootProgressTotal)
    {
        Log::instance().removeCallback(&g_StreamLogger);
        InputManager::instance().removeCallback(InputManager::Key, keyCallback);

        bFinished = true;
    }

    if(g_LogMode)
        return;

    size_t w = (g_ProgressW * g_BootProgressCurrent) / g_BootProgressTotal;
    if(g_Previous <= g_BootProgressCurrent)
        g_pFramebuffer->rect(g_ProgressX, g_ProgressY, w, g_ProgressH, g_ProgressColour, g_BgFgFormat);
    else
        g_pFramebuffer->rect(g_ProgressX + w, g_ProgressY, g_ProgressW-w, g_ProgressH, g_BackgroundColour, g_BgFgFormat);
    g_Previous = g_BootProgressCurrent;
    
    g_pFramebuffer->redraw(g_ProgressX, g_ProgressY, g_ProgressW, g_ProgressH, true);

    if(bFinished)
    {
        // Destroy the framebuffer now that we're done
        Graphics::destroyFramebuffer(g_pFramebuffer);
        g_pFramebuffer = 0;
    }
}

static void init()
{
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
        g_BackgroundColour = Graphics::createRgb(g_Bg.r, g_Bg.g, g_Bg.b);
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
        g_ForegroundColour = Graphics::createRgb(g_Fg.r, g_Fg.g, g_Fg.b);
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
        g_ProgressBorderColour = Graphics::createRgb(g_ProgressBorderCol.r, g_ProgressBorderCol.g, g_ProgressBorderCol.b);
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
        g_ProgressColour = Graphics::createRgb(g_ProgressCol.r, g_ProgressCol.g, g_ProgressCol.b);
        delete pResult;
    }

    // Grab the current graphics provider for the system, use it to display the
    // splash screen to the user.
    /// \todo Check for failure
    ServiceFeatures *pFeatures = ServiceManager::instance().enumerateOperations(String("graphics"));
    Service         *pService  = ServiceManager::instance().getService(String("graphics"));
    bool bSuccess = false;
    if(pFeatures->provides(ServiceFeatures::probe))
        if(pService)
            bSuccess = pService->serve(ServiceFeatures::probe, reinterpret_cast<void*>(&pProvider), sizeof(pProvider));
    
    if(!bSuccess)
    {
        FATAL("splash: couldn't find any graphics providers - at least one is required to run Pedigree!");
    }
    
    Display *pDisplay = pProvider.pDisplay;
    
    // Set up a mode we want
    if(!pDisplay->setScreenMode(0x117)) // 0x118 for 24-bit
    {
        NOTICE("splash: Falling back to 800x600");

        // Attempt to fall back to 800x600
        if(!pDisplay->setScreenMode(0x114)) // 0x115 for 24-bit
        {
            NOTICE("splash: Falling back to 640x480");

            // Finally try and fall back to 640x480
            if(!pDisplay->setScreenMode(0x111)) // 0x112 for 24-bit
            {
                ERROR("splash: Couldn't find a suitable display mode for this system (tried: 1024x768, 800x600, 640x480.");
                return;
            }
        }
    }

    Framebuffer *pParentFramebuffer = pProvider.pFramebuffer;
    
    g_Width   = pParentFramebuffer->getWidth();
    g_Height  = pParentFramebuffer->getHeight();

    g_pFramebuffer = Graphics::createFramebuffer(pParentFramebuffer, 0, 0, g_Width, g_Height, pParentFramebuffer->getFormat());
    
    g_pFramebuffer->rect(0, 0, g_Width, g_Height, g_BackgroundColour, g_BgFgFormat);
    
    // Create the logo buffer
    char *data = header_data;
    g_pBuffer = new uint8_t[width * height * 3]; // 24-bit, hardcoded...
    for (size_t i = 0; i < (width*height); i++)
        HEADER_PIXEL(data, &g_pBuffer[i * 3]); // 24-bit, hardcoded
    
    size_t origx = (g_Width - width) / 2;
    size_t origy = (g_Height - height) / 3;

    g_pFramebuffer->draw(g_pBuffer, 0, 0, origx, origy, width, height, Graphics::Bits24_Bgr);
    
    delete [] g_pBuffer;
        
    // Create the font buffer
    g_pBuffer = new uint8_t[(FONT_WIDTH * FONT_HEIGHT * 3) * 256]; // 24-bit
    memset(g_pBuffer, 0, (FONT_WIDTH * FONT_HEIGHT * 3) * 256);
    size_t offset = 0;
    
    // For each character
    for(size_t character = 0; character < 255; character++)
    {
        // For each character row
        for(size_t row = 0; row < FONT_HEIGHT; row++)
        {
            // For each character row bit
            for(size_t col = 0; col <= FONT_WIDTH; col++)
            {
                // Is this bit set?
                size_t fontRow = (character * FONT_HEIGHT) + row;
                if(font_data[fontRow] & (1 << (FONT_WIDTH - col)))
                {
                    // x: col
                    // y: fontRow
                    size_t bytesPerPixel = 3;
                    size_t bytesPerLine = FONT_WIDTH * bytesPerPixel;
                    size_t pixelOffset = (fontRow * bytesPerLine) + (col * bytesPerPixel);
                    size_t bufferOffset = pixelOffset;
                    
                    uint32_t *p = reinterpret_cast<uint32_t*>(adjust_pointer(g_pBuffer, bufferOffset));
                    *p = g_ForegroundColour;
                }
            }
        }
    }

    g_pFont = g_pFramebuffer->createBuffer(g_pBuffer, Graphics::Bits24_Rgb, FONT_WIDTH, FONT_HEIGHT * 256);
    
    g_ProgressX = (g_Width / 2) - 200;
    g_ProgressW = 400;
    g_ProgressY = (g_Height / 3) * 2;
    g_ProgressH = 15;

    g_LogBoxX = 0;
    g_LogBoxY = (g_Height / 4) * 3;
    g_LogW = g_Width;
    g_LogH = g_Height - g_LogBoxY;
    g_LogX = g_LogY = 0;

    // Yay text!
    centerStringAt("Please wait, Pedigree is loading...", g_Width / 2, g_ProgressY - (FONT_HEIGHT * 3));

    // Draw a border around the log area
    g_pFramebuffer->rect(g_LogBoxX, g_LogBoxY - 2, g_LogW, g_LogH - 2, g_ForegroundColour, g_BgFgFormat);
    g_pFramebuffer->rect(g_LogBoxX, g_LogBoxY - 1, g_LogW, g_LogH - 1, g_BackgroundColour, g_BgFgFormat);

    // Draw empty progress bar. Easiest way to draw a nonfilled rect? Draw two filled rects.
    g_pFramebuffer->rect(g_ProgressX - 2, g_ProgressY - 2, g_ProgressW + 4, g_ProgressH + 4, g_ProgressBorderColour, g_BgFgFormat);
    g_pFramebuffer->rect(g_ProgressX - 1, g_ProgressY - 1, g_ProgressW + 2, g_ProgressH + 2, g_BackgroundColour, g_BgFgFormat);
    
    g_pFramebuffer->redraw(0, 0, g_Width, g_Height, true);

    Log::instance().installCallback(&g_StreamLogger, true);

    g_BootProgressUpdate = &progress;
    InputManager::instance().installCallback(InputManager::Key, keyCallback);
}

static void destroy()
{
}

MODULE_INFO("splash", &init, &destroy, "gfx-deps", "config");

