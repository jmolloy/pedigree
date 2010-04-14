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
#ifndef NETWORK_BLOCK_TIMEOUT_H
#define NETWORK_BLOCK_TIMEOUT_H

#include <processor/types.h>

#define MACHINE_FORWARD_DECL_ONLY
#include <machine/Machine.h>
#include <machine/Timer.h>
#undef MACHINE_FORWARD_DECL_ONLY

#include <process/Semaphore.h>
#include <Log.h>

/** The Pedigree network stack - Network timeout (for anything that blocks on a semaphore) */
class NetworkBlockTimeout : public TimerHandler
{
  public:

    NetworkBlockTimeout() : m_Nanoseconds(0), m_Seconds(0), m_Timeout(30), m_Sem(0), m_TimedOut(0) {};

    inline void setTimeout(uint32_t seconds)
    {
      m_Timeout = seconds;
    }

    /** Sets the internal semaphore, which gets released if a timeout occurs */
    inline void setSemaphore(Semaphore* sem)
    {
      m_Sem = sem;
    }

    /** Sets the internal "timed out" bool pointer, so that the timeout is not mistaken
        as a data arrival Semaphore release */
    inline void setTimedOut(bool* b)
    {
      m_TimedOut = b;
    }

    virtual void timer(uint64_t delta, InterruptState &state)
    {
      if(UNLIKELY(m_Seconds < m_Timeout))
      {
        m_Nanoseconds += delta;
        if(UNLIKELY(m_Nanoseconds >= 1000000000ULL))
        {
          ++m_Seconds;
          m_Nanoseconds -= 1000000000ULL;
        }

        if(UNLIKELY(m_Seconds >= m_Timeout))
        {
          if(m_Sem)
            m_Sem->release();
          if(m_TimedOut)
            *m_TimedOut = true;
        }
      }
    }

  private:
    uint64_t m_Nanoseconds;
    uint64_t m_Seconds;
    uint32_t m_Timeout;

    Semaphore* m_Sem;
    bool* m_TimedOut;

    NetworkBlockTimeout(const NetworkBlockTimeout& s) : m_Nanoseconds(0), m_Seconds(0), m_Timeout(30), m_Sem(0), m_TimedOut(0) {};
    NetworkBlockTimeout& operator = (const NetworkBlockTimeout& s)
    {
      ERROR("NetworkBlockTimeout copy constructor has been called.");
      return *this;
    }
};

#endif

