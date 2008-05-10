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

#define CP0_WRITE_INDEX(val) asm volatile("mtc0 %0, $0; nop" : : "r" (val));
#define CP0_WRITE_RANDOM(val) asm volatile("mtc0 %0, $1; nop" : : "r" (val));
#define CP0_WRITE_ENTRYLO0(val) asm volatile("mtc0 %0, $2; nop" : : "r" (val));
#define CP0_WRITE_ENTRYLO1(val) asm volatile("mtc0 %0, $3; nop" : : "r" (val));
#define CP0_WRITE_CONTEXT(val) asm volatile("mtc0 %0, $4; nop" : : "r" (val));
#define CP0_WRITE_PAGEMASK(val) asm volatile("mtc0 %0, $5; nop" : : "r" (val));
#define CP0_WRITE_WIRED(val) asm volatile("mtc0 %0, $6; nop" : : "r" (val));
#define CP0_WRITE_BADVADDR(val) asm volatile("mtc0 %0, $8; nop" : : "r" (val));
#define CP0_WRITE_COUNT(val) asm volatile("mtc0 %0, $9; nop" : : "r" (val));
#define CP0_WRITE_ENTRYHI(val) asm volatile("mtc0 %0, $10; nop" : : "r" (val));
#define CP0_WRITE_COMPARE(val) asm volatile("mtc0 %0, $11; nop" : : "r" (val));
#define CP0_WRITE_SR(val) asm volatile("mtc0 %0, $12; nop" : : "r" (val));
#define CP0_WRITE_CAUSE(val) asm volatile("mtc0 %0, $13; nop" : : "r" (val));
#define CP0_WRITE_EPC(val) asm volatile("mtc0 %0, $14; nop" : : "r" (val));
#define CP0_WRITE_PRID(val) asm volatile("mtc0 %0, $15; nop" : : "r" (val));
#define CP0_WRITE_CONFIG(val) asm volatile("mtc0 %0, $16; nop" : : "r" (val));
#define CP0_WRITE_LLADDR(val) asm volatile("mtc0 %0, $17; nop" : : "r" (val));
#define CP0_WRITE_WATCHLO(val) asm volatile("mtc0 %0, $18; nop" : : "r" (val));
#define CP0_WRITE_WATCHHI(val) asm volatile("mtc0 %0, $19; nop" : : "r" (val));
#define CP0_WRITE_ECC(val) asm volatile("mtc0 %0, $26; nop" : : "r" (val));
#define CP0_WRITE_CACHEERR(val) asm volatile("mtc0 %0, $27; nop" : : "r" (val));
#define CP0_WRITE_TAGLO(val) asm volatile("mtc0 %0, $28; nop" : : "r" (val));
#define CP0_WRITE_TAGHI(val) asm volatile("mtc0 %0, $29; nop" : : "r" (val));
#define CP0_WRITE_ERROREPC(val) asm volatile("mtc0 %0, $30; nop" : : "r" (val));

#define CP0_READ_INDEX(val) asm volatile("mfc0 %0, $0; nop" : "=r" (val));
#define CP0_READ_RANDOM(val) asm volatile("mfc0 %0, $1; nop" : "=r" (val));
#define CP0_READ_ENTRYLO0(val) asm volatile("mfc0 %0, $2; nop" : "=r" (val));
#define CP0_READ_ENTRYLO1(val) asm volatile("mfc0 %0, $3; nop" : "=r" (val));
#define CP0_READ_CONTEXT(val) asm volatile("mfc0 %0, $4; nop" : "=r" (val));
#define CP0_READ_PAGEMASK(val) asm volatile("mfc0 %0, $5; nop" : "=r" (val));
#define CP0_READ_WIRED(val) asm volatile("mfc0 %0, $6; nop" : "=r" (val));
#define CP0_READ_BADVADDR(val) asm volatile("mfc0 %0, $8; nop" : "=r" (val));
#define CP0_READ_COUNT(val) asm volatile("mfc0 %0, $9; nop" : "=r" (val));
#define CP0_READ_ENTRYHI(val) asm volatile("mfc0 %0, $10; nop" : "=r" (val));
#define CP0_READ_COMPARE(val) asm volatile("mfc0 %0, $11; nop" : "=r" (val));
#define CP0_READ_SR(val) asm volatile("mfc0 %0, $12; nop" : "=r" (val));
#define CP0_READ_CAUSE(val) asm volatile("mfc0 %0, $13; nop" : "=r" (val));
#define CP0_READ_EPC(val) asm volatile("mfc0 %0, $14; nop" : "=r" (val));
#define CP0_READ_PRID(val) asm volatile("mfc0 %0, $15; nop" : "=r" (val));
#define CP0_READ_CONFIG(val) asm volatile("mfc0 %0, $16; nop" : "=r" (val));
#define CP0_READ_LLADDR(val) asm volatile("mfc0 %0, $17; nop" : "=r" (val));
#define CP0_READ_WATCHLO(val) asm volatile("mfc0 %0, $18; nop" : "=r" (val));
#define CP0_READ_WATCHHI(val) asm volatile("mfc0 %0, $19; nop" : "=r" (val));
#define CP0_READ_ECC(val) asm volatile("mfc0 %0, $26; nop" : "=r" (val));
#define CP0_READ_CACHEERR(val) asm volatile("mfc0 %0, $27; nop" : "=r" (val));
#define CP0_READ_TAGLO(val) asm volatile("mfc0 %0, $28; nop" : "=r" (val));
#define CP0_READ_TAGHI(val) asm volatile("mfc0 %0, $29; nop" : "=r" (val));
#define CP0_READ_ERROREPC(val) asm volatile("mfc0 %0, $30; nop" : "=r" (val));

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
