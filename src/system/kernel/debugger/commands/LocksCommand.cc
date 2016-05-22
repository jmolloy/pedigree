/*
 * Copyright (c) 2008-2014, Pedigree Developers
 *
 * Please see the CONTRIB file in the root of the source tree for a full
 * list of contributors.
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
    : DebuggerCommand(), m_bAcquiring(), m_LockIndex(0)
{
    ByteSet(m_pDescriptors, 0, sizeof(LockDescriptor) * MAX_DESCRIPTORS * LOCKS_COMMAND_NUM_CPU);
    for (size_t i = 0; i < LOCKS_COMMAND_NUM_CPU; ++i)
    {
        m_bAcquiring[i] = false;
    }
}

LocksCommand::~LocksCommand()
{
}

void LocksCommand::autocomplete(const HugeStaticString &input, HugeStaticString &output)
{
}

bool LocksCommand::execute(const HugeStaticString &input, HugeStaticString &output, InterruptState &state, DebuggerIO *pScreen)
{
    return false;
#if 0
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

        for (size_t j = 0; j < pD->n; j++)
        {
            uintptr_t symStart = 0;
            const char *pSym = KernelElf::instance().globalLookupSymbol(pD->ra[j], &symStart);
            if (pSym == 0)
            {
                output += " - ";
                output.append(pD->ra[j], 16);
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
        output += static_cast<unsigned>(pLock->m_bInterrupts);
        output += "\n  m_Atom:";
        output += (pLock->m_Atom) ? "Unlocked" : "Locked";
        output += "\n  m_Ra: ";
        output.append(pLock->m_Ra, 16);
        output += "\n  m_bAvoidTracking: ";
        output += static_cast<unsigned>(pLock->m_bAvoidTracking);
        output += "\n  m_Magic: ";
        output.append(pLock->m_Magic, 16);
        output += "\n";
    }

    return true;
#endif
}

void LocksCommand::setReady()
{
    g_bReady = true;
}

void LocksCommand::lockAttempted(Spinlock *pLock)
{
    if (!g_bReady || m_bAcquiring[Processor::id()])
        return;

    m_bAcquiring[Processor::id()] = true;

    // Get a backtrace.
    //Backtrace bt;
    //bt.performBpBacktrace(0, 0);

    LockDescriptor *pD = 0;
    for (int i = 0; i < MAX_DESCRIPTORS; i++)
    {
        if (m_pDescriptors[Processor::id()][i].used)
        {
            pD = &m_pDescriptors[Processor::id()][i];
            break;
        }
    }
    if (pD)
    {
        pD->used = true;
        pD->pLock = pLock;
        //MemoryCopy(&pD->ra, bt.m_pReturnAddresses, NUM_BT_FRAMES * sizeof(uintptr_t));
        //pD->n = bt.m_nStackFrames;
        pD->index = m_LockIndex += 1;
        pD->attempt = true;
    }

    m_bAcquiring[Processor::id()] = false;
}

void LocksCommand::lockAcquired(Spinlock *pLock)
{
    if (!g_bReady || m_bAcquiring[Processor::id()])
        return;

    m_bAcquiring[Processor::id()] = true;

    LockDescriptor *pD = 0;
    for (int i = 0; i < MAX_DESCRIPTORS; i++)
    {
        if (m_pDescriptors[Processor::id()][i].pLock == pLock)
        {
            pD = &m_pDescriptors[Processor::id()][i];
            break;
        }
    }
    if (pD)
    {
        // We got the lock, this is no longer an attempt.
        pD->attempt = false;
    }

    m_bAcquiring[Processor::id()] = false;
}

void LocksCommand::lockReleased(Spinlock *pLock)
{
    if (!g_bReady || m_bAcquiring[Processor::id()])
        return;

    m_bAcquiring[Processor::id()] = true;

    for (int i = 0; i < MAX_DESCRIPTORS; i++)
    {
        if (m_pDescriptors[Processor::id()][i].used &&
            m_pDescriptors[Processor::id()][i].pLock == pLock)
        {
            m_pDescriptors[Processor::id()][i].used = false;
        }
    }

    m_bAcquiring[Processor::id()] = false;
}

bool LocksCommand::checkState(Spinlock *pLock)
{
    // Core 0: A, B
    // Core 1: B, attempt A
    // ^ Detect this.

    bool bResult = true;

    // Enter critical section for all cores.
    for (size_t i = 0; i < LOCKS_COMMAND_NUM_CPU; ++i)
    {
        while (!m_bAcquiring[i].compareAndSwap(false, true))
            Processor::pause();
    }

    // ORDERING
    // Example: Core 0: A.acquire, B.acquire; Core 1: B.acquire, A.acquire
    // We consider this a failed check, as neither lock can ever be released.
    //
    // checkState() would get called twice: on core 0, after B.acquire fails,
    // and on core 1, after A.acquire fails.
    //
    // We'll have tracked an attempt for both A and B on their respective CPUs,
    // and a successful lock on the opposite CPU.
    //
    // Core 0:
    // lockAttempted(A)
    // lockAcquired(A)
    //
    // Core 1:
    // lockAttempted(B)
    // lockAcquired(B)
    //
    // Core 0:
    // lockAttempted(B)
    // checkState(B)
    //
    // Core 1:
    // lockAttempted(A)
    // checkState(A)
    //
    // (fail, all cores are in "attempted" state)
    //
    // It'd be nice to print more information, but as a first approximation we
    // can certainly know that, if each core is in an "attempted" state, and
    // have been checked once (as only a failed attempt calls checkState), the
    // system is deadlocked.
    //
    // Note: it should also be possible to find inversion and show the locks
    // involved; this would require a different data structure I think (queue
    // instead of the current model that just finds a place to fit a lock).

    for (size_t i = 0; i < LOCKS_COMMAND_NUM_CPU; ++i)
    {
        // Check all locks on this CPU.
        for (size_t j = 0; j < MAX_DESCRIPTORS; ++j)
        {
            LockDescriptor *pD = &m_pDescriptors[i][j];
            if (pD->used)
            {
                if (pD->lock == pLock)
                {
                    //
                }
            }
        }
    }

    // Want to detect incorrect ordering

    // Done with critical section.
    for (size_t i = 0; i < LOCKS_COMMAND_NUM_CPU; ++i)
    {
        m_bAcquiring[i] = false;
    }

    return bResult;
}
