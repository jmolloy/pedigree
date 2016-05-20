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

#include <network/NetworkBlockTimeout.h>


NetworkBlockTimeout::NetworkBlockTimeout() :
    m_Nanoseconds(0), m_Seconds(0), m_Timeout(30), m_Sem(0), m_TimedOut(0)
{
}

void NetworkBlockTimeout::setTimeout(uint32_t seconds)
{
  m_Timeout = seconds;
}

void NetworkBlockTimeout::setSemaphore(Semaphore* sem)
{
  m_Sem = sem;
}

void NetworkBlockTimeout::setTimedOut(bool* b)
{
  m_TimedOut = b;
}

void NetworkBlockTimeout::timer(uint64_t delta, InterruptState &state)
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
