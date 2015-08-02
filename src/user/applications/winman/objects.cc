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

#include "winman.h"
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>

#include <protocol.h>

#include "util.h"

static size_t g_nextContextId = 1;

DirtyRectangle::DirtyRectangle() :
    m_X(~0), m_Y(~0), m_X2(0), m_Y2(0)
{
}

DirtyRectangle::~DirtyRectangle()
{
}

void DirtyRectangle::point(size_t x, size_t y)
{
    if (x < m_X)
        m_X = x;
    if (x > m_X2)
        m_X2 = x;

    if (y < m_Y)
        m_Y = y;
    if (y > m_Y2)
        m_Y2 = y;
}

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

    m_Dimensions.update(x, y, w, h);

    refreshContext();
}

void WObject::bump(ssize_t bumpX, ssize_t bumpY)
{
    size_t x = m_Dimensions.getX() + bumpX;
    size_t y = m_Dimensions.getY() + bumpY;
    size_t w = m_Dimensions.getW();
    size_t h = m_Dimensions.getH();
    m_Dimensions.update(x, y, w, h);
}

Window::Window(uint64_t handle, int sock, struct sockaddr *sa, size_t sa_len,
               ::Container *pParent) :
    m_Handle(handle), m_pParent(pParent), m_Framebuffer(0), m_Dirty(),
    m_bPendingDecoration(false), m_bFocus(false), m_bRefresh(true),
    m_nRegionWidth(0), m_nRegionHeight(0), m_Socket(sock), m_Sa(sa),
    m_SaLen(sa_len)
{
    refreshContext();
    m_pParent->addChild(this);
}

Window::~Window()
{
    /// \todo Need a way to destroy the old framebuffer without breaking
    ///       the other side's reference to the same region... Refcount?
#ifdef TARGET_LINUX
    delete m_Framebuffer;
#endif
    delete m_Sa;
}

void Window::refreshContext()
{
    PedigreeGraphics::Rect &me = getDimensions();
    if(!m_bRefresh)
    {
        // Not refreshing context currently.
        m_bPendingDecoration = true;
        syslog(LOG_DEBUG, "not refreshing context, marking for redecoration");
        return;
    }

    if((me.getW() < WINDOW_CLIENT_LOST_W) || (me.getH() < WINDOW_CLIENT_LOST_H))
    {
        // We have some basic requirements for window sizes.
        syslog(LOG_DEBUG, "extents %ux%u are too small for a new context",
               me.getW(), me.getH());
        return;
    }

    syslog(LOG_DEBUG, "destroying old framebuffer...");
    delete m_Framebuffer;
    m_Framebuffer = 0;
    syslog(LOG_DEBUG, "destroying old framebuffer complete");

    // Size of the IPC region we need to allocate.
    size_t regionWidth = me.getW() - WINDOW_CLIENT_LOST_W;
    size_t regionHeight = me.getH() - WINDOW_CLIENT_LOST_H;
    int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, regionWidth);
    size_t regionSize = regionHeight * stride;
    m_Framebuffer = new SharedBuffer(regionSize);
    memset(m_Framebuffer->getBuffer(), 0, regionSize);

    syslog(LOG_DEBUG, "new framebuffer created: %zd bytes @%p", regionSize,
           m_Framebuffer->getBuffer());

    m_nRegionWidth = regionWidth;
    m_nRegionHeight = regionHeight;

    if((m_Socket >= 0) && m_Framebuffer)
    {
        size_t totalSize =
                sizeof(LibUiProtocol::WindowManagerMessage) +
                sizeof(LibUiProtocol::RepositionMessage);
        char *buffer = new char[totalSize];
        memset(buffer, 0, totalSize);

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

        // Transmit to the client.
        sendMessage(buffer, totalSize);

        delete [] buffer;
    }

    m_bPendingDecoration = true;
}

void *Window::getFramebuffer() const
{
    return m_Framebuffer ? m_Framebuffer->getBuffer() : 0;
}

void Window::sendMessage(const char *msg, size_t len)
{
    struct sockaddr_un *sun = (struct sockaddr_un *) m_Sa;
    sendto(m_Socket, msg, len, 0, m_Sa, m_SaLen);
}

void Window::setDirty(PedigreeGraphics::Rect &dirty)
{
    PedigreeGraphics::Rect &me = getDimensions();

    size_t realX = dirty.getX();
    size_t realY = dirty.getY();
    size_t realW = dirty.getW();
    size_t realH = dirty.getH();

    size_t clientW = me.getW() - WINDOW_CLIENT_LOST_W;
    size_t clientH = me.getH() - WINDOW_CLIENT_LOST_H;

    if(realX > clientW || realY > clientH)
    {
        return;
    }

    if((realX + realW) > clientW)
    {
        realW = clientW - realX;
    }

    if((realY + realH) > clientH)
    {
        realH = clientH - realY;
    }

    m_Dirty.update(realX + WINDOW_CLIENT_START_X, realY + WINDOW_CLIENT_START_Y, realW, realH);
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

    // Draw the child framebuffer before window decorations.
    void *pBuffer = getFramebuffer();
    if(pBuffer && (isDirty() && m_bRefresh))
    {
        cairo_save(cr);
        size_t regionWidth = m_nRegionWidth;
        size_t regionHeight = m_nRegionHeight;
        int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, regionWidth);

        cairo_surface_t *surface = cairo_image_surface_create_for_data(
                (uint8_t *) pBuffer,
                CAIRO_FORMAT_ARGB32,
                regionWidth,
                regionHeight,
                stride);

        PedigreeGraphics::Rect realDirty = getDirty();
        size_t dirtyX = realDirty.getX();
        size_t dirtyY = realDirty.getY();
        size_t dirtyWidth = realDirty.getW();
        size_t dirtyHeight = realDirty.getH();

        if(dirtyX < WINDOW_CLIENT_START_X)
        {
            dirtyX = WINDOW_CLIENT_START_X;
        }
        if(dirtyY < WINDOW_CLIENT_START_Y)
        {
            dirtyY = WINDOW_CLIENT_START_Y;
        }
        if((dirtyX + dirtyWidth) > regionWidth)
        {
            dirtyWidth = regionWidth;
        }
        if((dirtyY + dirtyHeight) > regionHeight)
        {
            dirtyHeight = regionHeight;
        }

        cairo_set_source_surface(cr, surface, me.getX() + WINDOW_CLIENT_START_X, me.getY() + WINDOW_CLIENT_START_Y);

        // Clip to the dirty rectangle (don't update anything more)
        cairo_rectangle(
                cr,
                me.getX() + realDirty.getX(),
                me.getY() + realDirty.getY(),
                realDirty.getW(),
                realDirty.getH());
        cairo_clip(cr);

        // Clip to the window only (fixes rendering glitches during resize)
        cairo_rectangle(
                cr,
                me.getX() + WINDOW_CLIENT_START_X,
                me.getY() + WINDOW_CLIENT_START_Y,
                me.getW() - WINDOW_CLIENT_LOST_W,
                me.getH() - WINDOW_CLIENT_LOST_H);
        cairo_clip(cr);

        // cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
        cairo_paint(cr);

        cairo_surface_destroy(surface);
        cairo_restore(cr);

        // No longer dirty - rendered.
        m_Dirty.update(0, 0, 0, 0);

        // Send back a response to ACK any pending redraw that's waiting.
        // We wait until after we perform the redraw to permit the client to
        // continue, as it doesn't make sense for the client to keep hitting
        // us with redraw messages when we aren't ready.
        LibUiProtocol::WindowManagerMessage ackmsg;
        memset(&ackmsg, 0, sizeof(ackmsg));
        ackmsg.messageCode = LibUiProtocol::RequestRedraw;
        ackmsg.widgetHandle = m_Handle;
        ackmsg.messageSize = 0;
        ackmsg.isResponse = true;
        sendMessage((const char *) &ackmsg, sizeof(ackmsg));
    }

    if(m_bPendingDecoration)
    {
        cairo_save(cr);
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
        cairo_move_to(cr, me.getX() + ((w / 2) - (extents.width / 2)), me.getY() + WINDOW_CLIENT_START_Y - 3);
        cairo_set_source_rgba(cr, 0.7, 0.7, 0.7, 1.0);
        cairo_show_text(cr, m_sWindowTitle.c_str());

        // Window border.
        cairo_set_source_rgba(cr, r, g, b, 1.0);
        cairo_set_line_width(cr, 1.0);

        cairo_rectangle(cr, x, y, w, h);
        cairo_stroke(cr);
        cairo_restore(cr);
    }

    m_bPendingDecoration = false;
}

void Window::focus()
{
    m_bFocus = true;
    m_pParent->setFocusWindow(this);
    m_bPendingDecoration = true;

    LibUiProtocol::WindowManagerMessage *pHeader =
        new LibUiProtocol::WindowManagerMessage;
    memset(pHeader, 0, sizeof(*pHeader));
    pHeader->messageCode = LibUiProtocol::Focus;
    pHeader->widgetHandle = m_Handle;
    pHeader->messageSize = 0;
    pHeader->isResponse = false;

    sendMessage((const char *) pHeader, sizeof(*pHeader));
    delete pHeader;
}

void Window::nofocus()
{
    m_bFocus = false;
    m_bPendingDecoration = true;

    LibUiProtocol::WindowManagerMessage *pHeader =
        new LibUiProtocol::WindowManagerMessage;
    memset(pHeader, 0, sizeof(*pHeader));
    pHeader->messageCode = LibUiProtocol::NoFocus;
    pHeader->widgetHandle = m_Handle;
    pHeader->messageSize = 0;
    pHeader->isResponse = false;

    sendMessage((const char *) pHeader, sizeof(*pHeader));
    delete pHeader;
}

void Window::resize(ssize_t horizDistance, ssize_t vertDistance, WObject *pChild)
{
    PedigreeGraphics::Rect &me = getDimensions();
    size_t currentWidth = me.getW();
    size_t currentHeight = me.getH();

    reposition(
        me.getX(), me.getY(),
        currentWidth + horizDistance, currentHeight + vertDistance);

    // Pass resize up to parent to arrange other tiles to resize around this
    // one, as necessary.
    m_pParent->resize(horizDistance, vertDistance, this);

    m_bPendingDecoration = true;
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

/// Don't refresh the context on every reposition.
void Container::norefresh()
{
    WObjectList_t::const_iterator it = m_Children.begin();
    for(; it != m_Children.end(); ++it)
    {
        (*it)->norefresh();
    }
}

/// Refresh context on every reposition.
void Container::yesrefresh()
{
    WObjectList_t::const_iterator it = m_Children.begin();
    for(; it != m_Children.end(); ++it)
    {
        (*it)->yesrefresh();
    }
}
