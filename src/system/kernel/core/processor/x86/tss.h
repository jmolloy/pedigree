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
#ifndef KERNEL_PROCESSOR_X86_TSS_H
#define KERNEL_PROCESSOR_X86_TSS_H

#include <compiler.h>
#include <processor/types.h>

/** @addtogroup kernelprocessorx86
 * @{ */

/** The protected-mode task-state segment */
struct X86TaskStateSegment
{
  uint16_t link;
  uint16_t res0;
  uint32_t esp0;
  uint16_t ss0;
  uint16_t res1;
  uint32_t esp1;
  uint16_t ss1;
  uint16_t res2;
  uint32_t esp2;
  uint16_t ss2;
  uint16_t res3;
  uint32_t cr3;
  uint32_t eip;
  uint32_t eflags;
  uint32_t eax;
  uint32_t ecx;
  uint32_t edx;
  uint32_t ebx;
  uint32_t esp;
  uint32_t ebp;
  uint32_t esi;
  uint32_t edi;
  uint16_t es;
  uint16_t res4;
  uint16_t cs;
  uint16_t res5;
  uint16_t ss;
  uint16_t res6;
  uint16_t ds;
  uint16_t res7;
  uint16_t fs;
  uint16_t res8;
  uint16_t gs;
  uint16_t res9;
  uint16_t ldt;
  uint16_t res10;
  uint16_t res11;
  uint16_t ioPermBitmap;
} PACKED;

/** @} */

#endif
