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

VirtualAddressSpace *VirtualAddressSpace::create()
{
  // TODO
  //return new X86VirtualAddressSpace();
  return 0;
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
  // In MIPS, all possible addresses are valid.
  return true;
}

bool MIPS32VirtualAddressSpace::isMapped(void *virtualAddress)
{
  // What page is this virtual address in?
  uintptr_t page = reinterpret_cast<uintptr_t> (virtualAddress) >> 12;

  // What page table chunk is that page's mapping located in?
  // (pageIndex * 2) / 4096, because each page has 2 words of data associated with it.
  uintptr_t chunkIdx = (page*2) / 4096;
  // At what offset in that chunk will we find information about the page mapping?
  uintptr_t chunkOff = (page*2) % 4096;

  // Get the chunk.
  uintptr_t chunkAddr = getPageTableChunk(chunkIdx);

  // If it was NULL, there is no mapping.
  if (chunkAddr == 0)
    return false;

  // Is this in the first 512MB of RAM? if so, we can access it via KSEG0.
  if (chunkAddr < 0x20000000)
  {
    uintptr_t mapping = * reinterpret_cast<uintptr_t*> (KSEG0(chunkAddr)+chunkOff);
    // Is it NULL?
    if (mapping == 0)
      return false;
    // Is it valid?
    if ((mapping & MIPS32_PTE_VALID) == 0)
      return false;
    // Otherwise, the mapping is valid.
    return true;
  }
  else
  {
    // We need to temporarily map the page into KSEG2.
    panic("Physical addressing over 512MB not implemented yet!");
  }
}

bool MIPS32VirtualAddressSpace::map(physical_uintptr_t physicalAddress,
                                    void *virtualAddress,
                                    size_t flags)
{
  // Cast virtualAddress into a nicer type for arithmetic.
  uintptr_t nVirtualAddress = reinterpret_cast<uintptr_t> (virtualAddress);

  // Firstly, do a sanity check to see if virtualAddress is within KSEG2 or KUSEG.
  if ( !IS_KUSEG(nVirtualAddress) && !IS_KSEG2(nVirtualAddress))
  {
    WARNING("MIPS32VirtualAddressSpace::map(" << Hex << physicalAddress << ", " << nVirtualAddress << ", " << flags << "): invalid virtual address.");
    return false;
  }

  // Secondly, sanity check to ensure this address isn't where the page table should be...!
  if ( (nVirtualAddress >= 0xC0000000) && (nVirtualAddress < 0xC0800000) )
  {
    WARNING("MIPS32VirtualAddressSpace::map(" << Hex << physicalAddress << ", " << nVirtualAddress << ", " << flags << "): invalid virtual address.");
    return false;
  }

  // What page is this virtual address in?
  uintptr_t page = nVirtualAddress >> 12;

  // What page table chunk is that page's mapping located in?
  // Each chunk can hold 512 descriptors (4096/8).
  uintptr_t chunkIdx = page / 512;
  // At what offset in that chunk will we find information about the page mapping?
  // Each chunk can hold 512 descriptors (4096/8), each being 8 bytes.
  uintptr_t chunkOff = (page % 512) * 8;

  // Get the chunk.
  uintptr_t chunkAddr = getPageTableChunk(chunkIdx);

  // If it was NULL, there is no mapping.
  if (chunkAddr == 0)
  {
    // ...So we have to make one!
    chunkAddr = generateNullChunk();
    setPageTableChunk(chunkIdx, chunkAddr);
  }

  // Is this in the first 512MB of RAM? if so, we can access it via KSEG0.
  if (chunkAddr < 0x20000000)
  {
    uintptr_t *mapping = reinterpret_cast<uintptr_t*> (KSEG0(chunkAddr)+chunkOff);
    // Is it non-NULL?
    if (*mapping != 0)
    {
      // Address is already mapped.
      WARNING("MIPS32VirtualAddressSpace::map(" << Hex << physicalAddress << ", " << nVirtualAddress << ", " << flags << "): Address already mapped.");
      return false;
    }

    // Make our mapping.

    //  33 22222222221111111111
    //  10 987654321098765432109876 543 2 1 0
    // +--+------------------------+---+-+-+-+
    // |00|  PHYSICAL FRAME NUMBER | C |D|V|G|
    // +--+------------------------+---+-+-+-+

    uintptr_t pfn = (static_cast<uintptr_t> (physicalAddress) >> 6) & 0x3FFFFFC0; // Shift right 6 because the 12 LSBs of an address are dumped.
    uintptr_t c   = (flags & CacheDisable) ? MIPS32_PTE_UNCACHED : MIPS32_PTE_CACHED;
    uintptr_t d   = (flags & Write) ? MIPS32_PTE_DIRTY : 0;
    uintptr_t v   = MIPS32_PTE_VALID;
    uintptr_t g   = 0; /// \todo When is an entry global? In KSEG2? But the page table TLB entries are ASID specific... hmm.
    // Write the EntryLo mapping.
    *mapping = pfn | c | d | v | g;

    // The next 32 bits are ours to mess around with, we store the processor independent flag set there.
    *++mapping = flags;

    // We are success.
    return true;
  }
  else
  {
    // We need to temporarily map the page into KSEG2.
    panic("Physical addressing over 512MB not implemented yet!");
  }
}

void MIPS32VirtualAddressSpace::getMapping(void *virtualAddress,
                                           physical_uintptr_t &physicalAddress,
                                           size_t &flags)
{
  // What page is this virtual address in?
  uintptr_t page = reinterpret_cast<uintptr_t> (virtualAddress) >> 12;

  // What page table chunk is that page's mapping located in?
  // (pageIndex * 2) / 4096, because each page has 2 words of data associated with it.
  uintptr_t chunkIdx = (page*2) / 4096;
  // At what offset in that chunk will we find information about the page mapping?
  uintptr_t chunkOff = (page*2) % 4096;

  // Get the chunk.
  uintptr_t chunkAddr = getPageTableChunk(chunkIdx);

  // If it was NULL, there is no mapping.
  if (chunkAddr == 0)
  {
    physicalAddress = 0;
    return;
  }

  // Is this in the first 512MB of RAM? if so, we can access it via KSEG0.
  if (chunkAddr < 0x20000000)
  {
    uintptr_t mapping = * reinterpret_cast<uintptr_t*> (KSEG0(chunkAddr)+chunkOff);
    uintptr_t _flags  = * reinterpret_cast<uintptr_t*> (KSEG0(chunkAddr)+chunkOff+4);
    // Is it NULL?
    if (mapping == 0)
    {
      physicalAddress = 0;
      return;
    }
    // Is it valid?
    if ((mapping & MIPS32_PTE_VALID) == 0)
    {
      physicalAddress = 0;
      return;
    }
    // Otherwise, the mapping is valid.
    physicalAddress = (mapping << 6) & 0xFFFFF000;
    flags = _flags;
    return;
  }
  else
  {
    // We need to temporarily map the page into KSEG2.
    panic("Physical addressing over 512MB not implemented yet!");
  }
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

void *MIPS32VirtualAddressSpace::allocateStack()
{
  // TODO
  return 0;
}
void MIPS32VirtualAddressSpace::freeStack(void *pStack)
{
  // TODO
}

void MIPS32VirtualAddressSpace::setPageTableChunk(uintptr_t chunkIdx, uintptr_t chunkAddr)
{
  NOTICE("setPageTableChunk(" << Hex << chunkIdx << ", " << chunkAddr << ")");
  if (chunkIdx < 1024)
  {
    m_pKusegDirectory[chunkIdx] = chunkAddr;
  }
  else if (chunkIdx >= 1536) // 1536 = 1024+512.
  {
    m_pKseg2Directory[chunkIdx-1536] = chunkAddr;
  }
  else
  {
    ERROR("Invalid page table chunk index: " << Dec << chunkIdx);
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
