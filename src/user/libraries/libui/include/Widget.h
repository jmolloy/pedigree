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
#ifndef _WIDGET_H
#define _WIDGET_H

#include <types.h>
#include <graphics/Graphics.h>

#include <string>

/** \addtogroup PedigreeGUI
 *  @{
 */

enum WidgetMessages
{
    RepaintNeeded,
    MouseMove,
    MouseDown,
    MouseUp,
    KeyDown,
    KeyUp
};

typedef bool (*widgetCallback_t)(WidgetMessages, size_t, void *);

/// Base class for all GUI widgets in the system. Defines the interface for all
/// widgets to use.
class Widget
{
    public:

        Widget();
        virtual ~Widget();

        /**
         * Obtains a handle for this widget. The handle is only usable by the
         * window manager, and is determined by the following formula during
         * widget construction in construct():
         * (PID << 32) | ((WidgetPointer >> 32) | (WidgetPointer & 0xFFFFFFFF))
         * If the system is not capable of 64-bit addressing, the 32-bit right
         * shift is not necessary.
         * The PID is assumed to never exceed the limits of a 32-bit unsigned
         * integer. Should this happen, the PID should be folded in the same way
         * as the widget pointer.
         */
        inline uint64_t getHandle()
        {
            return m_Handle;
        }

        /**
         * Constructs the internal state of the widget. This involves talking to
         * the window manager via IPC to obtain a framebuffer for rendering, as
         * well as notifying the window manager that this widget now exists.
         * No other calls are valid until this call has returned successfully.
         * \param cb Event handler callback.
         * \param dimensions Dimensions of the widget to be created.
         * \return True if the widget was successfully constructed, False otherwise.
         */
        bool construct(widgetCallback_t cb, PedigreeGraphics::Rect &dimensions);

        /**
         * Sets an internal property of the widget. Properties can be read by
         * the window manager, or created and read by the application developer
         * for an internal use. Creating a property is as simple as passing a
         * string that has not been used as a property name yet.
         * \note The window manager may choose to ignore or reject setting some
         *       properties if it determines changing the property would have a
         *       negative impact on the system or widget.
         * \param propName Name of the property to set.
         * \param propVal Pointer to a buffer containing the value to set.
         * \param maxSize Size of the buffer pointed to by propVal.
         * \return True if the property was set, False otherwise.
         */
        bool setProperty(std::string propName, void *propVal, size_t maxSize);

        /**
         * Gets an internal property of the widget.
         * \param propName Name of the property to set.
         * \param buffer Pointer to a pointer to a buffer to hold the value.
         * \param maxSize Maximum size of the buffer to read.
         * \return True if the read was successful, False otherwise.
         */
        bool getProperty(std::string propName, char **buffer, size_t maxSize);

        /**
         * Sets the parent of this widget. This will notify the window manager
         * so that the repaint ordering can be rearranged.
         * \note Should be called before any rendering takes place, as there is
         *       no "default parent".
         * \param pWidget Pointer to a widget to set as the new parent.
         */
        void setParent(Widget *pWidget);

        /**
         * Gets the current parent of this widget.
         * \return Pointer to the current parent of this widget. Null if there
         *         is no parent.
         */
        Widget *getParent();

        /**
         * Called to actually render the widget. Would normally be called by the
         * RepaintNeeded handler by application code.
         * \param rt Area that needs to be rendered.
         * \param dirty Object containing a dirty rectangle for redrawing.
         * \return True if the render succeeded, False otherwise.
         */
        virtual bool render(PedigreeGraphics::Rect &rt, PedigreeGraphics::Rect &dirty) = 0;

        /**
         * Called to begin a redraw of the widget by the window manager.
         * \param rt Rect defining the redraw area.
         * \return True if the redraw was successful, False otherwise.
         */
        bool redraw(PedigreeGraphics::Rect &rt);

        /**
         * Sets the visibility of the widget.
         * \note Be sure to redraw the parent widget after hiding a widget.
         * \param vis True if the widget is to be made visible, False otherwise.
         * \return True if successful, False otherwise.
         */
        bool visibility(bool vis);

        /**
         * Destroys the widget and all internal state.
         */
        void destroy();

        /**
         * Handles emptying the pending event queue and dispatch to callbacks
         * for an application automatically.
         */
        static void checkForEvents();

    protected:

        inline PedigreeGraphics::Framebuffer *getFramebuffer()
        {
            return m_pFramebuffer;
        }

    private:

        /** Has this widget been constructed yet? */
        bool m_bConstructed;

        /** Framebuffer for all rendering. */
        PedigreeGraphics::Framebuffer *m_pFramebuffer;

        /** Widget handle. */
        uint64_t m_Handle;

        /** Widget event handler. */
        widgetCallback_t m_EventCallback;
};

/** @} */

#endif
