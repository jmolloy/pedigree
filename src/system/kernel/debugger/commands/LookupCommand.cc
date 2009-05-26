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

#include "LookupCommand.h"
#include <utilities/utility.h>
#include <processor/Processor.h>
#include <linker/KernelElf.h>

LookupCommand::LookupCommand()
 : DebuggerCommand()
{
}

LookupCommand::~LookupCommand()
{
}

void LookupCommand::autocomplete(const HugeStaticString &input, HugeStaticString &output)
{
}

bool LookupCommand::execute(const HugeStaticString &input, HugeStaticString &output, InterruptState &state, DebuggerIO *pScreen)
{
    intptr_t n = input.intValue();
    if (n == -1)
    {
        uintptr_t addr = KernelElf::instance().lookupSymbol(static_cast<const char*>(input));
        if (addr)
            output.append(addr, 16);
        else
            output += "No symbol corresponding to given name";
        output += "\n";
    }
    else
    {
        uintptr_t symStart;
        const char *pSym = KernelElf::instance().globalLookupSymbol(static_cast<uintptr_t>(n), &symStart);
        if (!pSym)
        {
            output += "No symbol corresponding to given address.";
            return false;
        }
        output += "`";
        output += pSym;
        output += "' - symbol starts at ";
        output.append(symStart, 16);
        output += "\n";
    }
    return true;
}
