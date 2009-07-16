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

#ifndef HEADER_H
#define HEADER_H

#include "environment.h"
#include "colourSpace.h"
#include "Font.h"

#define TAB_SELECTABLE 0x1
#define TAB_SELECTED   0x2

class Header
{
public:
    Header(size_t nWidth);
    ~Header();

    size_t addTab(char *string, size_t flags);
    void removeTab(size_t id);

    void centreOn(size_t tabId);
    void select(size_t tabId);

    void render(rgb_t *pBuffer, DirtyRectangle &rect);

    size_t getHeight();

private:
    Header(const Header&);
    Header &operator = (const Header&);
    
    void update();
    size_t renderString(rgb_t *pBuffer, const char *str, size_t x, size_t y, rgb_t f, rgb_t b);

    size_t m_nWidth;
    size_t m_Page;
    size_t m_LastPage;
    Font *m_pFont;

    struct Tab
    {
        char *text;
        size_t id;
        size_t flags;
        size_t page;
        Tab *next;
    };
    Tab *m_pTabs;
    size_t m_NextTabId;
    
};

#endif
