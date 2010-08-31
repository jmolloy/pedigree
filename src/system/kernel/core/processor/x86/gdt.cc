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
#include <processor/VirtualAddressSpace.h>

X86GdtManager X86GdtManager::m_Instance;

// These will all be safe for use when entering a double fault handler
static char g_SafeStack[8192] = {0};

void X86GdtManager::initialise(size_t processorCount)
{
  // Calculate the number of entries
  m_DescriptorCount = 6 + (processorCount * 2);

  // Allocate the GDT
  m_Gdt = new segment_descriptor[m_DescriptorCount];

  // Fill the GDT
  setSegmentDescriptor(0, 0, 0, 0, 0);
  setSegmentDescriptor(1, 0, 0xFFFFF, 0x98, 0xC); // Kernel code - 0x08
  setSegmentDescriptor(2, 0, 0xFFFFF, 0x92, 0xC); // Kernel data - 0x10
  setSegmentDescriptor(3, 0, 0xFFFFF, 0xF8, 0xC); // User code - 0x18
  setSegmentDescriptor(4, 0, 0xFFFFF, 0xF2, 0xC); // User data - 0x20

#ifdef MULTIPROCESSOR

    /// \todo Multiprocessor #DF handler

  size_t i = 0;
  for (Vector<ProcessorInformation*>::Iterator it = Processor::m_ProcessorInformation.begin();
       it != Processor::m_ProcessorInformation.end();
       it++, i += 2)
  {
    NOTICE("Setting up TSS segment for CPU #" << Dec << i << Hex << ".");
  
    X86TaskStateSegment *Tss = new X86TaskStateSegment;
    initialiseTss(Tss);
    setTssDescriptor(i + 5, reinterpret_cast<uint32_t>(Tss));

    ProcessorInformation *processorInfo = *it;
    processorInfo->setTss(Tss);
    processorInfo->setTssSelector((i + 5) << 3);
    
    // TLS segment
    setSegmentDescriptor(i + 6, 0, 0xFFFFF, 0xF2, 0xC);
    processorInfo->setTlsSelector((i + 6) << 3);
  }
#else
  setSegmentDescriptor(6, 0, 0xFFFFF, 0xF2, 0xC); // User TLS data - 0x30
  
  X86TaskStateSegment *Tss = new X86TaskStateSegment;
  initialiseTss(Tss);
  setTssDescriptor(5, reinterpret_cast<uint32_t>(Tss)); // 0x28

  ProcessorInformation &processorInfo = Processor::information();
  processorInfo.setTss(Tss);
  processorInfo.setTssSelector(5 << 3);
  processorInfo.setTlsSelector(6 << 3);

  // Create the double-fault handler TSS
  static X86TaskStateSegment DFTss;
  initialiseDoubleFaultTss(&DFTss);
  setTssDescriptor(7, reinterpret_cast<uint32_t>(&DFTss)); // 0x38
#endif

}
void X86GdtManager::initialiseProcessor()
{
  struct
  {
    uint16_t size;
    uint32_t gdt;
  } PACKED gdtr = {static_cast<uint16_t>((m_Instance.m_DescriptorCount * 8 - 1) & 0xFFFF), reinterpret_cast<uintptr_t>(m_Instance.m_Gdt)};

  asm volatile("lgdt %0" :: "m"(gdtr));
  asm volatile("ltr %%ax" :: "a" (Processor::information().getTssSelector()));

  // Flush the segment registers (until now we're running with the ones the Multiboot loader set us
  // up with, which, when we *reload* them, may not be valid in the new GDT)
  asm volatile("jmp $0x8, $.flush; \
                .flush: \
                mov $0x10, %ax; \
                mov %ax, %ds; \
                mov %ax, %es; \
                mov %ax, %fs; \
                mov %ax, %gs; \
                mov %ax, %ss;");
}

X86GdtManager::X86GdtManager()
  : m_Gdt(0), m_DescriptorCount(0)
{
}
X86GdtManager::~X86GdtManager()
{
}

void X86GdtManager::setTlsBase(uintptr_t base)
{
#ifdef MULTIPROCESSOR
  /// \todo One TLS base per CPU
  setSegmentDescriptor(Processor::information().getTlsSelector() >> 3, base, 0xFFFFF, 0xF2, 0xC);
#else
  setSegmentDescriptor(6, base, 0xFFFFF, 0xF2, 0xC); // User TLS data - 0x28
#endif
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

struct linearAddress
{
    uint32_t offset : 12;
    uint32_t tbl : 10;
    uint32_t dir : 10;
} __attribute__((packed));

void X86GdtManager::initialiseDoubleFaultTss(X86TaskStateSegment *pTss)
{
    // All our interrupt handlers - the TSS will hook into the DF handler, and the entry point
    extern uintptr_t interrupt_handler_array[];
    extern void start();

    memset( reinterpret_cast<void*> (pTss), 0, sizeof(X86TaskStateSegment) );

    // Stack - set to the statically allocated stack (mapped later)
    pTss->ss0 = pTss->ss = 0x10;
    pTss->esp0 = pTss->esp = pTss->esp1 = pTss->esp2 = reinterpret_cast<uint32_t>(g_SafeStack) + 8192;

    // When the fault occurs, jump to the ISR handler (so it'll automatically jump to the common ISR handler)
    pTss->eip = static_cast<uint32_t>(interrupt_handler_array[8]);

    // Set up the segments
    pTss->ds = pTss->es = pTss->fs = pTss->gs = 0x10;
    pTss->cs = 0x08;

    // Grab the kernel address space and clone it
    /// \note This doesn't seem to work??? I've used the current CR3 for now, but it's probably
    ///       best to create a new PD for the DF handler.
#if 0
    VirtualAddressSpace *dfAddressSpace = VirtualAddressSpace::getKernelAddressSpace().clone();
    NOTICE("CR3 1 = " << Processor::readCr3());
    Processor::switchAddressSpace(*dfAddressSpace);
    NOTICE("CR3 2 = " << Processor::readCr3());
    pTss->cr3 = Processor::readCr3();
    Processor::switchAddressSpace(VirtualAddressSpace::getKernelAddressSpace());
#else
    pTss->cr3 = Processor::readCr3();
#endif

    return;
}
