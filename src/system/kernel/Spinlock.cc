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

  while (m_Atom.compareAndSwap(true, false) == false)
  {
#ifndef MULTIPROCESSOR
    FATAL("Spinlock: already acquired on a uniprocessor system, interrupts=" << Processor::getInterrupts());
    Processor::breakpoint();
#endif
  }
  m_bInterrupts = bInterrupts;

}
void Spinlock::release()
{
  if (Processor::getInterrupts())
  {
    FATAL("Spinlock: release with interrupts enabled!");
    Processor::breakpoint();
  }
  while (m_Atom.compareAndSwap(false, true) == false)
  {
//    FATAL("Spinlock: failed to release!");
//    Processor::breakpoint();
  }

  // Reenable irqs if they were enabled before
  if (m_bInterrupts)
    Processor::setInterrupts(true);
}
