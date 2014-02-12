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

#include <core/BootIO.h> // In src/system/kernel

#include "image.h"
#include "font.h"

Framebuffer *g_pFramebuffer = 0;

extern BootIO bootIO;

static uint8_t *g_pBuffer = 0;
static Graphics::Buffer *g_pFont = 0;
static size_t g_Width = 0;
static size_t g_Height = 0;

static uint32_t g_BackgroundColour      = 0x000000;
static uint32_t g_ForegroundColour      = 0xFFFFFF;
static uint32_t g_ProgressBorderColour  = 0x965000;
static uint32_t g_ProgressColour        = 0x966400;

static Graphics::PixelFormat g_ColorFormat = Graphics::Bits24_Rgb;

static size_t g_ProgressX, g_ProgressY;
static size_t g_ProgressW, g_ProgressH;
static size_t g_LogBoxX, g_LogBoxY;
static size_t g_LogX, g_LogY;
static size_t g_LogW, g_LogH;

static GraphicsService::GraphicsProvider pProvider;

static size_t g_Previous = 0;
static bool g_LogMode = false;

#ifdef NOGFX
static bool g_NoGraphics = true;
#else
static bool g_NoGraphics = false;
#endif

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

    if(c == '\t')
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
        g_pFramebuffer->rect(g_LogBoxX, g_LogBoxY + ((g_LogH / FONT_HEIGHT) - diff) * FONT_HEIGHT, g_LogW - g_LogBoxX, diff * FONT_HEIGHT, g_BackgroundColour, g_ColorFormat);

        g_LogY = (g_LogH / FONT_HEIGHT) - diff;

        g_pFramebuffer->redraw(g_LogBoxX, g_LogBoxY, g_LogW, g_LogH, true);
    }
}

void printString(const char *str)
{
    if(g_NoGraphics)
    {
        for(size_t i = 0; i < strlen(str); i++)
            printChar(str[i]);
    }
    else
    {
        static HugeStaticString s;
        s += str;
        
        BootIO::Colour c = BootIO::LightGrey;
        if(str[1] == 'W')
            c = BootIO::Orange;
        else if(str[1] == 'E' || str[1] == 'F')
            c = BootIO::Red;
        else if(str[1] == 'D')
            c = BootIO::DarkGrey;
            
        bootIO.write(s, c, BootIO::Black);
        s.clear();
    }
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

void centerStringAt(const char *str, size_t midX, size_t midY)
{
    /// \todo Handle overflows
    size_t width = strlen(str) * FONT_WIDTH;
    size_t height = FONT_HEIGHT;

    size_t x = midX - (width / 2);
    size_t y = midY - (height / 2);
    printStringAt(str, x, y);
}

class StreamingScreenLogger : public Log::LogCallback
{
    public:
        /// printString is used directly as well as in this callback object,
        /// therefore we simply redirect to it.
        void callback(const char *str)
        {
#ifdef DEBUGGER
            printString(str);
#endif
        }
};

static StreamingScreenLogger g_StreamLogger;

void keyCallback(InputManager::InputNotification &note)
{
    if(note.type != InputManager::Key)
        return;

    uint64_t key = note.data.key.key;
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

    bool bFinished = false;
    if((g_BootProgressCurrent + 1) >= g_BootProgressTotal)
    {
        Log::instance().removeCallback(&g_StreamLogger);

#ifdef DEBUGGER
        if(g_NoGraphics)
        {
            InputManager::instance().removeCallback(keyCallback);
        }
#endif

        bFinished = true;
    }

    if(g_LogMode)
        return;

    if(!g_NoGraphics)
    {
        size_t w = (g_ProgressW * g_BootProgressCurrent) / g_BootProgressTotal;
        if(g_Previous <= g_BootProgressCurrent)
            g_pFramebuffer->rect(g_ProgressX, g_ProgressY, w, g_ProgressH, g_ProgressColour, g_ColorFormat);
        else
            g_pFramebuffer->rect(g_ProgressX + w, g_ProgressY, g_ProgressW-w, g_ProgressH, g_BackgroundColour, g_ColorFormat);
        g_Previous = g_BootProgressCurrent;

        char buf[80];
        sprintf(buf, "%d%%", ((g_BootProgressCurrent * 100) / g_BootProgressTotal));
        centerStringAt(buf, g_ProgressX + (g_ProgressW / 2), g_ProgressY - FONT_HEIGHT);

        g_pFramebuffer->redraw(g_ProgressX, g_ProgressY - (FONT_HEIGHT * 2), g_ProgressW, g_ProgressH + (FONT_HEIGHT * 2), true);

        if(bFinished)
        {
            // Clean up font
            g_pFramebuffer->destroyBuffer(g_pFont);
            delete [] g_pBuffer;

            // Destroy the framebuffer now that we're done
            Graphics::destroyFramebuffer(g_pFramebuffer);
            g_pFramebuffer = 0;

            // No longer wanting any progress callback.
            g_BootProgressUpdate = 0;
        }
    }
}

static void getColor(const char *colorName, uint32_t &color)
{
    // The query string
    String sQuery;

    // Create the query string
    sQuery += "select r,g,b from 'colour_scheme' where name='";
    sQuery += colorName;
    sQuery += "';";

    // Query the database
    Config::Result *pResult = Config::instance().query(sQuery);

    // Did the query fail?
    if(!pResult)
    {
        ERROR("Splash: Error looking up '" << colorName << "' colour.");
        return;
    }

    if(!pResult->succeeded())
    {
        ERROR("Splash: Error looking up '" << colorName << "' colour: " << pResult->errorMessage());
        delete pResult;
        return;
    }

    // Get the color from the query result
    color = Graphics::createRgb(pResult->getNum(0, "r"), pResult->getNum(0, "g"), pResult->getNum(0, "b"));

    // Dispose of the query result
    delete pResult;
}

static void getDesiredMode(size_t &width, size_t &height, size_t &bpp)
{
    // Query the database
    Config::Result *pResult = Config::instance().query("select width,height,bpp from 'desired_display_mode';");

    // Did the query fail?
    if(!pResult)
        return;

    if(!pResult->succeeded())
    {
        delete pResult;
        return;
    }

    // Get the mode details from the query result
    width = pResult->getNum(0, "width");
    height = pResult->getNum(0, "height");
    bpp = pResult->getNum(0, "bpp");

    // Dispose of the query result
    delete pResult;
}

static void init()
{
#ifdef NOGFX
    Log::instance().installCallback(&g_StreamLogger, true);

    g_BootProgressUpdate = &progress;
#else
    getColor("splash-background", g_BackgroundColour);
    getColor("splash-foreground", g_ForegroundColour);
    getColor("border", g_ProgressBorderColour);
    getColor("fill", g_ProgressColour);

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
        Log::instance().installCallback(&g_StreamLogger, true);
        g_BootProgressUpdate = &progress;
        g_NoGraphics = true;
        return;
    }

    Display *pDisplay = pProvider.pDisplay;

    // Get the desired mode from the database
    size_t nDesiredWidth = 0, nDesiredHeight = 0, nDesiredBpp = 0;
    getDesiredMode(nDesiredWidth, nDesiredHeight, nDesiredBpp);

    // Set up the mode we want
    if(!(nDesiredWidth && nDesiredHeight && nDesiredBpp) || !pDisplay->setScreenMode(nDesiredWidth, nDesiredHeight, nDesiredBpp))
    {
        bool bModeFound = true;

        // 24-bit mode fallbacks
        NOTICE("splash: Falling back to 1024x768x24");
        if(!pDisplay->setScreenMode(1024, 768, 24))
        {
            // Attempt to fall back to 800x600
            NOTICE("splash: Falling back to 800x600x24");
            if(!pDisplay->setScreenMode(800, 600, 24))
            {
                // Finally try and fall back to 640x480
                NOTICE("splash: Falling back to 640x480x24");
                if(!pDisplay->setScreenMode(640, 480, 24))
                {
                    bModeFound = false;
                }
            }
        }

        if(!bModeFound)
        {
            // 16-bit mode fallbacks
            NOTICE("splash: Falling back to 1024x768x16");
            if(!pDisplay->setScreenMode(1024, 768, 16))
            {
                // Attempt to fall back to 800x600
                NOTICE("splash: Falling back to 800x600x16");
                if(!pDisplay->setScreenMode(800, 600, 16))
                {
                    // Finally try and fall back to 640x480
                    NOTICE("splash: Falling back to 640x480x16");
                    if(!pDisplay->setScreenMode(640, 480, 16))
                    {
                        ERROR("splash: Couldn't find a suitable display mode for this system (tried: 1024x768, 800x600, 640x480).");
                        g_NoGraphics = true;
                    }
                }
            }
        }
    }

    if(g_NoGraphics)
    {
        Log::instance().installCallback(&g_StreamLogger, true);
        g_BootProgressUpdate = &progress;
        return;
    }

    Framebuffer *pParentFramebuffer = pProvider.pFramebuffer;

    g_Width   = pParentFramebuffer->getWidth();
    g_Height  = pParentFramebuffer->getHeight();

    g_pFramebuffer = Graphics::createFramebuffer(pParentFramebuffer, 0, 0, g_Width, g_Height);
    g_ColorFormat = g_pFramebuffer->getFormat();

    g_pFramebuffer->rect(0, 0, g_Width, g_Height, g_BackgroundColour, g_ColorFormat);

    // Create the logo buffer
    uint8_t *data = header_data;
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
#ifdef DEBUGGER
#endif

#ifdef DEBUGGER
    // Draw a border around the log area
    centerStringAt("< Kernel Log >", g_LogW / 2, g_LogBoxY - 2 - (FONT_HEIGHT / 2) - FONT_HEIGHT);
    centerStringAt("(you can push ESCAPE to make the log fill the screen)", g_LogW / 2, g_LogBoxY - 2 - (FONT_HEIGHT / 2));
    g_pFramebuffer->rect(g_LogBoxX, g_LogBoxY - 2, g_LogW, g_LogH - 2, g_ForegroundColour, g_ColorFormat);
    g_pFramebuffer->rect(g_LogBoxX, g_LogBoxY - 1, g_LogW, g_LogH - 1, g_BackgroundColour, g_ColorFormat);
#endif

    // Draw empty progress bar. Easiest way to draw a nonfilled rect? Draw two filled rects.
    g_pFramebuffer->rect(g_ProgressX - 2, g_ProgressY - 2, g_ProgressW + 4, g_ProgressH + 4, g_ProgressBorderColour, g_ColorFormat);
    g_pFramebuffer->rect(g_ProgressX - 1, g_ProgressY - 1, g_ProgressW + 2, g_ProgressH + 2, g_BackgroundColour, g_ColorFormat);

    g_pFramebuffer->redraw(0, 0, g_Width, g_Height, true);

    Log::instance().installCallback(&g_StreamLogger, true);

    g_BootProgressUpdate = &progress;

#ifdef DEBUGGER
    InputManager::instance().installCallback(InputManager::Key, keyCallback);
#endif

#endif    
}

static void destroy()
{
}

MODULE_INFO("splash", &init, &destroy, "gfx-deps", "config");

