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
#ifndef KERNEL_PROCESSOR_MIPS_COMMON_PROCESSOR_H
#define KERNEL_PROCESSOR_MIPS_COMMON_PROCESSOR_H

#define CP0_INDEX    0
#define CP0_RANDOM   1
#define CP0_ENTRYLO0 2
#define CP0_ENTRYLO1 3
#define CP0_CONTEXT  4
#define CP0_PAGEMASK 5
#define CP0_WIRED    6
// 7 N'existe pas.
#define CP0_BADVADDR 8
#define CP0_COUNT    9
#define CP0_ENTRYHI  10
#define CP0_COMPARE  11
#define CP0_SR       12
#define CP0_CAUSE    13
#define CP0_EPC      14
#define CP0_PRID     15
#define CP0_CONFIG   16
#define CP0_LLADDR   17
#define CP0_WATCHLO  18
#define CP0_WATCHHI  19
#define CP0_ECC      26
#define CP0_CACHEERR 27
#define CP0_TAGLO    28
#define CP0_TAGHI    29
#define CP0_ERROREPC 30

#define SR_IE   (1<<0)
#define SR_EXL  (1<<1)
#define SR_ERL  (1<<2)
// TODO The rest...

void Processor::breakpoint()
{
  asm volatile ("break");
}

void Processor::halt()
{
  // TODO: gcc will most certainly optimize this away in -O1/2/3 so please
  //       replace it with some unoptimizable mighty magic
  for (;;);
}

#endif
