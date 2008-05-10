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

#ifndef SCHEDULING_ALGORITHM_H
#define SCHEDULING_ALGORITHM_H

class Thread;
class Processor;

/**
 * Class providing an abstraction of a long term scheduling algorithm.
 */
class SchedulingAlgorithm
{
public:
  /** Destructor */
  virtual ~SchedulingAlgorithm() {}

  /** Adds a new thread to be scheduled. */
  virtual void addThread(Thread *pThread) =0;

  /** Removes a thread - this thread should no longer be scheduled. */
  virtual void removeThread(Thread *pThread) =0;

  /** Return the next thread that should be scheduled for the given Processor.
   * \param pProcessor The Processor for which the scheduling should take place - this is
   *                   provided for heuristic purposes (core affinity etc).
   * \note It is assumed that this function will set the Thread's TTL and other such values
   *       itself. */
  virtual Thread *getNext(Processor *pProcessor) =0;
  
  /** Notifies us that the status of a thread has changed, and that we may need to take action. */
  virtual void threadStatusChanged(Thread *pThread) =0;
};

#endif
