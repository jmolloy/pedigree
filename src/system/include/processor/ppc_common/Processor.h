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

#ifndef KERNEL_PROCESSOR_PPC_COMMON_PROCESSOR_H
#define KERNEL_PROCESSOR_PPC_COMMON_PROCESSOR_H

void Processor::breakpoint()
{
  asm volatile("sc");
}

void Processor::halt()
{
  // TODO: gcc will most certainly optimize this away in -O1/2/3 so please
  //       replace it with some unoptimizable mighty magic
  for (;;);
}

void Processor::invalidate(void *pAddress)
{
  asm volatile("tlbie %0" : : "r" (pAddress));
}

void Processor::setSegmentRegisters(uint32_t segmentBase, bool supervisorKey, bool userKey)
{
  uint32_t segs[16];
  for (int i = 0; i < 16; i++)
  {
    segs[i] = 0;
    if (supervisorKey)
      segs[i] |= 0x40000000;
    if (userKey)
      segs[i] |= 0x20000000;
    segs[i] |= (segmentBase+i)&0x00FFFFFF;
  }
  asm volatile("mtsr 0, %0" : :"r"(segs[0]));
  asm volatile("mtsr 1, %0" : :"r"(segs[1]));
  asm volatile("mtsr 2, %0" : :"r"(segs[2]));
  asm volatile("mtsr 3, %0" : :"r"(segs[3]));
  asm volatile("mtsr 4, %0" : :"r"(segs[4]));
  asm volatile("mtsr 5, %0" : :"r"(segs[5]));
  asm volatile("mtsr 6, %0" : :"r"(segs[6]));
  asm volatile("mtsr 7, %0" : :"r"(segs[7]));
  asm volatile("mtsr 8, %0" : :"r"(segs[8]));
  asm volatile("mtsr 9, %0" : :"r"(segs[9]));
  asm volatile("mtsr 10, %0" : :"r"(segs[10]));
  asm volatile("mtsr 11, %0" : :"r"(segs[11]));
  asm volatile("mtsr 12, %0" : :"r"(segs[12]));
  asm volatile("mtsr 13, %0" : :"r"(segs[13]));
  asm volatile("mtsr 14, %0" : :"r"(segs[14]));
  asm volatile("mtsr 15, %0" : :"r"(segs[15]));
  asm volatile("sync");
}

#endif
