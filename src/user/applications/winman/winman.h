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
#ifndef _WINMAN_H
#define _WINMAN_H

#include <stdint.h>
#include <syslog.h>

#include <list>

#include <graphics/Graphics.h>
#include <input/Input.h>
#include <ipc/Ipc.h>

/** \addtogroup PedigreeGUI
 *  @{
 */

class WObject;
class Container;
class Window;

/**
 * WObject: base class for a window manager object.
 */
class WObject
{
    public:
        enum Type
        {
            Container,
            Window,
        };

        WObject() : m_Dimensions(0, 0, 0, 0)
        {
        }

        virtual ~WObject()
        {
        }

        virtual Type getType() const = 0;

        virtual void resize(ssize_t horizDistance, ssize_t vertDistance, WObject *pChild = 0) = 0;

        void reposition(size_t x = ~0UL, size_t y = ~0UL, size_t w = ~0UL, size_t h = ~0UL);

        void bump(ssize_t bumpX = 0, ssize_t bumpY = 0);

        virtual void resized()
        {
        }

    protected:
        void setDimensions(PedigreeGraphics::Rect &rt)
        {
            m_Dimensions = rt;
        }

        PedigreeGraphics::Rect &getDimensions()
        {
            return m_Dimensions;
        }

    private:
        PedigreeGraphics::Rect m_Dimensions;
};

/**
 * Window: an actual window in the system, with actual rendering logic and
 * event handling.
 */
class Window : public WObject
{
    public:
        /// \todo move this constructor to objects.cc, and call addChild
        Window(::Container *pParent, PedigreeGraphics::Framebuffer *pBaseFramebuffer);
        Window();

        virtual ~Window()
        {
        }

        virtual Type getType() const
        {
            return WObject::Window;
        }

        virtual void render();

        virtual void resize(ssize_t horizDistance, ssize_t vertDistance, WObject *pChild = 0);

        virtual void focus();
        virtual void nofocus();

    private:
        ::Container *m_pParent;

        PedigreeGraphics::Framebuffer *m_pBaseFramebuffer;

        bool m_bFocus;
};

/**
 * Container: contains windows (and other containers) and defines layout
 * semantics, including management of the sizing of its own children.
 */
class Container : public WObject
{
    public:
        enum Layout
        {
            SideBySide, // Subwindows are tiled side-by-side
            Stacked, // Subwindows are tiled each above the other
        };

        Container(WObject *pParent) :
            m_Children(), m_pParent(pParent), m_Layout(SideBySide)
        {
        }

        virtual ~Container()
        {
        }

        virtual Type getType() const
        {
            return WObject::Container;
        }

        Layout getLayout() const
        {
            return m_Layout;
        }

        void setLayout(Layout newLayout)
        {
            m_Layout = newLayout;
            retile();
        }

        /**
         * Add a new child.
         */
        void addChild(WObject *pChild)
        {
            m_Children.push_back(pChild);
            retile();
        }

        /**
         * Taking our dimensions and layout into account, retile our children.
         * This will resize and reposition children, which may cause them to
         * retile also.
         */
        void retile();

        /**
         * Resize the entire container.
         * A child might ask us to do this if it is resized horizontally and
         * we are tiling Stacked, as it no longer fits inside our container.
         */
        virtual void resize(ssize_t horizDistance, ssize_t vertDistance, WObject *pChild = 0);

        /**
         * Render children
         */
        void render();

    protected:
        typedef std::list<WObject*> WObjectList_t;
        std::list<WObject*> m_Children;

        Container()
        {
            Container(0);
        }

    private:
        WObject *m_pParent;

        Layout m_Layout;
};

/**
 * Root container. Can't be resized, holds the first level of children.
 */
class RootContainer : public Container
{
    public:
        RootContainer(size_t w, size_t h) :
            Container()
        {
            reposition(0, 0, w, h);
        }

        virtual void resize(ssize_t horizDistance, ssize_t vertDistance, WObject *pChild = 0);
};

/** @} */

#endif
