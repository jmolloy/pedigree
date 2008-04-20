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

#include <Semaphore.h>

// Note, this is in its own file purely so that a vtable can be generated.
Semaphore::Semaphore(size_t nInitialValue)
{
}

Semaphore::~Semaphore()
{
}

void Semaphore::acquire(size_t n)
{
//   while (m_Counter == 0);
//   m_Counter -= n;
//   while (m_Counter == 0);
#ifdef STRICT_LOCK_ORDERING
//     Processor::.acquired(*this);
#endif
}

bool Semaphore::tryAcquire(size_t n)
{
//   if (m_Counter > 0 && m_Counter.tryDecrement(n))
//   {
#ifdef STRICT_LOCK_ORDERING
//     LockManager::instance().acquired(*this);
#endif
    return true;
//   }
//   return false;
}

void Semaphore::release(size_t n)
{
//   m_Counter += n;
#ifdef STRICT_LOCK_ORDERING
#endif
}
