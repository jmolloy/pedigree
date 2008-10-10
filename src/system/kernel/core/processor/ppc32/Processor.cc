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

#include <processor/Processor.h>
#include <machine/openfirmware/OpenFirmware.h>
#include <machine/openfirmware/Device.h>
#include <processor/PhysicalMemoryManager.h>
#include "../ppc_common/PhysicalMemoryManager.h"
#include "HashedPageTable.h"
#include "VirtualAddressSpace.h"
#include "InterruptManager.h"
#include "Translation.h"
#include <Log.h>
#include <panic.h>

static uint32_t detectMemory()
{
  // Grab the memory node.
  OFDevice memory (OpenFirmware::instance().findDevice("/memory"));
  // Ask it about its registers.
  uint32_t registersLength = memory.getPropertyLength("reg");
  if (registersLength == static_cast<uint32_t> (-1))
  {
    ERROR("getproplen failed for /memory:reg.");
    return 0;
  }
  uint32_t registers[32];
  if (memory.getProperty("reg", reinterpret_cast<OFParam> (registers), 32*sizeof(uint32_t)) == -1)
  {
    ERROR("getprop failed for /memory:reg.");
    return 0;
  }

  // The current upper limit of RAM.
  uint32_t currentMax = 0;

  // Registers are address:size pairs - how many did we get?
  for (uint32_t i = 0; i < registersLength/(2*sizeof(uint32_t)); i++)
  {
    uint32_t address = registers[i*2];
    uint32_t size = registers[i*2+1];
    if (address != currentMax && size > 0)
      // This means we have a memory hole. We don't handle holes atm, so we have to signal it.
      panic("Memory hole detected, and not handled");
    currentMax += size;
  }

  NOTICE("Detected " << Dec << (currentMax/0x100000) << "MB of installed RAM.");
  return currentMax;
}

void Processor::initialise1(const BootstrapStruct_t &Info)
{
  // Initialise openfirmware.
  OpenFirmware::instance().initialise(reinterpret_cast<OpenFirmware::OFInterface> (Info.prom));

  // TODO: Initialise the physical memory-management
//   m_Initialised = 1;
}

void Processor::initialise2()
{
  Translations translations;
  uint32_t ramMax = detectMemory();

  // Remove some of the chaff we don't care about - that is
  // anything mapped below KERNEL_SPACE_START that is not in the
  // bottom 0x3000.
//  translations.removeRange(0x3000, KERNEL_SPACE_START);
  translations.removeRange(0xD0000000, 0xDE000000);

  PPC32VirtualAddressSpace &v = static_cast<PPC32VirtualAddressSpace&>
    (PPC32VirtualAddressSpace::getKernelAddressSpace());
  
  v.initialise(translations);

  HashedPageTable::instance().initialise(translations, ramMax);

  // Initialise this processor's interrupt handling
  /// \note This is now done in HashedPageTable::initialise, so that page table
  ///       problems can be caught easier.
  //PPC32InterruptManager::initialiseProcessor();

  // Initialise the virtual address space.
  v.initialRoster(translations);

  // And the physical memory manager.
  PpcCommonPhysicalMemoryManager &p = static_cast<PpcCommonPhysicalMemoryManager&>
    (PpcCommonPhysicalMemoryManager::instance());
  p.initialise(translations, ramMax);

  // Floating point is available.
  uint32_t msr;
  asm volatile("mfmsr %0" : "=r" (msr));
  msr |= MSR_FP;
  asm volatile("mtmsr %0" :: "r" (msr));

  m_Initialised = 2;
}

void Processor::identify(HugeStaticString &str)
{
}

void Processor::switchAddressSpace(VirtualAddressSpace &AddressSpace)
{
  PPC32VirtualAddressSpace &s = reinterpret_cast<PPC32VirtualAddressSpace&>(AddressSpace);
  
  uint32_t vsid = static_cast<uint32_t>(s.m_Vsid)*8;

  asm volatile("mtsr 0, %0" :: "r"(vsid+0));
  asm volatile("mtsr 1, %0" :: "r"(vsid+1));
  asm volatile("mtsr 2, %0" :: "r"(vsid+2));
  asm volatile("mtsr 3, %0" :: "r"(vsid+3));
  asm volatile("mtsr 4, %0" :: "r"(vsid+4));
  asm volatile("mtsr 5, %0" :: "r"(vsid+5));
  asm volatile("mtsr 6, %0" :: "r"(vsid+6));
  asm volatile("mtsr 7, %0" :: "r"(vsid+7));
  asm volatile("sync; isync");

  Processor::information().setVirtualAddressSpace(AddressSpace);
}
