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
#include <linker/KernelElf.h>
#include <utilities/demangle.h>
#include <processor/Processor.h>
#include <Backtrace.h>

LocksCommand g_LocksCommand;
extern Spinlock g_MallocLock;

// This is global because we need to rely on it before the constructor is called.
static bool g_bReady = false;

LocksCommand::LocksCommand()
    : DebuggerCommand(), m_bAcquiring(false)
{
    memset(m_pDescriptors, 0, sizeof(LockDescriptor)*MAX_DESCRIPTORS);
}

LocksCommand::~LocksCommand()
{
}

void LocksCommand::autocomplete(const HugeStaticString &input, HugeStaticString &output)
{
}

bool LocksCommand::execute(const HugeStaticString &input, HugeStaticString &output, InterruptState &state, DebuggerIO *pScreen)
{
    // If we see just "locks", no parameters were matched.
    uintptr_t address = 0;
    if (input != "locks")
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

    for (int i = 0; i < MAX_DESCRIPTORS; i++)
    {
        LockDescriptor *pD = &m_pDescriptors[i];
        if (pD->used == false) continue;

        if (address && address != reinterpret_cast<uintptr_t>(pD->pLock))
            continue;
                
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

    if (address)
    {
        Spinlock *pLock = reinterpret_cast<Spinlock*>(address);
        output += "Lock state:\n";
        output += "  m_bInterrupts: ";
        output += pLock->m_bInterrupts;
        output += "\n  m_Atom:";
        output += (pLock->m_Atom) ? "Unlocked" : "Locked";
        output += "\n  m_Ra: ";
        output.append(pLock->m_Ra, 16);
        output += "\n  m_bAvoidTracking: ";
        output += pLock->m_bAvoidTracking;
        output += "\n  m_Magic: ";
        output.append(pLock->m_Magic, 16);
        output += "\n";
    }

    return true;
}

void LocksCommand::setReady()
{
    g_bReady = true;
}
bool g_bMallocLockAcquired = false;
void LocksCommand::lockAcquired(Spinlock *pLock)
{
    if (!g_bReady || m_bAcquiring)
        return;

    m_bAcquiring = true;

    // Get a backtrace.
    Backtrace bt;
    bt.performBpBacktrace(0, 0);

    LockDescriptor *pD = 0;
    for (int i = 0; i < MAX_DESCRIPTORS; i++)
    {
        if (m_pDescriptors[i].used == false)
        {
            pD = &m_pDescriptors[i];
            break;
        }
    }
    if (pD)
    {
        pD->used = true;
        pD->pLock = pLock;
        memcpy(&pD->ra, bt.m_pReturnAddresses, NUM_BT_FRAMES*sizeof(uintptr_t));
        pD->n = bt.m_nStackFrames;
    }
    m_bAcquiring = false;
}

void LocksCommand::lockReleased(Spinlock *pLock)
{
    if (!g_bReady || m_bAcquiring)
        return;

    m_bAcquiring = true;

    for (int i = 0; i < MAX_DESCRIPTORS; i++)
    {
        if (m_pDescriptors[i].used == true &&
            m_pDescriptors[i].pLock == pLock)
        {
            m_pDescriptors[i].used = false;
        }
    }
            
    m_bAcquiring = false;
}
