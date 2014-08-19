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

#include <utilities/UnlikelyLock.h>

UnlikelyLock::UnlikelyLock() :
    m_Semaphore(UNLIKELY_LOCK_MAX_READERS + 1)
{
}

UnlikelyLock::~UnlikelyLock()
{
}

bool UnlikelyLock::enter()
{
    return m_Semaphore.acquire(1);
}

void UnlikelyLock::leave()
{
    m_Semaphore.release(1);
}

bool UnlikelyLock::acquire()
{
    // acquire() is defined to not return until all other threads have left
    // the critical section, so we simply loop in case the Semaphore fails to
    // acquire after blocking (eg, interrupted).
    while(!m_Semaphore.acquire(UNLIKELY_LOCK_MAX_READERS + 1))
        Scheduler::instance().yield();
}

void UnlikelyLock::release()
{
    m_Semaphore.release(UNLIKELY_LOCK_MAX_READERS + 1);
}
