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

ArmV7VirtualAddressSpace ArmV7VirtualAddressSpace::m_KernelSpace;

VirtualAddressSpace &VirtualAddressSpace::getKernelAddressSpace()
{
  return ArmV7VirtualAddressSpace::m_KernelSpace;
}

VirtualAddressSpace *VirtualAddressSpace::create()
{
  // TODO
  //return new X86VirtualAddressSpace();
  return 0;
}

ArmV7VirtualAddressSpace::ArmV7VirtualAddressSpace() :
  VirtualAddressSpace(reinterpret_cast<void*> (0))
{
}

ArmV7VirtualAddressSpace::~ArmV7VirtualAddressSpace()
{
}

bool ArmV7VirtualAddressSpace::initialise()
{
  return true;
}

bool ArmV7VirtualAddressSpace::isAddressValid(void *virtualAddress)
{
    // No address is "invalid" in the sense that we're looking for here.
    return true;
}

bool ArmV7VirtualAddressSpace::isMapped(void *virtualAddress)
{
    uintptr_t addr = reinterpret_cast<uintptr_t>(virtualAddress);
    uint32_t pdir_offset = addr >> 20;
    uint32_t ptab_offset = (addr >> 12) & 0xFF;

    // Grab the entry in the page directory
    FirstLevelDescriptor *pdir = reinterpret_cast<FirstLevelDescriptor *>(m_VirtualPageDirectory);
    if(pdir[pdir_offset].descriptor.entry)
    {
        // What type is the entry?
        switch(pdir[pdir_offset].descriptor.fault.type)
        {
            case 1:
            {
                // Page table walk.
                SecondLevelDescriptor *ptbl = reinterpret_cast<SecondLevelDescriptor *>(pdir[pdir_offset].descriptor.pageTable.baseaddr << 10);
                if(!ptbl[ptab_offset].descriptor.fault.type)
                    return false;
                break;
            }
            case 2:
                // Section or supersection
                WARNING("ArmV7VirtualAddressSpace::isAddressValid - sections and supersections not yet supported");
                break;
            default:
                return false;
        }
    }

    return true;
}

bool ArmV7VirtualAddressSpace::map(physical_uintptr_t physicalAddress,
                                    void *virtualAddress,
                                    size_t flags)
{
  return false;
}

void ArmV7VirtualAddressSpace::getMapping(void *virtualAddress,
                                           physical_uintptr_t &physicalAddress,
                                           size_t &flags)
{
  physicalAddress = 0;
  flags = 0;
}

void ArmV7VirtualAddressSpace::setFlags(void *virtualAddress, size_t newFlags)
{
}

void ArmV7VirtualAddressSpace::unmap(void *virtualAddress)
{
}

void *ArmV7VirtualAddressSpace::allocateStack()
{
  // TODO
  return 0;
}
void ArmV7VirtualAddressSpace::freeStack(void *pStack)
{
  // TODO
}
