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
  // Save the current irq status
  m_bInterrupts = Processor::getInterrupts();

  // Disable irqs if not already done
  if (m_bInterrupts)
    Processor::setInterrupts(false);

  while (m_Atom.compareAndSwap(true, false) == false);
}
void Spinlock::release()
{
  m_Atom = true;

  // Reenable irqs if they were enabled before
  if (m_bInterrupts)
    Processor::setInterrupts(true);
}
