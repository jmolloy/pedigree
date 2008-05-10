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

#ifndef KERNEL_PROCESSOR_X86_PROCESSOR_H
#define KERNEL_PROCESSOR_X86_PROCESSOR_H

#include <Log.h>

void Processor::contextSwitch(const ProcessorState &state)
{
  // Here we change stacks and set everything up for an IRET.
  uintptr_t *pStack = reinterpret_cast<uintptr_t*> (state.esp);

  *--pStack = state.eip;
  *--pStack = state.ebp;
  *--pStack = state.eax;
  *--pStack = state.ebx;
  *--pStack = state.ecx;
  *--pStack = state.edx;
  *--pStack = state.esi;
  *--pStack = state.edi;
  
  // Now we change stacks.
  asm volatile("mov %0, %%esp" : : "r" (reinterpret_cast<uintptr_t> (pStack)));

  asm volatile("pop %edi; \
                pop %esi; \
                pop %edx; \
                pop %ecx; \
                pop %ebx; \
                pop %eax; \
                pop %ebp; \
                ret");
}


#endif
