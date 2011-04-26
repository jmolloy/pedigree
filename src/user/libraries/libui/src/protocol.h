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
#ifndef LIBUI_PROTOCOL_H
#define LIBUI_PROTOCOL_H

#include <types.h>

#include <Widget.h>
#include <graphics/Graphics.h>

/** \addtogroup PedigreeGUI
 *  @{
 */

namespace LibUiProtocol
{
    /** Sends a message to the window manager. */
    bool sendMessage(void *pMessage, size_t messageLength);

    /** Receives a message from the window manager. Blocks. */
    bool recvMessage(void *pBuffer, size_t maxSize);

    /** Widget handle type. */
    typedef uint64_t handle_t;

    enum ButtonState
    {
        Down,
        Up,
        Unchanged
    };

    enum MessageIdentifiers
    {
        /**
         * LargeBufferPrepare: no direction. Begins a handshake for creation of
         * a buffer > 4 KB in size for communication. Most messages should fit
         * into a smaller buffer.
         */
        LargeBufferPrepare,

        /**
         * Create: widget -> window manager. Requests creation of a handle and
         * framebuffer.
         */
        Create,

        /**
         * SetProperty: widget -> window manager. Requests modification of a window
         * manager property for the widget.
         */
        SetProperty,

        /**
         * GetProperty: widget -> window manager. Requests the value of a window
         * manager property for the widget.
         */
        GetProperty,

        /**
         * SetParent: widget -> window manager. Requests a move of the widget's tree
         * of children (and itself) to a different parent.
         */
        SetParent,

        /**
         * GetParent: widget -> window manager. Requests the parent of the given
         * widget.
         */
        GetParent,

        /**
         * RequestRedraw: widget -> window manager. Requests a redraw of the given
         * area. Causes a redraw to propagate if the widget is visible and the area
         * is not obstructed by another widget.
         */
        RequestRedraw,

        /**
         * SetVisibility: widget -> window manager. Requests a change in the visibility
         * of the given widget.
         */
        SetVisibility,

        /**
         * Destroy: widget -> window manager. Requests removal of the widget from
         * the window manager. Invalidates the widget handle and also destroys it's
         * framebuffer.
         */
        Destroy,

        /**
         * MouseEvent: window manager -> widget. Notifies a widget of a mouse event.
         */
        MouseEvent,

        /**
         * KeyboardEvent: window manager -> widget. Notifies a widget of a keyboard
         * event.
         */
        KeyEvent
    };

    /** Applied as the header on a request or notification sent via IPC. */
    struct WindowManagerMessage
    {
        /// Code of the message being sent.
        MessageIdentifiers messageCode;

        /// Handle for the widget being referred to. Zero if no widget.
        handle_t widgetHandle;

        /// Size of the data in the message (after this header).
        size_t messageSize;
        
        /// Whether this message is a response from the window manager or not.
        bool isResponse;
    };

    /** Large buffer preparation message data. */
    struct LargeBufferPrepareMessage
    {
        /// Whether this is a response or a request.
        bool bIsResponse;

        /// Address off the large buffer. Null in a request.
        uintptr_t bufferAddress;

        /// Length of the buffer (in & out).
        size_t bufferLength;
    };

    /** Create message data. */
    struct CreateMessage
    {
        size_t x;
        size_t y;
        size_t width;
        size_t height;
    };

    /** Create message response data. */
    struct CreateMessageResponse
    {
        PedigreeGraphics::GraphicsProvider provider;
    };

    /**
     * SetPropety message data. Value data is copied directly after this struct
     * in the message to be sent.
     */
    struct SetPropertyMessage
    {
        char propertyName[256];
        size_t valueLength;
    };

    /** GetProperty message data. */
    struct GetPropertyMessage
    {
        char propertyName[256];
        size_t maxValueLength;
    };

    /** SetParent message data. */
    struct SetParentMessage
    {
        handle_t newParent;
    };

    /** GetParent message data (only used in the response message). */
    struct GetParentMessage
    {
        handle_t parent;
    };

    /** RequestRedraw message data. */
    struct RequestRedrawMessage
    {
        size_t x;
        size_t y;
        size_t width;
        size_t height;
    };

    /** SetVisibility message data. */
    struct SetVisibilityMessage
    {
        bool bVisible;
    };

    /** Destroy message data (none as yet). */
    struct DestroyMessage
    {
    };

    /** MouseEvent message data. */
    struct MouseEventMessage
    {
        size_t x;
        size_t y;

        /// Whether or not the mouse has moved. Used to determine whether to
        /// create a MouseMove event or just a MouseDown/Up event.
        bool bMouseMove;

        ButtonState buttons[8];
    };

    /** KeyEvent message data. */
    struct KeyEventMessage
    {
        ButtonState state;
        char key;
    };
};

/** @} */

#endif
