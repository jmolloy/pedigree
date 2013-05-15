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

#ifndef KERNEL_SPINLOCK_H
#define KERNEL_SPINLOCK_H

#include <Atomic.h>

class Spinlock
{
    friend class PerProcessorScheduler;
    friend class LocksCommand;
  public:
    inline Spinlock(bool bLocked = false, bool bAvoidTracking = false)
        : m_bInterrupts(), m_Atom(!bLocked), m_Ra(0), m_bAvoidTracking(bAvoidTracking), m_Magic(0xdeadbaba) {}

    void acquire();
    void release();

    bool acquired()
    {
        return !m_Atom;
    }

    bool interrupts() const
    {
        return m_bInterrupts;
    }

  private:
    volatile bool m_bInterrupts;
    Atomic<bool> m_Atom;

    uintptr_t m_Ra;
    bool m_bAvoidTracking;
    uint32_t m_Magic;
};

#endif
