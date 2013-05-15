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

#ifndef UNLIKELY_LOCK_H
#define UNLIKELY_LOCK_H

#include <processor/Processor.h>
#include <process/Scheduler.h>
#include <Atomic.h>

/** \file UnlikelyLock.h
    \author James Molloy
    \date 16 September 2009 */

/** An "Unlikely lock" is a lock which normally any number of threads can access
    concurrently, but when locked, all threads must exit and never reenter.

    This is implemented as a simple counting semaphore that starts at zero. Any
    thread can acquire by adding one to the semaphore. If a thread discovers that the
    semaphore was over a threshold (100000), it decrements again and returns false.

    Thus, the lock can be acquired by adding 100000 to the atomic member, and waiting until
    the atomic member's value drops to exactly 100000, at which point all threads will have
    exited.
*/
class UnlikelyLock
{
public:
    UnlikelyLock();
    ~UnlikelyLock();

    /** Enters the critical section.
        \return True if the lock was able to be acquired, false otherwise. */
    inline bool enter()
    {
        // If we manage to enter the critical section then reenter and try and
        // acquire, we'll deadlock. Not good. Disable reentrancy by disabling
        // interrupts to guarantee liveness.
        if (m_Atomic >= 100000)
            return false;

        m_Atomic += 1;

        // If the value was over the treshold - i.e. we're acquired, decrement 
        // again and return.
        if (m_Atomic >= 100000)
        {
            m_Atomic -= 1;
            return false;
        }
        
        return true;
    }
    /** Leaving the critical section. */
    inline void leave()
    {
        m_Atomic -= 1;
    }

    /** Locks the lock. Will not return until all other threads have exited
        the critical region. */
    inline bool acquire()
    {
        bool bOldInterrupts = Processor::getInterrupts();
        if(bOldInterrupts)
            Processor::setInterrupts(false);

        m_Atomic += 100000;

        if (m_Atomic >= 200000)
        {
            // Someone already acquire()d... :(
            m_Atomic -= 100000;
            Processor::setInterrupts(bOldInterrupts);
            return false;
        }

        while (m_Atomic > 100000)
        {
            Scheduler::instance().yield();
        }
        
        m_bInterrupts = bOldInterrupts;

        return true;
    }
    /** Releases the lock. */
    inline void release()
    {
        // Reset counter to one.
        m_Atomic -= 100000;
        Processor::setInterrupts(m_bInterrupts);
    }

private:
    Atomic<size_t> m_Atomic;
    
    bool m_bInterrupts;
};

#endif
