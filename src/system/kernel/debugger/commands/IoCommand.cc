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
#include "IoCommand.h"
#include <DebuggerIO.h>
#include <processor/IoPortManager.h>
#include <processor/PhysicalMemoryManager.h>

IoCommand::IoCommand()
 : DebuggerCommand()
{
}

IoCommand::~IoCommand()
{
}

void IoCommand::autocomplete(const HugeStaticString &input, HugeStaticString &output)
{
}

bool IoCommand::execute(const HugeStaticString &input, HugeStaticString &output, InterruptState &state, DebuggerIO *pScreen)
{
  #if !defined(KERNEL_PROCESSOR_NO_PORT_IO)
    Vector<IoPortManager::IoPortInfo*> IoPorts;
  
    // Copy the list of used I/O ports
    IoPortManager &ioPortManager = IoPortManager::instance();
    ioPortManager.allocateIoPortList(IoPorts);
  
    output += "I/O ports:\n";
  
    {
      Vector<IoPortManager::IoPortInfo*>::ConstIterator i = IoPorts.begin();
      Vector<IoPortManager::IoPortInfo*>::ConstIterator end = IoPorts.end();
      for (;i != end;i++)
      {
        output += ' ';
        output += (*i)->name;
        output += ": 0x";
        output.append((*i)->ioPort, 16);
        output += " - 0x";
        output.append((*i)->ioPort + (*i)->sIoPort - 1, 16);
        output += "\n";
      }
    }
  
    // Free the list of I/O ports
    ioPortManager.freeIoPortList(IoPorts);
  #endif

  Vector<PhysicalMemoryManager::MemoryRegionInfo*> MemoryRegions;

  // Copy the list of memory-regions
  PhysicalMemoryManager &physicalMemoryManager = PhysicalMemoryManager::instance();
  physicalMemoryManager.allocateMemoryRegionList(MemoryRegions);

  output += "Memory regions:\n";

  {
    Vector<PhysicalMemoryManager::MemoryRegionInfo*>::ConstIterator i = MemoryRegions.begin();
    Vector<PhysicalMemoryManager::MemoryRegionInfo*>::ConstIterator end = MemoryRegions.end();
    for (;i != end;i++)
    {
      output += ' ';
      output += (*i)->pName;
      output += ": virtual 0x";
      output.append(reinterpret_cast<uintptr_t>((*i)->pVirtualAddress),
                    16,
                    sizeof(processor_register_t) * 2,
                    '0');
      output += " - 0x";
      output.append(reinterpret_cast<uintptr_t>((*i)->pVirtualAddress) + (*i)->sVirtualAddress,
                    16,
                    sizeof(processor_register_t) * 2,
                    '0');
      if ((*i)->physicalAddress != 0)
      {
        output += '\n';
        output.append(' ', strlen((*i)->pName) + 3, ' ');
        output += "physical 0x";
        output.append((*i)->physicalAddress,
                      16,
                      sizeof(physical_uintptr_t) * 2,
                      '0');
        output += " - 0x";
        output.append((*i)->physicalAddress + (*i)->sVirtualAddress,
                      16,
                      sizeof(physical_uintptr_t) * 2,
                      '0');
      }
      output += '\n';
    }
  }

  return true;
}
