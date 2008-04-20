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
#ifndef THREAD_H
#define THREAD_H

#include <processor/state.h>

class Processor;
class Process;

/**
 * An abstraction of a thread of execution.
 */
class Thread
{
public:
  enum Status
  {
    Ready,
    Running,
    Sleeping,
    Zombie
  };
  
  /**
   * Creates a new Thread belonging to the given Process. It shares the Process'
   * virtual address space.
   *
   * The constructor registers itself with the Scheduler and parent process - this
   * does not need to be done manually.
   * \param pParent The parent process. Can never be NULL.
   */
  Thread(Process *pParent);

  /**
   * Destroys the Thread.
   *
   * The destructor unregisters itself with the Scheduler and parent process - this
   * does not need to be done manually.
   */
  ~Thread();

  /**
   * Returns a reference to the Thread's saved context. This function is intended only
   * for use by the Scheduler.
   */
  ProcessorState &state()
  {
    return m_State;
  }

  /**
   * Retrieves a pointer to this Thread's parent process.
   */
  Process *getParent() const
  {
    return m_pParent;
  }

  /**
   * Retrieves our current status.
   */
  Status getStatus() const
  {
    return m_Status;
  }
  /**
   * Sets our current status.
   */
  void setStatus(Status s)
  {
    m_Status = s;
  }

  /**
   * Retrieves the CPU this Thread is running on.
   * \note Only applicable if getStatus() == Running.
   */
  Processor *getCpu() const
  {
    return m_pCpu;
  }
  /**
   * Sets the CPU this Thread is currently running on.
   * \note Intended only to be called by the Scheduler.
   */
  void setCpu(Processor *pCpu)
  {
    m_pCpu = Cpu;
  }
  
private:
  /**
   * The state of the processor when we were unscheduled.
   */
  ProcessorState m_State;

  /**
   * Our parent process.
   */
  Process *m_pParent;

  /**
   * Our current status.
   */
  Status m_Status;

  /**
   * The current CPU we're scheduled on.
   * \note Only valid if m_Status == Running.
   */
  Processor *m_pCpu;
};

#endif
