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

#include "gdt.h"
#include <utilities/utility.h>
#include <processor/Processor.h>

X86GdtManager X86GdtManager::m_Instance;

void X86GdtManager::initialise(size_t processorCount)
{
  // Calculate the number of entries
  m_DescriptorCount = 5 + processorCount;

  // Allocate the GDT
  m_Gdt = new segment_descriptor[m_DescriptorCount];

  // Fill the GDT
  setSegmentDescriptor(0, 0, 0, 0, 0);
  setSegmentDescriptor(1, 0, 0xFFFFF, 0x98, 0xC); // Kernel code
  setSegmentDescriptor(2, 0, 0xFFFFF, 0x92, 0xC); // Kernel data
  setSegmentDescriptor(3, 0, 0xFFFFF, 0xF8, 0xC); // User code
  setSegmentDescriptor(4, 0, 0xFFFFF, 0xF2, 0xC); // User data

#ifdef MULTIPROCESSOR
  size_t i = 0;
  for (Vector<ProcessorInformation*>::Iterator it = Processor::m_ProcessorInformation.begin();
       it != Processor::m_ProcessorInformation.end();
       it++, i++)
  {
    X86TaskStateSegment *Tss = new X86TaskStateSegment;
    initialiseTss(Tss);
    setTssDescriptor(i + 5, reinterpret_cast<uint32_t>(Tss));

    ProcessorInformation *processorInfo = *it;
    processorInfo->setTss(Tss);
    processorInfo->setTssSelector((i + 5) << 3);
  }
#else
  X86TaskStateSegment *Tss = new X86TaskStateSegment;
  initialiseTss(Tss);
  setTssDescriptor(5, reinterpret_cast<uint32_t>(Tss));

  ProcessorInformation &processorInfo = Processor::information();
  processorInfo.setTss(Tss);
  processorInfo.setTssSelector(5 << 3);
#endif

}
void X86GdtManager::initialiseProcessor()
{
  struct
  {
    uint16_t size;
    uint32_t gdt;
  } PACKED gdtr = {m_Instance.m_DescriptorCount * 8 - 1, reinterpret_cast<uintptr_t>(m_Instance.m_Gdt)};

  asm volatile("lgdt %0" :: "m"(gdtr));
  asm volatile("ltr %%ax" :: "a" (Processor::information().getTssSelector()));
}

X86GdtManager::X86GdtManager()
  : m_Gdt(0), m_DescriptorCount(0)
{
}
X86GdtManager::~X86GdtManager()
{
}

void X86GdtManager::setSegmentDescriptor(size_t index, uint32_t base, uint32_t limit, uint8_t flags, uint8_t flags2)
{
  m_Gdt[index].limit0 = limit & 0xFFFF;
  m_Gdt[index].base0 = base & 0xFFFF;
  m_Gdt[index].base1 = (base >> 16) & 0xFF;
  m_Gdt[index].flags = flags;
  m_Gdt[index].flags_limit1 = ((flags2 & 0x0F) << 4) | ((limit >> 16) & 0x0F);
  m_Gdt[index].base2 = (base >> 24) & 0xFF;
}
void X86GdtManager::setTssDescriptor(size_t index, uint32_t base)
{
  setSegmentDescriptor(index, base, sizeof(X86TaskStateSegment), 0x89, 0x00);
}
void X86GdtManager::initialiseTss(X86TaskStateSegment *pTss)
{
  memset( reinterpret_cast<void*> (pTss), 0, sizeof(X86TaskStateSegment) );
  pTss->ss0 = 0x10;
}
