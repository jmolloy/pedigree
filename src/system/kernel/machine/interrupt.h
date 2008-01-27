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
#ifndef KERNEL_MACHINE_INTERRUPT_H
#define KERNEL_MACHINE_INTERRUPT_H

#include <machine/state.h>

class InterruptHandler
{
  public:
    virtual void interrupt(size_t interruptNumber, InterruptState &state) = 0;
};

class InterruptManager
{
  public:
    static InterruptManager &instance();
    virtual registerInterruptHandler(size_t interruptNumber, InterruptHandler *handler) = 0;
  
  protected:
    inline InterruptManager();
    inline virtual ~InterruptManager();
  
  private:
    InterruptManager(const InterruptManager &);
    InterruptManager &operator = (const InterruptManager &);
};

//
// Part of the Implementation
//
InterruptManager::InterruptManager()
{
}
InterruptManager::~InterruptManager()
{
}

#endif
