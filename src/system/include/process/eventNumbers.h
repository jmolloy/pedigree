/*
 * Copyright (c) 2008 James Molloy, Jörg Pfähler, Matthew Iselin
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
#ifndef EVENT_NUMBERS_H
#define EVENT_NUMBERS_H

/** The globally-defined event numbers for each Event subclass. These must be unique so that
    identification of serialized data is possible. */
namespace EventNumbers
{
    const size_t PosixSignalStart = 0;
    // Posix signals in here.
    const size_t PosixSignalEnd   = 32;

    const size_t TimeoutGuard     = 33;
    const size_t Interrupt        = 34;
    const size_t TimedTask        = 35;
    const size_t SelectEvent      = 36;

    const size_t InputEvent       = 37;
    const size_t TuiEvent         = 38;

    const size_t PollEvent        = 39;

    const size_t UserStart        = 0xFFFF; ///< Start of user-defined events.
}

#endif
