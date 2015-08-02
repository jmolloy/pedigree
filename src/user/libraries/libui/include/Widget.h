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

#ifndef _WIDGET_H
#define _WIDGET_H

#ifdef TARGET_LINUX
#include <stdint.h>
#include <unistd.h>

#include <util.h>
#else
#include <types.h>
#include <ipc/Ipc.h>
#endif

#include <graphics/Graphics.h>

#include <string>
#include <map>
#include <queue>

/** \addtogroup PedigreeGUI
 *  @{
 */

enum WidgetMessages
{
    RepaintNeeded,
    Reposition,
    MouseMove,
    MouseDown,
    MouseUp,
    KeyDown,
    KeyUp,
    RawKeyDown,
    RawKeyUp,
    Focus,
    NoFocus,
    Terminate,
};

typedef bool (*widgetCallback_t)(WidgetMessages, size_t, const void *);

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
         * \note (PID << 32) | ((WidgetPointer >> 32) | (WidgetPointer & 0xFFFFFFFF))
         * If the system is not capable of 64-bit addressing, the 32-bit right
         * shift is not necessary.
         * The PID is assumed to never exceed the limits of a 32-bit unsigned
         * integer. Should this happen, the PID should be folded in the same way
         * as the widget pointer.
         */
        inline uint64_t getHandle() const
        {
            return m_Handle;
        }

        /**
         * Constructs the internal state of the widget. This involves talking to
         * the window manager via IPC to obtain a framebuffer for rendering, as
         * well as notifying the window manager that this widget now exists.
         * No other calls are valid until this call has returned successfully.
         * \param endpoint Unique IPC endpoint name to listen on for this widget
         * \param cb Event handler callback.
         * \param dimensions Dimensions of the widget to be created.
         * \note The window manager may elect to override the requested dimensions
         *       for the purpose of rendering. Handling a Reposition event in the
         *       event handler callback will reveal the assigned dimensions.
         * \return True if the widget was successfully constructed, False otherwise.
         */
        bool construct(const char *endpoint, const char *title, widgetCallback_t cb, PedigreeGraphics::Rect &dimensions);

        /**
         * Sets the title of the widget.
         * \param title New title for the widget.
         * \return True is the new title was set, False otherwise.
         */
        bool setTitle(const std::string &newTitle);

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
         * Gets the raw framebuffer for the window.
         * This will be in ARGB32 format.
         */
        void *getRawFramebuffer()
        {
            return m_SharedFramebuffer ? m_SharedFramebuffer->getBuffer() : 0;
        }

        /**
         * Gets a raw socket that can be used for select() or poll().
         *
         * An application can take all widgets it is aware of and select() on them;
         * when they are readable, a call to Widget::checkForEvents will clear
         * the queued events that arrived.
         */
        int getSocket() const
        {
            return m_Socket;
        }

        /**
         * Handles emptying the pending event queue and dispatch to callbacks
         * for an application automatically.
         */
        static void checkForEvents(bool bAsync = false);

    private:

        /**
         * Waits for a message of the given type, queuing other messages along
         * the way.
         */
        static void checkForMessage(size_t which, bool bResponse=false);

        /** Handle the given message. */
        static void handleMessage(const char *pMessageBuffer);

        /** Clear out pending messages. */
        static bool handlePendingMessages();

        /** Get the Widget's callback. */
        widgetCallback_t getCallback() const
        {
            return m_EventCallback;
        }

        /** Has this widget been constructed yet? */
        bool m_bConstructed;

        /** Framebuffer for all rendering. */
        PedigreeGraphics::Framebuffer *m_pFramebuffer;

        /** Widget handle. */
        uint64_t m_Handle;

        /** Widget event handler. */
        widgetCallback_t m_EventCallback;

        /** IPC socket for window manager communication (supercedes IPC enpdoint). */
        int m_Socket;

#ifdef TARGET_LINUX
        /** Shared memory region on Linux, using shmem. */
        SharedBuffer *m_SharedFramebuffer;
#else
        /** IPC shared message that handles our framebuffer. */
        PedigreeIpc::SharedIpcMessage *m_SharedFramebuffer;
#endif

        /** Handle -> Callback mapping. For event handler. */
        static std::map<uint64_t, Widget *> s_KnownWidgets;

        /** Queue of messages that have not yet been delivered to a callback. */
        static std::queue<char *> s_PendingMessages;

        /** # of Widgets currently active. */
        static size_t s_NumWidgets;
};

/** @} */

#endif
