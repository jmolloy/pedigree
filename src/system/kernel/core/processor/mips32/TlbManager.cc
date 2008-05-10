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

#include <processor/mips32/TlbManager.h>
#include <processor/InterruptManager.h>
#include <processor/Processor.h>
#include "VirtualAddressSpace.h"
#include <Log.h>
#include <panic.h>
#include <Debugger.h>
#include "../mips_common/AsidManager.h"

MIPS32TlbManager MIPS32TlbManager::m_Instance;

MIPS32TlbManager &MIPS32TlbManager::instance()
{
  return m_Instance;
}

MIPS32TlbManager::MIPS32TlbManager() :
  m_nEntries(0)
{
}

MIPS32TlbManager::~MIPS32TlbManager()
{
}

void MIPS32TlbManager::initialise()
{
  // Firstly get the size of the TLB. This function trashes all the TLB entries
  // so is done first.
  m_nEntries = getNumEntries();
  
  // Now flush the entire TLB. This writes values in KSEG1 into all of them,
  // so that there is no possible way any could trigger.
  flush(m_nEntries);

  // Write the address of our page table into the Context register.
  writeContext(VIRTUAL_PAGE_DIRECTORY);

  // We need 1 fixed TLB entry for the VirtualAddressSpace.
  writeWired(1);

  // We use 4K pages always.
  writePageMask(0);

  // We handle the "TLB Exception (load/instruction fetch)" (2)
  //           and "TLB Exception (store)" (3)
  // exceptions.
  InterruptManager::instance().registerInterruptHandler(2, this);
  InterruptManager::instance().registerInterruptHandler(3, this);
}

void MIPS32TlbManager::interrupt(size_t interruptNumber, InterruptState &state)
{
  // The BadVAddr register will have the address that we couldn't access.
  // To make that into a 'chunk' index, we need to shift right by 12 bits
  // (to remove the page offset) and mask to only the bottom 11 bits 
  // (2048 == 0x800).
  uintptr_t badvaddr = state.m_BadVAddr;
  uintptr_t chunkIdx = (badvaddr >> 12) & 0x7FF;
  
  // Sanity check - was this TLB miss for the page table? or for elsewhere?
  if ( (badvaddr < 0xC0000000) || (badvaddr >= 0xC0800000) )
  {
    // Page fault.
    static LargeStaticString str;
    str.clear();
    str += "Page fault (";
    str.append(badvaddr, 16);
    str += ")";
    Debugger::instance().start(state, str);
    panic("Page fault");
  }

  // As the R4000 maps one virtual page to two consecutive physical frames,
  // we need to ensure that the first chunk we ask for is aligned properly.
  uintptr_t chunkSelect = chunkIdx & 0x1; // Which chunk were we accessing?
  chunkIdx &= ~0x1;

  // TODO: get the current virtual address space!
  MIPS32VirtualAddressSpace &addressSpace = static_cast<MIPS32VirtualAddressSpace&> (VirtualAddressSpace::getKernelAddressSpace());
  uintptr_t phys1 = addressSpace.getPageTableChunk(chunkIdx);
  uintptr_t phys2 = addressSpace.getPageTableChunk(chunkIdx+1);

  NOTICE("BadVaddr: " << Hex << badvaddr << ", chunkIdx: "<< chunkIdx << ", phys1: " << phys1 << ", phys2: " << phys2);

  if ((phys1 == 0 && chunkSelect == 0) ||
      (phys2 == 0 && chunkSelect == 1))
  {
    // Page fault.
    static LargeStaticString str;
    str.clear();
    str += "Page fault";
    Debugger::instance().start(state, str);
    panic("Page fault");
  }

  // Current EntryHi
  uintptr_t curEntryHi;
  CP0_READ_ENTRYHI(curEntryHi);

  NOTICE("CurEntryHi: " << Hex << curEntryHi);
  
  // Get the current ASID.
  AsidManager::Asid asid = 0; // TODO

  // Generate an EntryHi value.
  uintptr_t entryHi = (badvaddr & ~0x1FFF) | (curEntryHi & 0xFF);

  // Generate an EntryLo0 value.
  uintptr_t entryLo0 = phys1;
  if (entryLo0 != 0)
  {
    entryLo0 >>= 6;
    entryLo0 |= MIPS32_PTE_VALID | MIPS32_PTE_CACHED | MIPS32_PTE_DIRTY;
  }

  // Generate an EntryLo1 value.
  uintptr_t entryLo1 = phys2;
  if (entryLo1 != 0)
  {
    entryLo1 >>= 6;
    entryLo1 |= MIPS32_PTE_VALID | MIPS32_PTE_CACHED | MIPS32_PTE_DIRTY;
  }

  // Write into the TLB.
  writeTlb(entryHi, entryLo0, entryLo1);

  // Now that the chunk is in the TLB, we can read the bad virtual address
  // and check if it is NULL. If so, this is a page fault.
//  uintptr_t *pBadvaddr = reinterpret_cast<uintptr_t*> (badvaddr);
//  if (*pBadvaddr == 0)
//  {
//    // Page fault.
//    static LargeStaticString str;
//    str.clear();
//    str += "Page fault";
//    Debugger::instance().start(state, str);
//    panic("Page fault");
//  }

  // Success.
}
