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
#include <string>

#include <graphics/Graphics.h>
#include <input/Input.h>
#include <ipc/Ipc.h>

#include <cairo/cairo.h>

/** \addtogroup PedigreeGUI
 *  @{
 */

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

/**
 * DirtyRectangle: receives a number of points, uses these to determine the full
 * extent that has been modified in the lifetime of the object. This can be used
 * to calculate the full amount of screen space that needs to be redrawn.
 */
class DirtyRectangle
{
public:
    DirtyRectangle();
    ~DirtyRectangle();

    void point(size_t x, size_t y);

    size_t getX() const {return m_X;}
    size_t getY() const {return m_Y;}
    size_t getX2() const {return m_X2;}
    size_t getY2() const {return m_Y2;}
    size_t getWidth() const {return m_X2-m_X+1;}
    size_t getHeight() const {return m_Y2-m_Y+1;}

    void reset()
    {
        m_X = 0; m_Y = 0; m_X2 = 0; m_X2 = 0;
    }

private:
    size_t m_X, m_Y, m_X2, m_Y2;
};

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

        PedigreeGraphics::Rect getCopyDimensions() const
        {
            return m_Dimensions;
        }

        /// Don't refresh the context on every reposition.
        virtual void norefresh()
        {
        }

        /// Refresh context on every reposition.
        virtual void yesrefresh()
        {
        }

        //// Render.
        virtual void render(cairo_t *cr)
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
        Window(uint64_t handle, int sock, ::Container *pParent);
        Window();

        virtual ~Window()
        {
        }

        virtual Type getType() const
        {
            return WObject::Window;
        }

        virtual void setTitle(const std::string &s)
        {
            m_sWindowTitle = s;
        }

        virtual void render(cairo_t *cr);

        virtual void resize(ssize_t horizDistance, ssize_t vertDistance, WObject *pChild = 0);

        virtual void focus();
        virtual void nofocus();

        /// Don't refresh the context on every reposition.
        virtual void norefresh()
        {
            m_bRefresh = false;
        }

        /// Refresh context on every reposition.
        virtual void yesrefresh()
        {
            m_bRefresh = true;
            refreshContext();
        }

        virtual void refreshContext();

        void *getFramebuffer() const
        {
            return m_Framebuffer ? m_Framebuffer->getBuffer() : 0;
        }

        // virtual void sendMessage(const char *msg, size_t len);

        PedigreeIpc::IpcEndpoint *getEndpoint() const
        {
            return m_Endpoint;
        }

        int getSocket() const
        {
            return m_Socket;
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

        void setDirty(PedigreeGraphics::Rect &dirty);

        PedigreeGraphics::Rect getDirty() const
        {
            // Different behaviour if we are waiting on a window redecoration
            if(m_bPendingDecoration)
            {
                // Redraw ALL the things.
                PedigreeGraphics::Rect rt = getCopyDimensions();
                rt.update(0, 0, rt.getW(), rt.getH());
                return rt;
            }
            return m_Dirty;
        }

        bool isDirty() const
        {
            return m_bPendingDecoration || isClientDirty();
        }

    private:
        bool isClientDirty() const
        {
            return !(
                m_Dirty.getX() == 0 &&
                m_Dirty.getY() == 0 &&
                m_Dirty.getW() == 0 &&
                m_Dirty.getH() == 0);
        }

        uint64_t m_Handle;

        PedigreeIpc::IpcEndpoint *m_Endpoint;

        ::Container *m_pParent;

        PedigreeIpc::SharedIpcMessage *m_Framebuffer;

        std::string m_sWindowTitle;

        PedigreeGraphics::Rect m_Dirty;

        bool m_bPendingDecoration;

        bool m_bFocus;

        bool m_bRefresh;

        size_t m_nRegionWidth;
        size_t m_nRegionHeight;

        int m_Socket;
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
         * Parent of this container.
         */
        WObject *getParent() const
        {
            return m_pParent;
        }

        /**
         * Resize the entire container.
         * A child might ask us to do this if it is resized horizontally and
         * we are tiling Stacked, as it no longer fits inside our container.
         */
        virtual void resize(ssize_t horizDistance, ssize_t vertDistance, WObject *pChild = 0);

        /**
         * Render children
         */
        void render(cairo_t *cr);

        /// Don't refresh the context on every reposition.
        virtual void norefresh();

        /// Refresh context on every reposition.
        virtual void yesrefresh();

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
