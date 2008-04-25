/*
 * Copyright (c) 2008 James Molloy, James Pritchett, Jörg Pfähler, Matthew Iselin
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

#include <process/Semaphore.h>

// NOTE, this is in its own file purely so that a vtable can be generated.
Semaphore::Semaphore(size_t nInitialValue)
  : m_Counter(nInitialValue)
{
}

Semaphore::~Semaphore()
{
}

void Semaphore::acquire(size_t n)
{
  while (tryAcquire(n) == false);
}

bool Semaphore::tryAcquire(size_t n)
{
  ssize_t value = m_Counter;
  if ((value - n) < 0)return false;

  if (m_Counter.compareAndSwap(value, value - n))
  {
    #ifdef STRICT_LOCK_ORDERING
      // TODO LockManager::acquired(*this);
    #endif
    return true;
  }
  return false;
}

void Semaphore::release(size_t n)
{
  m_Counter += n;

  #ifdef STRICT_LOCK_ORDERING
    // TODO LockManager::released(*this);
  #endif
}
