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

#define TRACK_LOCKS

#ifdef TRACK_LOCKS
#include <LocksCommand.h>
#endif

#include <Log.h>

#include <panic.h>

#include <machine/Machine.h>
#include <utilities/StaticString.h>

#define COM1 0x3F8
#define COM2 0x2F8

#define CPU_LOG(s)  do { \
  uint16_t x = COM1; \
  if (Processor::id()) \
    x = COM2; \
  const char *s_ = s; \
  for (size_t i = 0; i < str.length(); ++i) \
    __asm__ __volatile__("outb %1, %0" :: "Nd" (x), "a" (s_[i])); \
} while(0)

static Atomic<size_t> x(0);

bool Spinlock::acquire(bool recurse, bool safe)
{
  Thread *pThread = Processor::information().getCurrentThread();

  /// \todo add a bitmap of CPUs that allows us to figure out if we've managed to get all CPUs

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
      FATAL_NOLOCK("Wrong magic in acquire [" << m_Magic << "] [this=" << reinterpret_cast<uintptr_t>(this) << "]");
  }

#ifdef TRACK_LOCKS
//  if (!m_bAvoidTracking)
      g_LocksCommand.lockAttempted(this);
#endif

  bool bPrintedSpin = false;

  HugeStaticString str;
  str << "Spinlock: acquire [";
  str.append(reinterpret_cast<uintptr_t>(this), 16);
  str << ", ";
  str.append(reinterpret_cast<uintptr_t>(pThread), 16);
  str << ", " << Processor::id() << "]";
  str << " -> ";
  str.append(reinterpret_cast<uintptr_t>(__builtin_return_address(0)), 16);
  str << " ";
  str.append(x += 1);
  str << "\n";
  CPU_LOG(str);
  while (m_Atom.compareAndSwap(true, false) == false)
  {
    if (!bPrintedSpin)
    {
      str.clear();
      str << "  -> A spin ";
      str << Processor::id();
      str << "\n";
      CPU_LOG(str);

      bPrintedSpin = true;
    }
    // Couldn't take the lock - can we re-enter the critical section?
    if (m_bOwned && (m_pOwner == pThread) && recurse)
    {
      // Yes.
      ++m_Level;
      break;
    }

#ifdef TRACK_LOCKS
//  if (!m_bAvoidTracking)
      if (!g_LocksCommand.checkState(this))
      {
        FATAL_NOLOCK("Spinlock: LocksCommand disallows this acquire.");
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
    ERROR_NOLOCK("T: " << pThread << " / " << m_pOwner);
    ERROR_NOLOCK("Spinlock has deadlocked, level is " << m_Level);
    ERROR_NOLOCK("Spinlock has deadlocked, my return address is " << myra << ", return address of other locker is " << m_Ra);
    FATAL_NOLOCK("Spinlock has deadlocked, spinlock is " << reinterpret_cast<uintptr_t>(this) << ", atom is " << atom << ".");

    // Panic in case there's a return from the debugger (or the debugger isn't available)
    panic("Spinlock has deadlocked");
  }
  m_Ra = reinterpret_cast<uintptr_t>(__builtin_return_address(0));

#ifdef TRACK_LOCKS
//  if (!m_bAvoidTracking)
      g_LocksCommand.lockAcquired(this);
#endif

  if (recurse && !m_bOwned)
  {
    m_pOwner = static_cast<void *>(pThread);
    m_bOwned = true;
    m_Level = 1;
  }

  m_bInterrupts = bInterrupts;
  m_OwnedProcessor = Processor::id();
    m_pOwner = static_cast<void *>(pThread);

  return true;

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
      return;
    }
  }

  m_pOwner = 0;
  m_bOwned = false;
  m_OwnedProcessor = ~0;

  HugeStaticString str;
  str << "Spinlock: release [";
  str.append(reinterpret_cast<uintptr_t>(this), 16);
  str << ", ";
  str.append(reinterpret_cast<uintptr_t>(pThread), 16);
  str << ", " << Processor::id() << "]";
  str << "\n";
  CPU_LOG(str);
  if (m_Atom.compareAndSwap(false, true) == false)
  {
    /// \note When we hit this breakpoint, we're not able to backtrace as backtracing
    ///       depends on the log spinlock, which may have deadlocked. So we actually
    ///       force the spinlock to release here, then hit the breakpoint.
    size_t atom = m_Atom;
    m_Atom = true;

    uintptr_t myra = reinterpret_cast<uintptr_t>(__builtin_return_address(0));
    FATAL_NOLOCK("Spinlock has deadlocked in release, my return address is " << myra << ", return address of other locker is " << m_Ra << ", spinlock is " << reinterpret_cast<uintptr_t>(this) << ", atom is " << atom << ".");

    // Panic in case there's a return from the debugger (or the debugger isn't available)
    panic("Spinlock has deadlocked");
  }

#ifdef TRACK_LOCKS
//  if (!m_bAvoidTracking)
    g_LocksCommand.lockReleased(this);
#endif
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
  m_Ra = 0;
}
