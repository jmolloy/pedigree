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
#ifndef PROCESS_H
#define PROCESS_H

#include <process/Thread.h>
#include <processor/state.h>
#include <utilities/Vector.h>
#include <utilities/StaticString.h>
#include <Atomic.h>

/**
 * An abstraction of a Process - a container for one or more threads all running in
 * the same address space.
 */
class Process
{
public:
  /** Default constructor. */
  Process();
  
  /** Destructor. */
  ~Process();
  
  /** Adds a thread to this process.
   *  \return The thread ID to be assigned to the new Thread. */
  size_t addThread(Thread *pThread);
  /** Removes a thread from this process. */
  void removeThread(Thread *pThread);
  
  /** Returns the number of threads in this process. */
  size_t getNumThreads();
  /** Returns the n'th thread in thsi process. */
  Thread *getThread(size_t n);
  
  /** Returns the process ID. */
  size_t getId()
  {
    return m_Id;
  }

  /** Returns the description string of this process. */
  LargeStaticString &description()
  {
    return str;
  }

private:
  /**
   * Our list of threads.
   */
  Vector<Thread*> m_Threads;
  /**
   * The next available thread ID.
   */
  Atomic<size_t> m_NextTid;
  /**
   * Our Process ID.
   */
  size_t m_Id;
  /**
   * Our description string.
   */
  LargeStaticString str;
};

#endif
