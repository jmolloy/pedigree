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
#include <processor/types.h>

class Processor;
class Process;

#define KERNEL_STACK_SIZE 4096

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
  
  typedef int (*ThreadStartFunc)(void*);
  
  /**
   * Creates a new Thread belonging to the given Process. It shares the Process'
   * virtual address space.
   *
   * The constructor registers itself with the Scheduler and parent process - this
   * does not need to be done manually.
   * 
   * If kernelMode is true, and pStack is NULL, no stack space is assigned 
   * \param pParent The parent process. Can never be NULL.
   * \param kernelMode Is the thread going to be operating in kernel space only?
   * \param pStartFunction The function to be run when the thread starts.
   * \param pParam A parameter to give the startFunction.
   * \param pStack (Optional) A (user mode) stack to give the thread - applicable for user mode threads
   *               only.
   */
  Thread(Process *pParent, ThreadStartFunc pStartFunction, void *pParam, 
         uintptr_t *pStack=0);
  
  /**
   * Alternative constructor - this should be used only by initialiseMultitasking() to
   * define the first kernel thread.
   */
  Thread(Process *pParent);

  /**
   * Destroys the Thread.
   *
   * The destructor unregisters itself with the Scheduler and parent process - this
   * does not need to be done manually.
   */
  virtual ~Thread();

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
   * Retrieves the exit status of the Thread.
   * \note Valid only if the Thread is in the Zombie state.
   */
  int getExitCode()
  {
    return m_ExitCode;
  }

  /**
   * Retrieves a pointer to the top of the Thread's kernel stack.
   */
  uintptr_t *getKernelStack()
  {
    return m_pKernelStack;
  }
  
  /**
   * Sets the exit code of the Thread and sets the state to Zombie, if it is being waited on;
   * if it is not being waited on the Thread is destroyed.
   * \note This is meant to be called only by the thread trampoline - this is the only reason it
   *       is public. It should NOT be called by anyone else!
   */
  void threadExited(int code);
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
   * Our exit code
   */
  int m_ExitCode;
  
  /**
   * Our kernel stack.
   */
  uintptr_t *m_pKernelStack;
};

#endif
