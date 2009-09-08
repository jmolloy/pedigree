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

#include "MappingCommand.h"
#include <utilities/utility.h>
#include <processor/Processor.h>
#include <processor/VirtualAddressSpace.h>
#include <processor/PhysicalMemoryManager.h>

MappingCommand::MappingCommand()
    : DebuggerCommand()
{
}

MappingCommand::~MappingCommand()
{
}

void MappingCommand::autocomplete(const HugeStaticString &input, HugeStaticString &output)
{
}

bool MappingCommand::execute(const HugeStaticString &input, HugeStaticString &output, InterruptState &state, DebuggerIO *pScreen)
{
    // If we see just "mapping", no parameters were matched.
    uintptr_t address = 0;
    if (input != "mapping")
    {
        // Is it an address?
        address = input.intValue();

        if (address == 0)
        {
            // No, try a symbol name.
            // TODO.
            output = "Not a valid address: `";
            output += input;
            output += "'.\n";
            return true;
        }
    }
    else
    {
        output = "Usage: mapping <effective address>";
        return true;
    }

    VirtualAddressSpace &thisVa = Processor::information().getVirtualAddressSpace();
    VirtualAddressSpace &kernelVa = VirtualAddressSpace::getKernelAddressSpace();

    address &= ~(PhysicalMemoryManager::getPageSize()-1);

    void *vAddr = reinterpret_cast<void*>(address);

    output = "0x";
    output.append(address, 16);
    output += ":\n";

    if (thisVa.isMapped(vAddr))
    {
        size_t flags, phys;
        thisVa.getMapping(vAddr, phys, flags);
        output += "    Mapped to ";
        output.append(phys, 16);
        output += " (flags ";
        output.append(flags, 16);
        output += ") in this address space.\n";
    }
    else
        output += "    Not mapped in this address space.\n";

    Processor::switchAddressSpace(kernelVa);

    if (kernelVa.isMapped(vAddr))
    {
        size_t flags, phys;
        kernelVa.getMapping(vAddr, phys, flags);
        output += "    Mapped to ";
        output.append(phys, 16);
        output += " (flags ";
        output.append(flags, 16);
        output += ") in the kernel address space.\n";
    }
    else
        output += "    Not mapped in the kernel address space.\n";

    Processor::switchAddressSpace(thisVa);

    return true;
}
