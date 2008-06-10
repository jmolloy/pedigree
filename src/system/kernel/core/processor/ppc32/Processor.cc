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
    NOTICE("Address: " << Hex << address);
    NOTICE("Size: " << Hex << size);
    if (address != currentMax && size > 0)
      // This means we have a memory hole. We don't handle holes atm, so we have to signal it.
      panic("Memory hole detected, and not handled");
    currentMax += size;
  }

  NOTICE("Detected " << Dec << (currentMax/0x100000) << "MB of installed RAM.");
  return currentMax;
}

static uint32_t getTranslations(Translation *translations)
{
  // Get the current translations list.
  OFDevice chosen (OpenFirmware::instance().findDevice("/chosen"));
  OFDevice mmu (chosen.getProperty("mmu"));

  if (mmu.getProperty("translations", translations, sizeof(Translation)*256) == -1)
  {
    ERROR("Translations not detected correctly");
    return 0;
  }

  return mmu.getPropertyLength("translations");
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

  PPC32VirtualAddressSpace &v = static_cast<PPC32VirtualAddressSpace&>
    (PPC32VirtualAddressSpace::getKernelAddressSpace());
  
  v.initialise();

  Translation pTranslations[256];
  uint32_t nTranslations = getTranslations(pTranslations) / sizeof(Translation);
  uint32_t ramMax = detectMemory();
  uint32_t bleh = 0;
  PPC32InterruptManager::initialiseProcessor();
  HashedPageTable::instance().initialise(pTranslations, nTranslations, ramMax);

  // Initialise this processor's interrupt handling

//   m_Initialised = 2;
}

void Processor::identify(HugeStaticString &str)
{
}
