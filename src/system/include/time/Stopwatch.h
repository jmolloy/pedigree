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

#ifndef STOPWATCH_H
#define STOPWATCH_H

#include <time/Time.h>

namespace Time
{

/**
 * Stopwatch provides a timer that can be stopped, started, and reset.
 *
 * The value only ticks while the timer is running (it does not tick up during
 * stop). The value is only current while the stopwatch is stopped.
 */
class Stopwatch
{
public:
    Stopwatch(bool startRunning = false);
    virtual ~Stopwatch();

    /// Start the stopwatch if it is not already started.
    void start();

    /// Stop the stopwatch if it is not already stopped.
    void stop();

    /// Reset the stopwatch to zero.
    void reset();

    /// Read the stopwatch value in nanoseconds.
    Timestamp value();

private:
    /// Current value.
    Timestamp m_Value;

    /// Start time.
    Timestamp m_StartValue;

    /// Running or not.
    bool m_bRunning;
};

}

#endif
