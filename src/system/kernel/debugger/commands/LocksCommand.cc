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
// #include <linker/KernelElf.h>
#include <utilities/demangle.h>
#include <processor/Processor.h>
// #include <Backtrace.h>

LocksCommand g_LocksCommand;
#ifndef TESTSUITE
extern Spinlock g_MallocLock;
#endif

// This is global because we need to rely on it before the constructor is called.
static bool g_bReady = false;

#define ERROR_OR_FATAL(x) do { \
    if (m_bFatal) \
        FATAL_NOLOCK(x); \
    else \
        ERROR_NOLOCK(x); \
} while(0)

LocksCommand::LocksCommand()
    : DebuggerCommand(), m_pDescriptors(), m_bAcquiring(), m_LockIndex(0), m_bFatal(true)
{
    for (size_t i = 0; i < LOCKS_COMMAND_NUM_CPU; ++i)
    {
        m_bAcquiring[i] = false;
        m_NextPosition[i] = 0;
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

void LocksCommand::setFatal()
{
    m_bFatal = true;
}

void LocksCommand::clearFatal()
{
    m_bFatal = false;
}

bool LocksCommand::lockAttempted(Spinlock *pLock, size_t nCpu)
{
    if (!g_bReady)
        return true;
    if (nCpu == ~0U)
        nCpu = Processor::id();

    size_t pos = (m_NextPosition[nCpu] += 1) - 1;
    if (pos > MAX_DESCRIPTORS)
    {
        ERROR_OR_FATAL("Spinlock " << pLock << " ran out of room for locks [" << pos << "].");
        return false;
    }

    LockDescriptor *pD = &m_pDescriptors[nCpu][pos];

    if (pD->state != Inactive)
    {
        ERROR_OR_FATAL("LocksCommand tracking state is corrupt.");
        return false;
    }

    pD->pLock = pLock;
    pD->state = Attempted;

    return true;
}

bool LocksCommand::lockAcquired(Spinlock *pLock, size_t nCpu)
{
    if (!g_bReady)
        return true;
    if (nCpu == ~0U)
        nCpu = Processor::id();

    size_t back = m_NextPosition[nCpu] - 1;
    if (back > MAX_DESCRIPTORS)
    {
        ERROR_OR_FATAL("Spinlock " << pLock << " acquired unexpectedly (no tracked locks).");
        return false;
    }

    LockDescriptor *pD = &m_pDescriptors[nCpu][back];

    if (pD->state != Attempted || pD->pLock != pLock)
    {
        ERROR_OR_FATAL("Spinlock " << pLock << " acquired unexpectedly.");
        return false;
    }

    pD->state = Acquired;

    return true;
}

bool LocksCommand::lockReleased(Spinlock *pLock, size_t nCpu)
{
    if (!g_bReady)
        return true;
    if (nCpu == ~0U)
        nCpu = Processor::id();

    size_t back = m_NextPosition[nCpu] - 1;
    if (back > MAX_DESCRIPTORS)
    {
        ERROR_OR_FATAL("Spinlock " << pLock << " released unexpectedly (no tracked locks).");
        return false;
    }

    LockDescriptor *pD = &m_pDescriptors[nCpu][back];

    if (pD->state != Acquired || pD->pLock != pLock)
    {
        // Maybe we need to unwind another CPU.
        /// \todo not SMP-safe...
        bool ok = false;
        for (size_t i = 0; i < LOCKS_COMMAND_NUM_CPU; ++i)
        {
            if (i == nCpu)
                continue;

            back = m_NextPosition[i] - 1;
            if (back < MAX_DESCRIPTORS)
            {
                LockDescriptor *pCheckD = &m_pDescriptors[i][back];
                if (pCheckD->state == Acquired && pCheckD->pLock == pLock)
                {
                    nCpu = i;
                    ok = true;
                    pD = pCheckD;
                    break;
                }
            }
        }

        if (!ok)
        {
            ERROR_OR_FATAL("Spinlock " << pLock << " released out-of-order [expected lock " << pD->pLock << ", state " << stateName(pD->state) << "].");
            return false;
        }
    }

    pD->pLock = 0;
    pD->state = Inactive;

    m_NextPosition[nCpu] -= 1;

    return true;
}

bool LocksCommand::checkState(Spinlock *pLock, size_t nCpu)
{
    if (!g_bReady)
        return true;
    if (nCpu == ~0U)
        nCpu = Processor::id();

    bool bResult = true;

    // Enter critical section for all cores.
    for (size_t i = 0; i < LOCKS_COMMAND_NUM_CPU; ++i)
    {
        while (!m_bAcquiring[i].compareAndSwap(false, true))
            Processor::pause();
    }

    // Check state of our lock against all other CPUs.
    for (size_t i = 0; i < LOCKS_COMMAND_NUM_CPU; ++i)
    {
        if (i == nCpu)
            continue;

        bool foundLock = false;
        LockDescriptor *pD = 0;
        for (size_t j = 0; j < m_NextPosition[i]; ++j)
        {
            pD = &m_pDescriptors[i][j];
            if (pD->state == Inactive)
            {
                pD = 0;
                break;
            }

            if (pD->pLock == pLock && pD->state == Acquired)
            {
                foundLock = true;
            }
        }

        // If the most recent lock they tried is ours, we're OK.
        if (!foundLock || !pD || pD->pLock == pLock)
        {
            continue;
        }

        if (pD->state != Attempted)
        {
            continue;
        }

        // Okay, we have an attempted lock, which we could hold.
        for (size_t j = 0; j < m_NextPosition[nCpu]; ++j)
        {
            LockDescriptor *pMyD = &m_pDescriptors[nCpu][j];
            if (pMyD->state == Inactive)
            {
                break;
            }

            if (pMyD->pLock == pD->pLock && pMyD->state == Acquired)
            {
                // We hold their attempted lock. We're waiting on them.
                // Deadlock.
                ERROR_OR_FATAL("Detected lock dependency inversion (deadlock) between " << pLock << " and " << pD->pLock << "!");
                return false;
            }
        }
    }

    // Done with critical section.
    for (size_t i = 0; i < LOCKS_COMMAND_NUM_CPU; ++i)
    {
        m_bAcquiring[i] = false;
    }

    return bResult;
}
