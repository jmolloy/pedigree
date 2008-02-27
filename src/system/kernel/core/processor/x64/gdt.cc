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
#include "gdt.h"
#include "tss.h"

X64GdtManager X64GdtManager::m_Instance;

X64GdtManager &X64GdtManager::instance()
{
  return m_Instance;
}

void X64GdtManager::initialise(size_t processorCount)
{
  // Calculate the number of entries
  m_DescriptorCount = 3 + 2 * processorCount;

  // Allocate the GDT
  m_Gdt = new segment_descriptor[m_DescriptorCount];

  // Fill the GDT
  setSegmentDescriptor(0, 0, 0, 0, 0);
  setSegmentDescriptor(1, 0, 0, 0x98, 0x2); // Kernel code
  setSegmentDescriptor(2, 0, 0, 0xF8, 0x2); // User code
  for (size_t i = 0;i < processorCount;i++)
  {
    X64TaskStateSegment *Tss = new X64TaskStateSegment;
    setTssDescriptor(2 * i + 3, reinterpret_cast<uint64_t>(Tss));
    // TODO: The processor object should know about its task-state-segment
  }
}
void X64GdtManager::initialiseProcessor()
{
  struct
  {
    uint16_t size;
    uint64_t gdt;
  } PACKED gdtr = {m_Instance.m_DescriptorCount * 8 - 1, reinterpret_cast<uint64_t>(m_Instance.m_Gdt)};

  asm volatile("lgdt %0" : "=m"(gdtr));
}

X64GdtManager::X64GdtManager()
  : m_Gdt(0), m_DescriptorCount(0)
{
}
X64GdtManager::~X64GdtManager()
{
}

void X64GdtManager::setSegmentDescriptor(size_t index, uint64_t base, uint32_t limit, uint8_t flags, uint8_t flags2)
{
  m_Gdt[index].limit0 = limit & 0xFFFF;
  m_Gdt[index].base0 = base & 0xFFFF;
  m_Gdt[index].base1 = (base >> 16) & 0xFF;
  m_Gdt[index].flags = flags;
  m_Gdt[index].flags_limit1 = ((flags2 & 0x0F) << 4) | ((limit >> 16) & 0x0F);
  m_Gdt[index].base2 = (base >> 24) & 0xFF;
}
void X64GdtManager::setTssDescriptor(size_t index, uint64_t base)
{
  setSegmentDescriptor(index, base & 0xFFFFFFFF, sizeof(X64TaskStateSegment), 0x89, 0x00);
  tss_descriptor *Tss = reinterpret_cast<tss_descriptor*>(&m_Gdt[index + 1]);
  Tss->base3 = (base >> 32) & 0xFFFFFFFF;
  Tss->res = 0;
}
