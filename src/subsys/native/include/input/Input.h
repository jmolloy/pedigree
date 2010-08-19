/*
 * Copyright (c) 2010 Matthew Iselin
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
#ifndef _INPUT_MANAGER_H
#define _INPUT_MANAGER_H

#include <types.h>

namespace Input
{
    /// The type for a given callback.
    enum CallbackType
    {
        Key = 0,
        Mouse = 1,
        Joystick = 2,
        Unknown = 255
    };

    /// Structure containing notification to the remote application
    /// of input. Used to generalise input handling across the system
    /// for all types of devices.
    struct InputNotification
    {
        CallbackType type;

        union
        {
            struct
            {
                uint64_t key;
            } key;
            struct
            {
                ssize_t relx;
                ssize_t rely;
                ssize_t relz;

                bool buttons[64];
            } pointy;
        } data;
    };

    /// Callback function type
    typedef void (*callback_t)(InputNotification &);

    /// Installs an input callback, to allow a program to be notified of
    /// input from any of the possible input devices.
    void installCallback(callback_t cb);

    /// Removes a given callback
    void removeCallback(callback_t cb);

    /// Loads a new keymap and sets it as the system-wide keymap from the
    /// given file. There is currently no supported way to obtain mappings
    /// for keys from the kernel.
    void loadKeymapFromFile(const char *path);
};

#endif
