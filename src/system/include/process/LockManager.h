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

#ifndef LOCK_MANAGER_H
#define LOCK_MANAGER_H

#include <process/Semaphore.h>
#include <utilities/Vector.h>

/**
 * A class for managing locks. It is only used if ENFORCE_LOCK_ORDERING is defined,
 * and expects Semaphores to notify it of acquisition and release. If a semaphore is
 * released before one which was acquired after it, an assertion fires.
 *
 * It is expected to have one LockManager per processor.
 */
class LockManager
{
public:
  /** Constructor */
  LockManager();
  /** Destructor */
  ~LockManager();

  /** Called by Semaphore on successful acquisition. */
  void acquired(Semaphore &sem);

  /** Called by Semaphore on successful release. */
  void released(Semaphore &sem);

private:
  /** The stack of acquired semaphores. */
  Vector<Semaphore*> m_Stack;
};

#endif
