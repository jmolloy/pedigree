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

#include "DynamicLinker.h"
#include <Log.h>
#include <vfs/VFS.h>
#include <utilities/StaticString.h>
#include <Module.h>
#include <processor/Processor.h>
#include <processor/VirtualAddressSpace.h>
#include <processor/PhysicalMemoryManager.h>
#include <processor/KernelCoreSyscallManager.h>
#include <process/Scheduler.h>
#include <panic.h>

extern "C" void plt_resolve(void);

#define HA(x) (((x >> 16) + ((x & 0x8000) ? 1 : 0)) & 0xFFFF)

void DynamicLinker::initPlt(Elf *pElf, uintptr_t value)
{
  // The PLT on PowerPC is structured differently to i686. The first 18 words (72 bytes) are reserved for the dynamic linker (us).
  // In there, we copy the contents of ./asm-ppc.s.
  // After that comes the PLT itself. It's not initialised - we must do that ourselves. We get two words in each entry to play with.
  // Ather that comes the PLT table - it's a table at the end of the PLT, one word for each entry. In here we put the address of the
  // resolved symbol location, and 'plt_call' uses that to jump correctly.

  // The PLTGOT entry in the Elf's dynamic structure which usually represents the GOT address in PPC gives the PLT address.
  uintptr_t prePlt = pElf->getGlobalOffsetTable ();
  uintptr_t pltEntries = prePlt + 72;
  uintptr_t pltSize = pElf->getPltSize ();
  uintptr_t postPlt = pltEntries + pltSize;

  if (prePlt == value) return; // No PLT.

  // Memcpy over the pre-plt functions into the user address space.
  // plt_resolve is an ASM function, defined in ./asm-ppc.s
  memcpy(reinterpret_cast<uint8_t*> (prePlt), reinterpret_cast<uint8_t*> (&::plt_resolve), 72); // 72 bytes exactly. See ELF spec.

  // There are 4 words in the stub we just copied that need relocation:
  // addis 4, 0, 0   # 4 -- These will be filled in by the pltInit function
  // addi 4, 4, 0    # 5 -- to be a library identifier.
  //
  // addis 11, 11, 0 # 13 -- These get filled in by the pltInit function
  // lwz 11, 0(11)   # 14 -- to be the address of the PLT table.

  uint32_t *pWord = (uint32_t*) (prePlt+4*4);
  *pWord = (*pWord) | HA(value);

  pWord++;
  *pWord = (*pWord) | (value&0xFFFF);

  pWord = (uint32_t*) (prePlt+13*4);
  *pWord = (*pWord) | HA(postPlt);

  pWord++;
  *pWord = (*pWord) | (postPlt&0xFFFF);

  // Each PLT entry consists of an addi and a branch;
  //  plt$i:
  //    addi r11, r0, 4*$i
  //    b    plt_resolve  (which is located at prePlt+0)

  for (int i = 0; i < pltSize/8; i++)
  {
    //                    1111111111222222222233
    // ADDI:    01234567890123456789012345678901
    //          OPC---DEST-ADD--IMMEDIATE-------
    //        0b0011100101100000---IMMEDIATE----
    //        0x3   9   6   0
    uint32_t addi = 0x39600000 | (4*i);

    //                    1111111111222222222233
    // B:       01234567890123456789012345678901
    //          OPC---LI----------------------AL
    //        0b010010---IMMEDIATE------------00
    //        0x4   8
    int32_t offset = (int32_t)postPlt - (int32_t)(pltEntries+8*i+4);
    uint32_t b = 0x48000000 | offset;

    uint32_t *pEntry = (uint32_t*) (pltEntries + i*8);
    *pEntry++ = addi;
    *pEntry   = b;
  }
}

uintptr_t DynamicLinker::resolvePltSymbol(uintptr_t libraryId, uintptr_t symIdx)
{
  // Find the correct ELF to patch.
  Elf *pElf = 0;
  Elf *pOrigElf = 0;
  uintptr_t loadBase = 0;

  pOrigElf = m_ProcessElfs.lookup(Processor::information().getCurrentThread()->getParent());

  if (libraryId == 0)
    pElf = m_ProcessElfs.lookup(Processor::information().getCurrentThread()->getParent());
  else
  {
    // Library search.
    // Grab the list of loaded shared objects for this process.
    List<SharedObject*> *pList = m_ProcessObjects.lookup(Processor::information().getCurrentThread()->getParent());

    // Look through the shared object list.
    for (List<SharedObject*>::Iterator it = pList->begin();
        it != pList->end();
        it++)
    {
      pElf = findElf(libraryId, *it, loadBase);
      if (pElf != 0) break;
    }
  }

  /// \todo Why is there a /4 here?
  uintptr_t result = pElf->applySpecificRelocation(symIdx/4, pOrigElf->getSymbolTable(), loadBase, SymbolTable::NotOriginatingElf);

  return result;
}
