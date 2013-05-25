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
#include <Spinlock.h>
#include <processor/Processor.h>

#ifdef TRACK_LOCKS
#include <LocksCommand.h>
#endif

#include <Log.h>

#include <panic.h>

void Spinlock::acquire()
{
  // Save the current irq status.

  // This save to local variable prevents a heinous race condition where the thread is
  // preempted between the getInterrupts and setInterrupts, then this same spinlock is called
  // in the new thread with interrupts disabled. It gets back to us, and m_bInterrupts==false.
  // Oh dear, hanging time.
  //
  // We write to a local so the interrupt value is saved onto the stack until interrupts are
  // definately disabled; then we can write it back to the member variable.
  bool bInterrupts = Processor::getInterrupts();

  // Disable irqs if not already done
  if (bInterrupts)
    Processor::setInterrupts(false);

  if (m_Magic != 0xdeadbaba)
  {
      FATAL_NOLOCK("Wrong magic in acquire [" << m_Magic << "] [this=" << ((uintptr_t) this) << "]");
  }

  while (m_Atom.compareAndSwap(true, false) == false)
  {
#ifndef MULTIPROCESSOR
    /// \note When we hit this breakpoint, we're not able to backtrace as backtracing
    ///       depends on the log spinlock, which may have deadlocked. So we actually
    ///       force the spinlock to release here, then hit the breakpoint.
      size_t atom = m_Atom;
      m_Atom = true;

    FATAL_NOLOCK("Spinlock has deadlocked, return address of other locker is " << m_Ra << ", spinlock is " << reinterpret_cast<uintptr_t>(this) << ", atom is " << atom << ".");

    // Panic in case there's a return from the debugger (or the debugger isn't available)
    panic("Spinlock has deadlocked");
#endif
  }
  m_Ra = reinterpret_cast<uintptr_t>(__builtin_return_address(0));

#ifdef TRACK_LOCKS
//  if (!m_bAvoidTracking)
      g_LocksCommand.lockAcquired(this);
#endif

  m_bInterrupts = bInterrupts;

}
void Spinlock::release()
{
  bool bWasInterrupts = Processor::getInterrupts();
  if (bWasInterrupts == true)
  {
      FATAL_NOLOCK("Spinlock: release() called with interrupts enabled.");
  }
  bool bInterrupts = m_bInterrupts;

  if (m_Magic != 0xdeadbaba)
  {
      FATAL_NOLOCK("Wrong magic in release.");
  }

  if (m_Atom.compareAndSwap(false, true) == false)
  {
    /// \note When we hit this breakpoint, we're not able to backtrace as backtracing
    ///       depends on the log spinlock, which may have deadlocked. So we actually
    ///       force the spinlock to release here, then hit the breakpoint.
    size_t atom = m_Atom;
    m_Atom = true;

    FATAL_NOLOCK("Spinlock has deadlocked, return address of other locker is " << m_Ra << ", spinlock is " << reinterpret_cast<uintptr_t>(this) << ", atom is " << atom << ".");

    // Panic in case there's a return from the debugger (or the debugger isn't available)
    panic("Spinlock has deadlocked");
  }

#ifdef TRACK_LOCKS
//  if (!m_bAvoidTracking)
    g_LocksCommand.lockReleased(this);
#endif

  // Reenable irqs if they were enabled before
  if (bInterrupts)
  {
    Processor::setInterrupts(true);
  }
}
