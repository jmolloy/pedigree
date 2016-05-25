/*
 * Copyright (c) 2008-2014, Pedigree Developers
 *
 * Please see the CONTRIB file in the root of the source tree for a full
 * list of contributors.
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
#include <process/Thread.h>

#ifdef TRACK_LOCKS
#include <LocksCommand.h>
#endif

#include <Log.h>

#include <panic.h>

static Atomic<size_t> x(0);

bool Spinlock::acquire(bool recurse, bool safe)
{
  Thread *pThread = Processor::information().getCurrentThread();

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
      FATAL_NOLOCK("Wrong magic in acquire [" << Hex << m_Magic << "] [this=" << reinterpret_cast<uintptr_t>(this) << "]");
  }

#ifdef TRACK_LOCKS
  if (!m_bAvoidTracking)
  {
    g_LocksCommand.clearFatal();
    if (!g_LocksCommand.lockAttempted(this, Processor::id(), bInterrupts))
    {
      uintptr_t myra = reinterpret_cast<uintptr_t>(__builtin_return_address(0));
      FATAL_NOLOCK("Spinlock: LocksCommand disallows this acquire [return=" << Hex << myra << "].");
    }
    g_LocksCommand.setFatal();
  }
#endif

  while (m_Atom.compareAndSwap(true, false) == false)
  {
    // Couldn't take the lock - can we re-enter the critical section?
    if (m_bOwned && (m_pOwner == pThread) && recurse)
    {
      // Yes.
      ++m_Level;
      break;
    }

    Processor::pause();

#ifdef TRACK_LOCKS
    if (!m_bAvoidTracking)
    {
      g_LocksCommand.clearFatal();
      if (!g_LocksCommand.checkState(this))
      {
        uintptr_t myra = reinterpret_cast<uintptr_t>(__builtin_return_address(0));
        FATAL_NOLOCK("Spinlock: LocksCommand failed a state check [return=" << Hex << myra << "].");
      }
      g_LocksCommand.setFatal();
    }
#endif

#ifdef MULTIPROCESSOR
    if (Processor::getCount() > 1)
    {
      if (safe)
      {
        // If the other locker is in fact this CPU, we're trying to re-enter
        // and that won't work at all.
        if (Processor::id() != m_OwnedProcessor)
        {
          // OK, the other CPU could still release the lock.
          continue;
        }
      }
      else
      {
        // Unsafe mode, so we don't detect obvious re-entry.
        continue;
      }
    }
#endif

    /// \note When we hit this breakpoint, we're not able to backtrace as backtracing
    ///       depends on the log spinlock, which may have deadlocked. So we actually
    ///       force the spinlock to release here, then hit the breakpoint.
      size_t atom = m_Atom;
      m_Atom = true;

    uintptr_t myra = reinterpret_cast<uintptr_t>(__builtin_return_address(0));
    ERROR_NOLOCK("Spinlock has deadlocked in acquire");
    ERROR_NOLOCK(" -> level is " << m_Level);
    ERROR_NOLOCK(" -> my return address is " << Hex << myra);
    ERROR_NOLOCK(" -> return address of other locker is " << m_Ra);
    FATAL_NOLOCK("Spinlock has deadlocked, spinlock is " << Hex << reinterpret_cast<uintptr_t>(this) << ", atom is " << atom << ".");

    // Panic in case there's a return from the debugger (or the debugger isn't available)
    panic("Spinlock has deadlocked");
  }
  m_Ra = reinterpret_cast<uintptr_t>(__builtin_return_address(0));

#ifdef TRACK_LOCKS
  if (!m_bAvoidTracking)
  {
    g_LocksCommand.clearFatal();
    if (!g_LocksCommand.lockAcquired(this, Processor::id(), bInterrupts))
    {
      uintptr_t myra = reinterpret_cast<uintptr_t>(__builtin_return_address(0));
      FATAL_NOLOCK("Spinlock: LocksCommand disallows this acquire [return=" << Hex << myra << "].");
    }
    g_LocksCommand.setFatal();
  }
#endif

  if (recurse && !m_bOwned)
  {
    m_pOwner = static_cast<void *>(pThread);
    m_bOwned = true;
    m_Level = 1;
  }

  m_bInterrupts = bInterrupts;
  m_OwnedProcessor = Processor::id();

  return true;

}

void Spinlock::trackRelease() const
{
#ifdef TRACK_LOCKS
  if (!m_bAvoidTracking)
  {
    g_LocksCommand.clearFatal();
    if (!g_LocksCommand.lockReleased(this))
    {
      uintptr_t myra = reinterpret_cast<uintptr_t>(__builtin_return_address(0));
      FATAL_NOLOCK("Spinlock: LocksCommand disallows this release [return=" << Hex << myra << "].");
    }
    g_LocksCommand.setFatal();
  }
#endif
}

void Spinlock::exit()
{
  Thread *pThread = Processor::information().getCurrentThread();

  bool bWasInterrupts = Processor::getInterrupts();
  if (bWasInterrupts == true)
  {
      FATAL_NOLOCK("Spinlock: release() called with interrupts enabled.");
  }

  if (m_Magic != 0xdeadbaba)
  {
      FATAL_NOLOCK("Wrong magic in release.");
  }

  // Don't actually release the lock if we re-entered the critical section.
  if (m_Level)
  {
    if (--m_Level)
    {
      // Recursive acquire() still tracks, so we still need to track its
      // release or else we'll run into false positive "out-of-order release"s.
      trackRelease();
      return;
    }
  }

  m_pOwner = 0;
  m_bOwned = false;
  m_OwnedProcessor = ~0;

  // Track the release just before we actually release the lock to avoid an
  // immediate reschedule screwing with the tracking.
  trackRelease();

  if (m_Atom.compareAndSwap(false, true) == false)
  {
    /// \note When we hit this breakpoint, we're not able to backtrace as backtracing
    ///       depends on the log spinlock, which may have deadlocked. So we actually
    ///       force the spinlock to release here, then hit the breakpoint.
    size_t atom = m_Atom;
    m_Atom = true;

    uintptr_t myra = reinterpret_cast<uintptr_t>(__builtin_return_address(0));
    FATAL_NOLOCK("Spinlock has deadlocked in release, my return address is " << Hex << myra << ", return address of other locker is " << m_Ra << ", spinlock is " << reinterpret_cast<uintptr_t>(this) << ", atom is " << Dec << atom << ".");

    // Panic in case there's a return from the debugger (or the debugger isn't available)
    panic("Spinlock has deadlocked");
  }
}

void Spinlock::release()
{
  bool bInterrupts = m_bInterrupts;

  exit();

  // Reenable irqs if they were enabled before
  if (bInterrupts)
  {
    Processor::setInterrupts(true);
  }
}

void Spinlock::unwind()
{
  // We're about to be forcefully unlocked, so we must unwind entirely.
  m_Level = 0;
  m_bOwned = false;
  m_pOwner = 0;
  m_OwnedProcessor = ~0;
}
