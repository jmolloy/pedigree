/*
 * Copyright (c) 2010 James Molloy, Matthew Iselin
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

#include "winman.h"
#include <string.h>

#include <protocol.h>

#define WINDOW_BORDER_X 2
#define WINDOW_BORDER_Y 2
#define WINDOW_TITLE_H 20

// Start location of the client rendering area.
#define WINDOW_CLIENT_START_X (WINDOW_BORDER_X)
#define WINDOW_CLIENT_START_Y (WINDOW_BORDER_Y + WINDOW_TITLE_H)

// Insets from the end of the client area.
#define WINDOW_CLIENT_END_X   (WINDOW_BORDER_X)
#define WINDOW_CLIENT_END_Y   (WINDOW_BORDER_Y)

// Total space from the W/H that the window manager has used and taken from the
// client area for rendering decorations etc...
#define WINDOW_CLIENT_LOST_W (WINDOW_CLIENT_START_X + WINDOW_CLIENT_END_X + 1)
#define WINDOW_CLIENT_LOST_H (WINDOW_CLIENT_START_Y + WINDOW_CLIENT_END_Y + 1)

static size_t g_nextContextId = 1;

void WObject::reposition(size_t x, size_t y, size_t w, size_t h)
{
    if(x == ~0UL)
        x = m_Dimensions.getX();
    if(y == ~0UL)
        y = m_Dimensions.getY();
    if(w == ~0UL)
        w = m_Dimensions.getW();
    if(h == ~0UL)
        h = m_Dimensions.getH();

    PedigreeGraphics::Rect old = m_Dimensions;
    m_Dimensions.update(x, y, w, h);

    refreshContext(old);
}

void WObject::bump(ssize_t bumpX, ssize_t bumpY)
{
    size_t x = m_Dimensions.getX() + bumpX;
    size_t y = m_Dimensions.getY() + bumpY;
    size_t w = m_Dimensions.getW();
    size_t h = m_Dimensions.getH();
    m_Dimensions.update(x, y, w, h);
}

Window::Window(uint64_t handle, PedigreeIpc::IpcEndpoint *endpoint, ::Container *pParent) :
    m_Handle(handle), m_Endpoint(endpoint), m_pParent(pParent), m_Framebuffer(0), m_bFocus(false)
{
    PedigreeGraphics::Rect none;
    refreshContext(none);
    m_pParent->addChild(this);
}

void Window::refreshContext(PedigreeGraphics::Rect oldDimensions)
{
    delete m_Framebuffer;

    PedigreeGraphics::Rect &me = getDimensions();
    if((me.getW() < WINDOW_CLIENT_LOST_W) || (me.getH() < WINDOW_CLIENT_LOST_H))
    {
        // We have some basic requirements for window sizes.
        return;
    }

    /// \todo get a cairo context from somewhere, wipe out the old dimensions...

    // Size of the IPC region we need to allocate.
    size_t regionWidth = me.getW() - WINDOW_CLIENT_LOST_W;
    size_t regionHeight = me.getH() - WINDOW_CLIENT_LOST_H;
    int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, regionWidth);
    size_t regionSize = regionHeight * stride;
    m_Framebuffer = new PedigreeIpc::SharedIpcMessage(regionSize, 0);
    m_Framebuffer->initialise();
    memset(m_Framebuffer->getBuffer(), 0, regionSize);

    if(m_Endpoint && m_Framebuffer)
    {
        size_t totalSize =
                sizeof(LibUiProtocol::WindowManagerMessage) +
                sizeof(LibUiProtocol::RepositionMessage);

        PedigreeIpc::IpcMessage *pMessage = new PedigreeIpc::IpcMessage();
        pMessage->initialise();

        char *buffer = (char *) pMessage->getBuffer();

        LibUiProtocol::WindowManagerMessage *pHeader =
            reinterpret_cast<LibUiProtocol::WindowManagerMessage*>(buffer);
        pHeader->messageCode = LibUiProtocol::Reposition;
        pHeader->widgetHandle = m_Handle;
        pHeader->messageSize = sizeof(LibUiProtocol::RepositionMessage);
        pHeader->isResponse = false;

        LibUiProtocol::RepositionMessage *pReposition =
            reinterpret_cast<LibUiProtocol::RepositionMessage*>(buffer + sizeof(LibUiProtocol::WindowManagerMessage));
        pReposition->rt.update(0, 0, regionWidth, regionHeight);
        pReposition->shmem_handle = m_Framebuffer->getHandle();
        pReposition->shmem_size = regionSize;

        PedigreeIpc::send(m_Endpoint, pMessage, true);
    }
}

void Window::render(cairo_t *cr)
{
    // Render pretty window frames.
    PedigreeGraphics::Rect &me = getDimensions();
    size_t x = me.getX() + WINDOW_BORDER_X;
    size_t y = me.getY() + WINDOW_BORDER_Y;
    size_t w = me.getW() - (WINDOW_BORDER_X * 2);
    size_t h = me.getH() - (WINDOW_BORDER_Y * 2);

    double r = 0.0, g = 0.0, b = 0.0;
    if(m_bFocus)
        r = 1.0;
    else if(m_pParent->getFocusWindow() == this)
        r = 0.5;
    else
        r = g = b = 1.0;

    cairo_save(cr);

    // Blank out the window in full.
    cairo_set_line_width(cr, 0);
    cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.8);
    cairo_rectangle(cr, x, y, w, h);
    cairo_fill(cr);

    // Draw the child framebuffer before window decorations.
    void *pBuffer = getFramebuffer();
    if(pBuffer)
    {
        size_t regionWidth = me.getW() - WINDOW_CLIENT_LOST_W;
        size_t regionHeight = me.getH() - WINDOW_CLIENT_LOST_H;
        int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, regionWidth);

        cairo_surface_t *surface = cairo_image_surface_create_for_data(
                (uint8_t *) pBuffer,
                CAIRO_FORMAT_ARGB32,
                regionWidth,
                regionHeight,
                stride);

        cairo_translate(
                cr,
                me.getX() + WINDOW_CLIENT_START_X,
                me.getY() + WINDOW_CLIENT_START_Y);

        cairo_set_source_surface(cr, surface, 0, 0);

        cairo_paint(cr);

        cairo_surface_destroy(surface);

        cairo_identity_matrix(cr);
    }

    cairo_set_line_cap(cr, CAIRO_LINE_CAP_SQUARE);
    cairo_set_line_join(cr, CAIRO_LINE_JOIN_MITER);
    cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);

    // Window title bar.
    cairo_set_line_width(cr, 0);
    cairo_set_source_rgba(cr, 0.2, 0.3, 0.3, 1.0);
    cairo_rectangle(cr, x, y, w, WINDOW_TITLE_H);
    cairo_fill(cr);

    // Window title.
    cairo_text_extents_t extents;
    cairo_set_font_size(cr, 13);
    cairo_text_extents(cr, m_sWindowTitle.c_str(), &extents);
    cairo_move_to(cr, me.getX() + ((w / 2) - (extents.width / 2)), me.getY() + WINDOW_CLIENT_START_Y - (extents.height / 2));
    cairo_set_source_rgba(cr, 0.7, 0.7, 0.7, 1.0);
    cairo_show_text(cr, m_sWindowTitle.c_str());

    // Window border.
    cairo_set_source_rgba(cr, r, g, b, 1.0);
    cairo_set_line_width(cr, 1.0);

    cairo_rectangle(cr, x, y, w, h);
    cairo_stroke(cr);

    cairo_restore(cr);
}

void Window::focus()
{
    m_bFocus = true;
    m_pParent->setFocusWindow(this);
}

void Window::nofocus()
{
    m_bFocus = false;
}

void Window::resize(ssize_t horizDistance, ssize_t vertDistance, WObject *pChild)
{
    PedigreeGraphics::Rect &me = getDimensions();
    size_t currentWidth = me.getW();
    size_t currentHeight = me.getH();

    reposition(
        me.getX(), me.getY(),
        currentWidth + horizDistance, currentHeight + vertDistance);
}

void Container::retile()
{
    if(m_Children.size() == 0)
    {
        syslog(LOG_INFO, "winman: no children in container, no retile");
        return;
    }

    PedigreeGraphics::Rect &me = getDimensions();
    size_t currentWidth = me.getW();
    size_t currentHeight = me.getH();

    if(m_Layout == SideBySide)
    {
        size_t dividedW = currentWidth / m_Children.size();
        size_t newX = me.getX();
        size_t newY = me.getY();

        WObjectList_t::iterator it = m_Children.begin();
        for(; it != m_Children.end(); ++it)
        {
            (*it)->reposition(newX, newY, dividedW, currentHeight);
            newX += dividedW;
        }
    }
    else if(m_Layout == Stacked)
    {
        size_t dividedH = currentHeight / m_Children.size();
        size_t newX = me.getX();
        size_t newY = me.getY();

        WObjectList_t::iterator it = m_Children.begin();
        for(; it != m_Children.end(); ++it)
        {
            (*it)->reposition(newX, newY, currentWidth, dividedH);
            newY += dividedH;
        }
    }

    // Retile child containers (reposition does not do that)
    WObjectList_t::iterator it = m_Children.begin();
    for(; it != m_Children.end(); ++it)
    {
        if((*it)->getType() == WObject::Container ||
            (*it)->getType() == WObject::Root)
        {
            Container *pContainer = static_cast<Container*>(*it);
            pContainer->retile();
        }
    }
}

void Container::resize(ssize_t horizDistance, ssize_t vertDistance, WObject *pChild)
{
    PedigreeGraphics::Rect &me = getDimensions();
    size_t currentWidth = me.getW();
    size_t currentHeight = me.getH();

    if(pChild)
    {
        ssize_t bumpX = horizDistance;
        ssize_t bumpY = vertDistance;

        horizDistance = -horizDistance;
        vertDistance = -vertDistance;

        WObjectList_t::iterator it = m_Children.begin();
        bool bResize = false;
        for(; it != m_Children.end(); ++it)
        {
            if(bResize)
            {
                (*it)->bump(bumpX, bumpY); // Bump X/Y co-ords.
                (*it)->resize(horizDistance, vertDistance);
                return; // Resize will cascade.
            }

            if((*it) == pChild)
            {
                bResize = true;
            }
        }

        if(!bResize)
            syslog(LOG_INFO, "winman: didn't find children for resize???");
    }
    else
    {
        reposition(
            me.getX(), me.getY(),
            currentWidth + horizDistance, currentHeight + vertDistance);

        if(m_Children.size())
        {
            // We need tiles to fill the space created by resizing.
            // For most resizes, we'll be going one direction, but if
            // we get resized vertically and horizontally, this handles
            // that nicely.
            // Is this right?
            ssize_t extendW = horizDistance / static_cast<ssize_t>(m_Children.size());
            ssize_t extendH = vertDistance / static_cast<ssize_t>(m_Children.size());

            // Resize children in the relevant direction for them.
            WObjectList_t::iterator it = m_Children.begin();
            size_t bumpX = 0;
            for(; it != m_Children.end(); ++it)
            {
                if(m_Layout == Stacked)
                {
                    (*it)->resize(horizDistance, extendH);
                }
                else if(m_Layout == SideBySide)
                {
                    (*it)->bump(bumpX, 0);
                    (*it)->resize(extendW, vertDistance);

                    bumpX += extendW;
                }
            }
        }
    }

    // Resize siblings.
    if(m_pParent && ((m_pParent->getType() == WObject::Container) ||
        m_pParent->getType() == WObject::Root))
    {
        Container *pContainer = static_cast<Container*>(m_pParent);

        pContainer->resize(horizDistance, vertDistance, static_cast<WObject*>(this));
    }
}

void Container::render(cairo_t *cr)
{
    WObjectList_t::iterator it = m_Children.begin();
    for(; it != m_Children.end(); ++it)
    {
        if((*it)->getType() == WObject::Container)
        {
            Container *pContainer = static_cast<Container*>(*it);
            pContainer->render(cr);
        }
        else if((*it)->getType() == WObject::Window)
        {
            ::Window *pWindow = static_cast< ::Window*>(*it);
            pWindow->render(cr);
        }
    }
}

WObject *Container::getLeftSibling(const WObject *pChild) const
{
    WObjectList_t::const_iterator it = m_Children.begin();
    for(; it != m_Children.end(); ++it)
    {
        if((*it) == pChild)
        {
            if(it == m_Children.begin())
            {
                break;
            }

            --it;
            return (*it);
        }
    }

    return 0;
}

WObject *Container::getRightSibling(const WObject *pChild) const
{
    WObjectList_t::const_iterator it = m_Children.begin();
    for(; it != m_Children.end(); ++it)
    {
        if((*it) == pChild)
        {
            ++it;

            if(it == m_Children.end())
            {
                break;
            }

            return (*it);
        }
    }

    return 0;
}

WObject *Container::getLeftObject() const
{
    return getLeftSibling(this);
}

WObject *Container::getRightObject() const
{
    return getRightSibling(this);
}

WObject *Container::getLeft(const WObject *obj) const
{
    WObject *sibling = 0;
    if(m_Layout == SideBySide)
    {
        WObject *sibling = getLeftSibling(obj);
        if(sibling)
        {
            return sibling;
        }
    }

    // No sibling to the child. If we are inside a side-by-side layout,
    // this could be trivial.
    if(m_pParent && (m_pParent->getType() == WObject::Container))
    {
        const Container *pContainer = static_cast<const Container*>(m_pParent);
        if(pContainer->getLayout() == SideBySide)
        {
            sibling = getLeftObject();
            if(sibling)
            {
                return sibling;
            }
        }
    }

    // Root has no parent.
    if(getType() != WObject::Root)
    {
        const Container *pContainer = static_cast<const Container*>(m_pParent);
        return pContainer->getLeft(this);
    }

    return 0;
}

WObject *Container::getRight(const WObject *obj) const
{
    WObject *sibling = 0;
    if(m_Layout == SideBySide)
    {
        WObject *sibling = getRightSibling(obj);
        if(sibling)
        {
            return sibling;
        }
    }

    // No sibling to the child. If we are inside a side-by-side layout,
    // this could be trivial.
    if(m_pParent && (m_pParent->getType() == WObject::Container))
    {
        const Container *pContainer = static_cast<const Container*>(m_pParent);
        if(pContainer->getLayout() == SideBySide)
        {
            sibling = getRightObject();
            if(sibling)
            {
                return sibling;
            }
        }
    }

    // Root has no parent.
    if(getType() != WObject::Root)
    {
        const Container *pContainer = static_cast<const Container*>(m_pParent);
        return pContainer->getRight(this);
    }

    return 0;
}

WObject *Container::getUp(const WObject *obj) const
{
    WObject *sibling = 0;
    if(m_Layout == Stacked)
    {
        WObject *sibling = getLeftSibling(obj);
        if(sibling)
        {
            return sibling;
        }
    }

    // No sibling to the child. If we are inside a stacked layout,
    // this could be trivial.
    if(m_pParent && (m_pParent->getType() == WObject::Container))
    {
        const Container *pContainer = static_cast<const Container*>(m_pParent);
        if(pContainer->getLayout() == Stacked)
        {
            sibling = getLeftObject();
            if(sibling)
            {
                return sibling;
            }
        }
    }

    // Root has no parent.
    if(getType() != WObject::Root)
    {
        const Container *pContainer = static_cast<const Container*>(m_pParent);
        return pContainer->getUp(this);
    }

    return 0;
}

WObject *Container::getDown(const WObject *obj) const
{
    WObject *sibling = 0;
    if(m_Layout == Stacked)
    {
        WObject *sibling = getRightSibling(obj);
        if(sibling)
        {
            return sibling;
        }
    }

    // No sibling to the child. If we are inside a stacked layout,
    // this could be trivial.
    if(m_pParent && (m_pParent->getType() == WObject::Container))
    {
        const Container *pContainer = static_cast<const Container*>(m_pParent);
        if(pContainer->getLayout() == Stacked)
        {
            sibling = getRightObject();
            if(sibling)
            {
                return sibling;
            }
        }
    }

    // Root has no parent.
    if(getType() != WObject::Root)
    {
        const Container *pContainer = static_cast<const Container*>(m_pParent);
        return pContainer->getDown(this);
    }

    return 0;
}

void RootContainer::resize(ssize_t horizDistance, ssize_t vertDistance, WObject *pChild)
{
    if(pChild)
    {
        // Use the Container implementation to resize siblings of the
        // child that we've been passed.
        Container::resize(horizDistance, vertDistance, pChild);
    }
}
