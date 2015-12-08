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

#include "Header.h"

#include <syslog.h>

#include <native/config/Config.h>
#include <native/graphics/Graphics.h>

#include <cairo/cairo.h>

extern PedigreeGraphics::Framebuffer *g_pFramebuffer;

extern cairo_t *g_Cairo;

uint32_t g_BorderColour                    = 0x965000; // {150, 80, 0,0};
uint32_t g_TabBackgroundColour             = 0x000000; // {0, 0, 0,0};
uint32_t g_TabTextColour                   = 0x966400; // {150, 100, 0,0};
uint32_t g_SelectedTabBackgroundColour     = 0x966400; // {150, 100, 0,0};
uint32_t g_SelectedTabTextColour           = 0xFFFFFF; // {255, 255, 255,0};
uint32_t g_TextColour                      = 0xFFFFFF; // {255, 255, 255,0};
uint32_t g_MainBackgroundColour            = 0x000000; // {0, 0, 0,0};
size_t g_FontSize = 16;

static void getRgbColorFromDb(const char *colorName, uint32_t &color)
{
    // Use defaults on Linux.
#ifndef TARGET_LINUX
    // The query string
    std::string sQuery;

    // Create the query string
    sQuery += "select r,g,b from 'colour_scheme' where name='";
    sQuery += colorName;
    sQuery += "';";

    // Query the database
    Config::Result *pResult = Config::query(sQuery.c_str());

    // Did the query fail?
    if(!pResult)
    {
        syslog(LOG_ALERT, "TUI: Error looking up '%s' colour.", colorName);
        return;
    }
    if(!pResult->succeeded())
    {
        syslog(LOG_ALERT, "TUI: Error looking up '%s' colour: %s\n", colorName, pResult->errorMessage().c_str());
        delete pResult;
        return;
    }

    // Get the color from the query result
    color = PedigreeGraphics::createRgb(pResult->getNum(0, "r"), pResult->getNum(0, "g"), pResult->getNum(0, "b"));

    // Dispose of the query result
    delete pResult;
#endif
}

Header::Header(size_t nWidth) :
    m_nWidth(nWidth), m_Page(0), m_LastPage(0), m_pFont(0),
    m_pTabs(0), m_NextTabId(0), m_pFramebuffer(0)
{
#ifdef TARGET_LINUX
    m_pFont = new Font(g_FontSize, "/usr/share/fonts/truetype/dejavu/DejaVuSansMono-Bold.ttf", false, nWidth);
#else
    m_pFont = new Font(g_FontSize, "/system/fonts/DejaVuSansMono-Bold.ttf", false, nWidth);
#endif
    g_FontSize = m_pFont->getHeight();

    getRgbColorFromDb("border", g_BorderColour);
    g_TabBackgroundColour = g_BorderColour;
    getRgbColorFromDb("fill", g_SelectedTabBackgroundColour);
    getRgbColorFromDb("selected-text", g_TextColour);
    g_TabTextColour = g_TextColour;
    getRgbColorFromDb("tui-background", g_MainBackgroundColour);
}

Header::~Header()
{
    delete m_pFont;
}

size_t Header::getHeight()
{
    return g_FontSize+5;
}

void Header::render(rgb_t *pBuffer, DirtyRectangle &rect)
{
    size_t charWidth = m_pFont->getWidth();

    if(!g_Cairo)
    {
        return;
    }

    // Set up the dirty rectangle to cover the entire header area.
    rect.point(0, 0);
    rect.point(m_nWidth, g_FontSize+5);

    cairo_save(g_Cairo);
    cairo_set_operator(g_Cairo, CAIRO_OPERATOR_SOURCE);

    // Wipe the background.
    cairo_set_source_rgba(
            g_Cairo,
            ((g_MainBackgroundColour >> 16) & 0xFF) / 256.0,
            ((g_MainBackgroundColour >> 8) & 0xFF) / 256.0,
            ((g_MainBackgroundColour) & 0xFF) / 256.0,
            0.8);

    cairo_rectangle(g_Cairo, 0, 0, m_nWidth, g_FontSize + 4);
    cairo_fill(g_Cairo);

    size_t offset = 0;
    if (m_Page != 0)
    {
        // Render a left-double arrow.
        offset = renderString("<", offset, 2, g_TextColour, g_MainBackgroundColour);
    }

    Tab *pTab = m_pTabs;
    while (pTab)
    {
        if (pTab->page == m_Page)
        {
            offset += 5;

            uint32_t foreColour = g_TextColour;
            if (pTab->flags & TAB_SELECTED)
                foreColour = g_SelectedTabTextColour;
            else if (pTab->flags & TAB_SELECTABLE)
                foreColour = g_TabTextColour;

            uint32_t backColour = g_MainBackgroundColour;
            if (pTab->flags & TAB_SELECTED)
                backColour = g_SelectedTabBackgroundColour;
            else if (pTab->flags & TAB_SELECTABLE)
                backColour = g_TabBackgroundColour;

            // Fill to background colour.
            cairo_set_source_rgba(
                    g_Cairo,
                    ((backColour >> 16) & 0xFF) / 256.0,
                    ((backColour >> 8) & 0xFF) / 256.0,
                    ((backColour) & 0xFF) / 256.0,
                    1.0);

            cairo_rectangle(g_Cairo, offset - 5, 0, strlen(pTab->text) * charWidth + 10, g_FontSize + 4);
            cairo_fill(g_Cairo);

            offset = renderString(pTab->text, offset, 2, foreColour, backColour);

            offset += 5;
            // Add a seperator
            cairo_set_source_rgba(
                    g_Cairo,
                    ((g_BorderColour >> 16) & 0xFF) / 256.0,
                    ((g_BorderColour >> 8) & 0xFF) / 256.0,
                    ((g_BorderColour) & 0xFF) / 256.0,
                    1.0);
            cairo_move_to(g_Cairo, offset, 0);
            cairo_line_to(g_Cairo, offset, g_FontSize + 4);
            cairo_stroke(g_Cairo);
        }
        pTab = pTab->next;
    }

    if (m_Page != m_LastPage)
    {
        offset += 5;
        // Render a right-double arrow.
        offset = renderString(">", offset, 2, g_TextColour, g_MainBackgroundColour);
    }

    // Height = font size + 2 px top and bottom + border 1px =
    // font-size + 5px.
    cairo_set_source_rgba(
            g_Cairo,
            ((g_BorderColour >> 16) & 0xFF) / 256.0,
            ((g_BorderColour >> 8) & 0xFF) / 256.0,
            ((g_BorderColour) & 0xFF) / 256.0,
            1.0);
    cairo_move_to(g_Cairo, 0, g_FontSize + 4);
    cairo_line_to(g_Cairo, m_nWidth, g_FontSize + 4);
    cairo_stroke(g_Cairo);

    cairo_restore(g_Cairo);
}

size_t Header::renderString(const char *str, size_t x, size_t y, uint32_t f, uint32_t b)
{
    x += m_pFont->render(str, x, y, f, b, false);
    return x;
}

void Header::update()
{
    size_t charWidth = m_pFont->getWidth();

    // Here we must run through the list of tabs and assign each a page number.
    size_t offset = 0;
    size_t page = 0;

    // Leave room for a left-arrow.
    offset += charWidth + 5;

    Tab *pTab = m_pTabs;
    while (pTab)
    {
        offset += 5 + strlen(pTab->text)*charWidth + 5;
        if (offset >= m_nWidth - charWidth - 5)
        {
            page ++;
            offset = charWidth + 5;
        }
        pTab->page = page;

        pTab = pTab->next;
    }

    m_LastPage = page;
}

void Header::select(size_t tabId)
{
    Tab *pTab = m_pTabs;
    while(pTab)
    {
        if (pTab->id == tabId)
            pTab->flags |= TAB_SELECTED;
        else
            pTab->flags &= ~TAB_SELECTED;
        pTab = pTab->next;
    }
}

void Header::centreOn(size_t tabId)
{
    Tab *pTab = m_pTabs;
    while(pTab)
    {
        if (pTab->id == tabId)
        {
            m_Page = pTab->page;
            return;
        }
        pTab = pTab->next;
    }
}

size_t Header::addTab(char *string, size_t flags)
{
    Tab *pTab = new Tab;
    pTab->text = string;
    pTab->id = m_NextTabId++;
    pTab->flags = flags;
    pTab->page = 0;
    pTab->next = 0;

    Tab *_pTab = m_pTabs;
    if (!m_pTabs)
    {
        m_pTabs = pTab;
        return pTab->id;
    }

    while(_pTab->next)
        _pTab = _pTab->next;
    _pTab->next = pTab;

    update();

    return pTab->id;
}

