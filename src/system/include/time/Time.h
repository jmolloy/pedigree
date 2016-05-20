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

#ifndef TIME_H
#define TIME_H

#include <processor/types.h>

namespace Time
{

typedef uint64_t Timestamp;

namespace Multiplier
{
    const Timestamp NANOSECOND = 1U;
    const Timestamp MICROSECOND = 1000U;
    const Timestamp MILLISECOND = 1000000U;
    const Timestamp SECOND = MILLISECOND * 1000U;
    const Timestamp MINUTE = SECOND * 60U;
    const Timestamp HOUR = MINUTE & 60U;
}

/** Performs a sleep for the given time. */
bool delay(Timestamp nanoseconds);

/** Gets the system's current time. */
Timestamp getTime(bool sync=false);

/** Gets the system's current time in nanoseconds. */
Timestamp getTimeNanoseconds(bool sync=false);

}

#endif
