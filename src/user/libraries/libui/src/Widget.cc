/*
 * Copyright (c) 2011 Matthew Iselin
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
#include <Widget.h>

Widget::Widget() : m_bConstructed(false), m_pFramebuffer(0)
{
}

Widget::~Widget()
{
}

bool Widget::construct(widgetCallback_t cb, PedigreeGraphics::Rect &dimensions)
{
    return false;
}

bool Widget::setProperty(std::string propName, void *propVal, size_t maxSize)
{
    return false;
}

bool Widget::getProperty(std::string propName, char **buffer, size_t maxSize)
{
    return false;
}

void Widget::setParent(Widget *pWidget)
{
}

Widget *Widget::getParent()
{
    return 0;
}

bool Widget::redraw(PedigreeGraphics::Rect &rt)
{
    return false;
}

bool Widget::visibility(bool vis)
{
    return false;
}

void Widget::destroy()
{
}

void Widget::checkForEvents()
{
}
