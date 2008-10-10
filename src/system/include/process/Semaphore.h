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

#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include <Atomic.h>
#include <processor/types.h>
#include <Spinlock.h>
#include <utilities/List.h>

/**
 * A counting semaphore.
 */
class Semaphore
{
public:
  /** Constructor
   * \param nInitialValue The initial value of the semaphore. */
  Semaphore(size_t nInitialValue);
  /** Destructor */
  virtual ~Semaphore();

  /** Attempts to acquire n items from the semaphore. This will block until the semaphore
   *  is non-zero.
   * \param n The number of semaphore items required. Must be non-zero. */
  void acquire(size_t n=1);

  /** Attempts to acquire n items from the semaphore. This will not block.
   * \param n The number of semaphore items required. Must be non-zero.
   * \return True if acquire succeeded, false otherwise. */
  bool tryAcquire(size_t n=1);

  /** Releases n items from the semaphore.
   * \param n The number of semaphore items to release. Must be non-zero. */
  void release(size_t n=1);

//private:
  /** Private copy constructor
      \note NOT implemented. */
  Semaphore(const Semaphore&);
  /** Private operator=
      \note NOT implemented. */
  void operator =(const Semaphore&);

  Atomic<ssize_t> m_Counter;
  Spinlock m_BeingModified;
  List<class Thread*> m_Queue;
  class Thread *m_pParent;
  int magic;
  
};

#endif
