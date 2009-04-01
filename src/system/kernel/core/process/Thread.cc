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

#if defined(THREADS)
#include <process/Thread.h>
#include <process/Scheduler.h>
#include <processor/Processor.h>
#include <processor/StackFrame.h>
#include <machine/Machine.h>
#include <Log.h>

Thread::Thread(Process *pParent, ThreadStartFunc pStartFunction, void *pParam, 
               void *pStack) :
  m_State(), m_pParent(pParent), m_Status(Ready), m_ExitCode(0),  m_pKernelStack(0), m_Id(0),
  m_Errno(0), m_pInterruptState(0), m_pSavedInterruptState(0), m_bUseSavedState(false)
{
  if (pParent == 0)
  {
    FATAL("Thread::Thread(): Parent process was NULL!");
  }

  // Initialise our kernel stack.
  m_pKernelStack = VirtualAddressSpace::getKernelAddressSpace().allocateStack();

  // If we've been given a user stack pointer, we are a user mode thread.
  bool bUserMode = true;
  if (pStack == 0)
  {
    bUserMode = false;
    pStack = m_pKernelStack;
    m_pKernelStack = 0; // No kernel stack if kernel mode thread - causes bug on PPC
  }
  
  // Start initialising our ProcessorState.
  m_State.setStackPointer (reinterpret_cast<processor_register_t> (pStack));
  m_State.setInstructionPointer (reinterpret_cast<processor_register_t>
                                   (pStartFunction));
  
  // Construct a stack frame in our ProcessorState for the call to the thread starting function.
  StackFrame::construct (m_State, // Store the frame in this state.
                         reinterpret_cast<uintptr_t> (&threadExited), // Return to threadExited.
                         1,       // There is one parameter.
                         pParam); // Parameter

  // Construct an interrupt state on the stack too.
  m_pInterruptState = InterruptState::construct (m_State, bUserMode);

  m_Id = m_pParent->addThread(this);
  
  // Now we are ready to go into the scheduler.
  Scheduler::instance().addThread(this);
}

Thread::Thread(Process *pParent) :
  m_State(), m_pParent(pParent), m_Status(Running), m_ExitCode(0), m_pKernelStack(0), m_Id(0),
  m_Errno(0), m_pInterruptState(0), m_pSavedInterruptState(0), m_bUseSavedState(false)
{
  if (pParent == 0)
  {
    FATAL("Thread::Thread(): Parent process was NULL!");
  }
  m_Id = m_pParent->addThread(this);

  // Initialise our kernel stack.
  // NO! No kernel stack for kernel-mode threads. On PPC, causes bug!
  //m_pKernelStack = VirtualAddressSpace::getKernelAddressSpace().allocateStack();
}

Thread::Thread(Process *pParent, ProcessorState state) :
  m_State(), m_pParent(pParent), m_Status(Ready), m_ExitCode(0),  m_pKernelStack(0), m_Id(0),
  m_Errno(0), m_pInterruptState(0), m_pSavedInterruptState(0), m_bUseSavedState(false)
{
  if (pParent == 0)
  {
    FATAL("Thread::Thread(): Parent process was NULL!");
  }

  static Spinlock spinlock;
  spinlock.acquire();

  // Initialise our kernel stack.
  m_pKernelStack = VirtualAddressSpace::getKernelAddressSpace().allocateStack();

  VirtualAddressSpace &as = Processor::information().getVirtualAddressSpace();
  Processor::switchAddressSpace(*pParent->getAddressSpace());
  
  // Construct an interrupt state on the stack.
  m_pInterruptState = InterruptState::construct (state, /* Back to user mode */ true);

  Processor::switchAddressSpace(as);

  m_Id = m_pParent->addThread(this);
  
  // Now we are ready to go into the scheduler.
  Scheduler::instance().addThread(this);

  spinlock.release();
}

Thread::~Thread()
{
  // Remove us from the scheduler.
  Scheduler::instance().removeThread(this);

  m_pParent->removeThread(this);
  
  // TODO delete any pointer data.

  if (m_pKernelStack)
    VirtualAddressSpace::getKernelAddressSpace().freeStack(m_pKernelStack);
}

void Thread::setStatus(Thread::Status s)
{
  m_Status = s;
  Scheduler::instance().threadStatusChanged(this);
}

void Thread::threadExited(int code)
{
  NOTICE("Thread exited");
  // TODO apply these to the current thread - we don't have a this pointer.
//  m_Status = Zombie;
//  delete this;
  for(;;);
  Processor::halt();
}

#endif
