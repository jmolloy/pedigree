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

#include <panic.h>
#include <Log.h>
#include <utilities/utility.h>
#include <processor/Processor.h>
#include <processor/types.h>
#include <processor/PhysicalMemoryManager.h>
#include "VirtualAddressSpace.h"

static bool g_Kseg2Initialised = false;

MIPS32VirtualAddressSpace MIPS32VirtualAddressSpace::m_KernelSpace;
uintptr_t MIPS32VirtualAddressSpace::m_pKseg2Directory[512];

VirtualAddressSpace &VirtualAddressSpace::getKernelAddressSpace()
{
  return MIPS32VirtualAddressSpace::m_KernelSpace;
}

MIPS32VirtualAddressSpace::MIPS32VirtualAddressSpace() :
  VirtualAddressSpace(reinterpret_cast<void*> (KERNEL_VIRTUAL_HEAP))
{
  // TODO Locks around here? When it's getting initialised I'm pretty sure
  // no other processors will be brought up, and threading won't be enabled.

  // If we haven't yet NULL'd KSEG2, do it now.
  if (!g_Kseg2Initialised)
  {
    memset(m_pKseg2Directory, 0, 512*sizeof(uintptr_t));
    g_Kseg2Initialised = true;
  }
  // NULL Kuseg.
  memset(m_pKusegDirectory, 0, 1024*sizeof(uintptr_t));
}

MIPS32VirtualAddressSpace::~MIPS32VirtualAddressSpace()
{
  for(int i = 0; i < 1024; i++)
  {
    if (m_pKusegDirectory[i] != 0)
    {
      PhysicalMemoryManager::instance().freePage(static_cast<physical_uintptr_t>(m_pKusegDirectory[i]));
    }
  }
}

bool MIPS32VirtualAddressSpace::isAddressValid(void *virtualAddress)
{
  return false;
}

bool MIPS32VirtualAddressSpace::isMapped(void *virtualAddress)
{
  return false;
}

bool MIPS32VirtualAddressSpace::map(physical_uintptr_t physicalAddress,
                                    void *virtualAddress,
                                    size_t flags)
{
  return false;
}

void MIPS32VirtualAddressSpace::getMapping(void *virtualAddress,
                                           physical_uintptr_t &physicalAddress,
                                           size_t &flags)
{
}

void MIPS32VirtualAddressSpace::setFlags(void *virtualAddress, size_t newFlags)
{
}

void MIPS32VirtualAddressSpace::unmap(void *virtualAddress)
{
}

uintptr_t MIPS32VirtualAddressSpace::getPageTableChunk(uintptr_t chunkIdx)
{
  if (chunkIdx < 1024)
  {
    return m_pKusegDirectory[chunkIdx];
  }
  else if (chunkIdx >= 1536) // 1536 = 1024+512.
  {
    return m_pKseg2Directory[chunkIdx-1536];
  }
  else
  {
    ERROR("Invalid page table chunk index: " << Dec << chunkIdx);
    return 0;
  }
}

uintptr_t MIPS32VirtualAddressSpace::generateNullChunk()
{
  physical_uintptr_t frame = PhysicalMemoryManager::instance().allocatePage();
  
  // If the page is under 512MB, we can access it through KSEG0.
  if (frame < 0x20000000)
  {
    memset(reinterpret_cast<void*> (KSEG0(frame)), 0, PAGE_SIZE);
    return static_cast<uintptr_t> (frame);
  }
  else
  {
    // We need to temporarily map the page into KSEG2.
    panic("Physical addressing over 512MB not implemented yet!");
  }
}
