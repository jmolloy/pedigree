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

#include "Header.h"
#include <pedigree_config.h>

rgb_t g_BorderColour                = {150, 80, 0,0};
rgb_t g_TabBackgroundColour         = {0, 0, 0,0};
rgb_t g_TabTextColour               = {150, 100, 0,0};
rgb_t g_SelectedTabBackgroundColour = {150, 100, 0,0};
rgb_t g_SelectedTabTextColour       = {255, 255, 255,0};
rgb_t g_TextColour                  = {255, 255, 255,0};
rgb_t g_MainBackgroundColour        = {0, 0, 0,0};
size_t g_FontSize = 16;

Header::Header(size_t nWidth) :
    m_nWidth(nWidth), m_Page(0), m_LastPage(0), m_pFont(0),
    m_pTabs(0), m_NextTabId(0)
{
    m_pFont = new Font(g_FontSize, "/system/fonts/DejaVuSansMono-BoldOblique.ttf", false, nWidth);
    g_FontSize = m_pFont->getHeight();

    int result = pedigree_config_query("select * from 'colour-scheme';");
    if (result == -1)
    {
        log("Unable to query config!");
        return;
    }

    char buf[256];
    if (pedigree_config_was_successful(result) != 0)
    {
        pedigree_config_get_error_message(result, buf, 256);
        log(buf);
        return;
    }

    while (pedigree_config_nextrow(result) == 0)
    {
        memset (buf, 0, 256);
        pedigree_config_getstr_n (result, 0, buf, 256);
        rgb_t rgb;
        rgb.r = pedigree_config_getnum_n (result, 1);
        rgb.g = pedigree_config_getnum_n (result, 2);
        rgb.b = pedigree_config_getnum_n (result, 3);
        rgb.a = 0;

        if (!strcmp(buf, "border"))
        {
            g_BorderColour = rgb;
            g_TabBackgroundColour = rgb;
        }
        else if (!strcmp(buf, "fill"))
            g_SelectedTabBackgroundColour = rgb;
        else if (!strcmp(buf, "selected-text"))
            g_SelectedTabTextColour = rgb;
        else if (!strcmp(buf, "text"))
        {
            g_TextColour = rgb;
            g_TabTextColour = rgb;
        }
        else if (!strcmp(buf, "tui-background"))
            g_MainBackgroundColour = rgb;
    }
    pedigree_config_freeresult(result);

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

    // Set up the dirty rectangle to cover the entire header area.
    rect.point(0, 0);
    rect.point(m_nWidth, g_FontSize+5);

    // Height = font size + 2 px top and bottom + border 1px =
    // font-size + 5px.
    for (size_t i = m_nWidth*(g_FontSize+4); i < m_nWidth*(g_FontSize+5); i++)
    {
        pBuffer[i].r = g_BorderColour.r;
        pBuffer[i].g = g_BorderColour.g;
        pBuffer[i].b = g_BorderColour.b;
    }

    size_t offset = 0;
    if (m_Page != 0)
    {
        // Render a left-double arrow.
        offset = renderString(pBuffer, "<", offset, 2, g_TextColour, g_MainBackgroundColour);
    }

    Tab *pTab = m_pTabs;
    while (pTab)
    {
        if (pTab->page == m_Page)
        {
            // Add 5 pixels.
            offset += 5;

            rgb_t foreColour = g_TextColour;
            if (pTab->flags & TAB_SELECTED)
                foreColour = g_SelectedTabTextColour;
            else if (pTab->flags & TAB_SELECTABLE)
                foreColour = g_TabTextColour;

            rgb_t backColour = g_MainBackgroundColour;
            if (pTab->flags & TAB_SELECTED)
                backColour = g_SelectedTabBackgroundColour;
            else if (pTab->flags & TAB_SELECTABLE)
                backColour = g_TabBackgroundColour;

            // Fill to background colour.
            Syscall::fillRect(pBuffer, offset-4, 0, strlen(pTab->text)*charWidth+10, g_FontSize+4, backColour);

            offset = renderString(pBuffer, pTab->text, offset, 2, foreColour, backColour);

            offset += 5;
            // Add a seperator.
            for (size_t i = offset; i < m_nWidth*(g_FontSize+4)+offset; i += m_nWidth)
            {
                pBuffer[i].r = g_BorderColour.r;
                pBuffer[i].g = g_BorderColour.g;
                pBuffer[i].b = g_BorderColour.b;
            }
        }
        pTab = pTab->next;
    }

    if (m_Page != m_LastPage)
    {
        offset += 5;
        // Render a right-double arrow.
        offset = renderString(pBuffer, ">", offset, 2, g_TextColour, g_MainBackgroundColour);
    }
}

size_t Header::renderString(rgb_t *pBuffer, const char *str, size_t x, size_t y, rgb_t f, rgb_t b)
{
    while (*str)
        x += m_pFont->render(pBuffer, *str++, x, y, f, b);
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
