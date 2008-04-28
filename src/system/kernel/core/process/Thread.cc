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

#include <process/Thread.h>
#include <process/Scheduler.h>
#include <processor/Processor.h>
#include <processor/StackFrame.h>
#include <machine/Machine.h>
#include <Log.h>

/**
 * The trampoline to start a new thread. It calls pFunc(pParam), then calls pThread->threadExited
 * with the return value.
 */
static void threadStartTrampoline(Thread *pThread, void *pParam, Thread::ThreadStartFunc pFunc)
{
  Scheduler::instance().m_Mutex.release();
  /// \todo What IRQ do we ACK? It's machine specific.
  Machine::instance().getIrqManager()->acknowledgeIrq(0x20);
  Processor::setInterrupts(true);
  
  int code = pFunc(pParam);
  pThread->threadExited(code);
  // Should never get here.
  FATAL("threadStartTrampoline: Got past threadExited!");
}

Thread::Thread(Process *pParent, ThreadStartFunc pStartFunction, void *pParam, 
               uintptr_t *pStack) :
    m_State(), m_pParent(pParent), m_Status(Ready), m_ExitCode(0), m_pKernelStack(0)
{
  // Initialise our kernel stack.
  uintptr_t *pKernelStackBottom = new uintptr_t[KERNEL_STACK_SIZE/sizeof(uintptr_t)];
  m_pKernelStack = pKernelStackBottom+(KERNEL_STACK_SIZE/sizeof(uintptr_t))-1;
  
  // If we've been given a user stack pointer, we use that, else we use our kernel stack.
  if (pStack == 0)
    pStack = m_pKernelStack;

  // Start initialising our ProcessorState.
  m_State.setStackPointer (reinterpret_cast<processor_register_t> (pStack));
  m_State.setInstructionPointer (reinterpret_cast<processor_register_t>
                                   (&threadStartTrampoline));
  
  // Construct a stack frame in our ProcessorState for the call to threadStartTrampoline.
  /// \todo Is the implicit conversion done on the this, pParam and pStack parameters acceptable?
  StackFrame::construct (m_State, // Store the frame in this state.
                         0,       // We don't care about a return address.
                         3,       // There are three parameters.
                         this,    // First parameter.
                         pParam,  // Second parameter.
                         pStartFunction); // Third parameter.

  // TODO Register ourselves with the given Process.
  
  // Now we are ready to go into the scheduler.
  Scheduler::instance().addThread(this);
}

Thread::Thread(Process *pParent) :
    m_State(), m_pParent(pParent), m_Status(Running), m_ExitCode(0), m_pKernelStack(0)
{
  // TODO Register ourselves with the given Process.
}

Thread::~Thread()
{
  // Remove us from the scheduler.
  Scheduler::instance().removeThread(this);

  // TODO delete any pointer data.
  if (m_pKernelStack)
  {
    uintptr_t *pKernelStackBottom = m_pKernelStack-(KERNEL_STACK_SIZE/sizeof(uintptr_t))+1;
    delete [] pKernelStackBottom;
  }
}

void Thread::threadExited(int code)
{
  NOTICE("Thread exited with code " << Dec << code);
  m_Status = Zombie;
  delete this;
  Processor::halt();
}
