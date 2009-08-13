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

#include "LocksCommand.h"
#include <utilities/utility.h>
#include <processor/Processor.h>
#include <KernelElf.h>
#include <utilities/demangle.h>
#include <processor/Processor.h>
#include <Backtrace.h>

LocksCommand g_LocksCommand;

LocksCommand::LocksCommand()
    : DebuggerCommand(), m_Descriptors()
{
}

LocksCommand::~LocksCommand()
{
}

void LocksCommand::autocomplete(const HugeStaticString &input, HugeStaticString &output)
{
}

bool LocksCommand::execute(const HugeStaticString &input, HugeStaticString &output, InterruptState &state, DebuggerIO *pScreen)
{
    for (Tree<Spinlock*,LockDescriptor*>::Iterator it = m_Descriptors.begin();
         it != m_Descriptors.end();
         it++)
    {
        LockDescriptor *pD = reinterpret_cast<LockDescriptor*>(it.value());
        output += "Lock at ";
        output.append(reinterpret_cast<uintptr_t>(pD->pLock), 16);
        output += ":\n";

        for (size_t i = 0; i < pD->n; i++)
        {
            uintptr_t symStart = 0;
            const char *pSym = KernelElf::instance().globalLookupSymbol(pD->ra[i], &symStart);
            if (pSym == 0)
            {
                output += " - ";
                output.append(pD->ra[i], 16);
            }
            else
            {
                LargeStaticString sym(pSym);

                output += " - [";
                output.append(symStart, 16);
                output += "] ";
                static symbol_t symbol;
                demangle(sym, &symbol);
                output += static_cast<const char*>(symbol.name);
            }
            output += "\n";
        }
    }

    return true;
}

void LocksCommand::lockAcquired(Spinlock *pLock)
{
    // Get a backtrace.
    Backtrace bt;
    bt.performBpBacktrace(0, 0);

    LockDescriptor *pD = new LockDescriptor;
    pD->pLock = pLock;
    memcpy(&pD->ra, bt.m_pReturnAddresses, NUM_BT_FRAMES*sizeof(uintptr_t));
    
    m_Descriptors.insert(pLock, pD);
}

void LocksCommand::lockReleased(Spinlock *pLock)
{
    LockDescriptor *pD = m_Descriptors.lookup(pLock);
    if (pD)
    {
        delete pD;
    }
    m_Descriptors.remove(pLock);
}
