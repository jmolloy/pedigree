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

#include <vector>

#include <graphics/Graphics.h>
#include <input/Input.h>
#include <ipc/Ipc.h>

/** \addtogroup PedigreeGUI
 *  @{
 */

class WObject;
class Container;
class RootContainer;
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
            Root,
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

        /// Refresh our graphical context, called after reposition.
        virtual void refreshContext()
        {
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
        Window(uint64_t handle, PedigreeIpc::IpcEndpoint *endpoint, ::Container *pParent, PedigreeGraphics::Framebuffer *pBaseFramebuffer);
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

        virtual void refreshContext();

        PedigreeGraphics::Framebuffer *getContext() const
        {
            return m_pRealFramebuffer;
        }

        PedigreeIpc::IpcEndpoint *getEndpoint() const
        {
            return m_Endpoint;
        }

        uint64_t getHandle() const
        {
            return m_Handle;
        }

        ::Container *getParent() const
        {
            return m_pParent;
        }

        void setParent(::Container *p)
        {
            m_pParent = p;
        }

    private:
        uint64_t m_Handle;

        PedigreeIpc::IpcEndpoint *m_Endpoint;

        ::Container *m_pParent;

        PedigreeGraphics::Framebuffer *m_pBaseFramebuffer;
        PedigreeGraphics::Framebuffer *m_pRealFramebuffer;

        bool m_bFocus;
};

/**
 * Container: contains windows (and other containers) and defines layout
 * semantics, including management of the sizing of its own children.
 */
class Container : public WObject
{
    protected:
        typedef std::vector<WObject*> WObjectList_t;

    public:
        enum Layout
        {
            SideBySide, // Subwindows are tiled side-by-side
            Stacked, // Subwindows are tiled each above the other
        };

        Container(WObject *pParent) :
            m_Children(), m_pParent(pParent), m_Layout(SideBySide), m_pFocusWindow(0)
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

        ::Window *getFocusWindow() const
        {
            return m_pFocusWindow;
        }

        void setFocusWindow(::Window *w)
        {
            m_pFocusWindow = w;
        }

        /**
         * Add a new child.
         */
        void addChild(WObject *pChild, bool bNoRetile = false)
        {
            // insertion breaks retile() somehow.
            //insertChild(m_pFocusWindow, pChild);
            m_Children.push_back(pChild);
            if((pChild->getType() == WObject::Window) && (m_pFocusWindow == 0))
            {
                m_pFocusWindow = static_cast< ::Window*>(pChild);
            }

            if(!bNoRetile)
            {
                retile();
            }
        }

        /**
         * Replaces a child.
         */
        void replaceChild(WObject *pChild, WObject *pNewChild)
        {
            WObjectList_t::iterator it = m_Children.begin();
            for(; it != m_Children.end(); ++it)
            {
                if((*it) == pChild)
                {
                    it = m_Children.erase(it);
                    m_Children.insert(it, pNewChild);
                    break;
                }
            }
        }

        /**
         * Inserts a child after the given child, or at the end if
         * pCurrent is null.
         */
        void insertChild(WObject *pCurrent, WObject *pNewChild)
        {
            WObjectList_t::iterator it = m_Children.begin();
            for(; it != m_Children.end(); ++it)
            {
                if((*it) == pCurrent)
                {
                    ++it;
                    it = m_Children.insert(it, pNewChild);
                    break;
                }
            }

            if(it == m_Children.end())
            {
                m_Children.push_back(pNewChild);
            }

            retile();
        }

        /**
         * Removes the given child.
         */
        void removeChild(WObject *pChild)
        {
            WObjectList_t::iterator it = m_Children.begin();
            for(; it != m_Children.end(); ++it)
            {
                if((*it) == pChild)
                {
                    m_Children.erase(it);
                    break;
                }
            }

            // Did we actually erase something?
            if(it != m_Children.end())
            {
                retile();
            }
        }

        /**
         * Gets the number of children in the container.
         */
        size_t getChildCount() const
        {
            return m_Children.size();
        }

        /**
         * Gets the nth child in the container.
         */
        WObject *getChild(size_t n) const
        {
            if(n > m_Children.size())
            {
                return 0;
            }

            return m_Children[n];
        }

        /**
         * Finds the left sibling of the given child.
         * Note that in the 'Stacked' layout, this is the container above the
         * child, not the container to its left.
         */
        WObject *getLeftSibling(const WObject *pChild) const;

        /**
         * Finds the right sibling of the given child.
         * Note that in the 'Stacked' layout, this is the container below the
         * child, not the container to its right.
         */
        WObject *getRightSibling(const WObject *pChild) const;

        /**
         * Finds any object to our left, other than the root container.
         * This is the left of the container itself.
         */
        WObject *getLeftObject() const;

        /**
         * Finds any object to our right, other than the root container.
         * This is the right of the container itself.
         */
        WObject *getRightObject() const;

        /**
         * Gets the object to the left of the given child, or 0 if no
         * object is to the left of the given child.
         * This is a VISUAL left.
         */
        WObject *getLeft(const WObject *obj) const;

        /**
         * Gets the object to the right of the given child, or 0 if no
         * object is to the right of the given child.
         * This is a VISUAL right.
         */
        WObject *getRight(const WObject *obj) const;

        /**
         * Gets the object above the given child, or 0 if no object is
         * above the given child.
         * This is a VISUAL above.
         */
        WObject *getUp(const WObject *obj) const;

        /**
         * Gets the object below the given child, or 0 if no object is
         * below the given child.
         * This is a VISUAL below.
         */
        WObject *getDown(const WObject *obj) const;

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
        std::vector<WObject*> m_Children;

        Container()
        {
            Container(0);
        }

    private:
        WObject *m_pParent;

        Layout m_Layout;

        ::Window *m_pFocusWindow;
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

        virtual Type getType() const
        {
            return WObject::Root;
        }

        virtual void resize(ssize_t horizDistance, ssize_t vertDistance, WObject *pChild = 0);
};

/** @} */

#endif
