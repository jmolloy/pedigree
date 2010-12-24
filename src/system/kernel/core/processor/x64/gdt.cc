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

// These will all be safe for use when entering a double fault handler
static char g_SafeStack[8192] = {0};

X64GdtManager X64GdtManager::m_Instance;

void X64GdtManager::initialise(size_t processorCount)
{
  // Calculate the number of entries
  m_DescriptorCount = 7 + 2 * processorCount;

  // Allocate the GDT
  m_Gdt = new segment_descriptor[m_DescriptorCount];

  // Fill the GDT
  setSegmentDescriptor(0, 0, 0, 0, 0);
  setSegmentDescriptor(1, 0, 0, 0x98, 0x2); // Kernel code - 0x08
  setSegmentDescriptor(2, 0, 0, 0x92, 0x2); // Kernel data - 0x10
  setSegmentDescriptor(3, 0, 0, 0xF8, 0x2); // User code32 - 0x18
  setSegmentDescriptor(4, 0, 0, 0xF2, 0x2); // User data32 - 0x20
  setSegmentDescriptor(5, 0, 0, 0xF8, 0x22); // User code64 - 0x28
  setSegmentDescriptor(6, 0, 0, 0xF2, 0x22); // User data64 - 0x30

#ifdef MULTIPROCESSOR

    /// \todo Multiprocessor #DF handler

  size_t i = 0;
  for (Vector<ProcessorInformation*>::Iterator it = Processor::m_ProcessorInformation.begin();
       it != Processor::m_ProcessorInformation.end();
       it++, i += 2)
  {
    NOTICE("Setting up TSS segment for CPU #" << Dec << i << Hex << ".");
  
    X64TaskStateSegment *Tss = new X64TaskStateSegment;
    initialiseTss(Tss);
    setTssDescriptor(i + 7, reinterpret_cast<uint64_t>(Tss));

    ProcessorInformation *processorInfo = *it;
    processorInfo->setTss(Tss);
    processorInfo->setTssSelector((i + 7) << 3);
    
    // TLS segment
    setSegmentDescriptor(i + 8, 0, 0xFFFFF, 0xF2, 0xC);
    processorInfo->setTlsSelector((i + 8) << 3);
  }
#else
  setSegmentDescriptor(8, 0, 0xFFFFF, 0xF2, 0xC); // User TLS data
  
  X64TaskStateSegment *Tss = new X64TaskStateSegment;
  initialiseTss(Tss);
  setTssDescriptor(7, reinterpret_cast<uint64_t>(Tss));

  ProcessorInformation &processorInfo = Processor::information();
  processorInfo.setTss(Tss);
  processorInfo.setTssSelector(7 << 3);
  processorInfo.setTlsSelector(8 << 3);

  // Create the double-fault handler TSS
  static X64TaskStateSegment DFTss;
  initialiseDoubleFaultTss(&DFTss);
  setTssDescriptor(9, reinterpret_cast<uint64_t>(&DFTss));
#endif
}

void X64GdtManager::setTlsBase(uintptr_t base)
{
#ifdef MULTIPROCESSOR
  /// \todo One TLS base per CPU
  setSegmentDescriptor(Processor::information().getTlsSelector() >> 3, base, 0xFFFFF, 0xF2, 0xC);
#else
  setSegmentDescriptor(8, base, 0xFFFFF, 0xF2, 0xC); // User TLS data
#endif
}

void X64GdtManager::initialiseProcessor()
{
  struct
  {
    uint16_t size;
    uint64_t gdt;
  } PACKED gdtr = {m_Instance.m_DescriptorCount * 8 - 1, reinterpret_cast<uint64_t>(m_Instance.m_Gdt)};

  asm volatile("lgdt %0" :: "m"(gdtr));
  asm volatile("ltr %%ax" :: "a" (Processor::information().getTssSelector()));

  loadSegmentRegisters();
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
void X64GdtManager::initialiseTss(X64TaskStateSegment *pTss)
{
  memset( reinterpret_cast<void*> (pTss), 0, sizeof(X64TaskStateSegment) );
}

void X64GdtManager::initialiseDoubleFaultTss(X64TaskStateSegment *pTss)
{
    /// \todo Write.
    
    return;
}

