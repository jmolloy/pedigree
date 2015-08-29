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

#include <panic.h>
#include <LockGuard.h>
#include <utilities/StaticString.h>
#include "InterruptManager.h"
#if defined(DEBUGGER)
#include <Debugger.h>
#endif

#ifdef THREADS
#include <process/Process.h>
#include <Subsystem.h>
#endif

using namespace __pedigree_hosted;

#include <signal.h>

HostedInterruptManager HostedInterruptManager::m_Instance;

#define MAX_SIGNAL  16

InterruptManager &InterruptManager::instance()
{
    return HostedInterruptManager::instance();
}

bool HostedInterruptManager::registerInterruptHandler(size_t nInterruptNumber,
        InterruptHandler *pHandler)
{
    // Lock the class until the end of the function
    LockGuard<Spinlock> lock(m_Lock);

    // Sanity checks
    if (UNLIKELY(nInterruptNumber >= MAX_SIGNAL))
        return false;
    if (UNLIKELY(pHandler != 0 && m_pHandler[nInterruptNumber] != 0))
        return false;
    if (UNLIKELY(pHandler == 0 && m_pHandler[nInterruptNumber] == 0))
        return false;

    // Change the pHandler
    m_pHandler[nInterruptNumber] = pHandler;

    return true;
}

#if defined(DEBUGGER)

bool HostedInterruptManager::registerInterruptHandlerDebugger(size_t nInterruptNumber,
        InterruptHandler *pHandler)
{
    // Lock the class until the end of the function
    LockGuard<Spinlock> lock(m_Lock);

    if(nInterruptNumber == SIGTRAP)
        NOTICE("registering debugger interrupt for SIGTRAP");

    // Sanity checks
    if (UNLIKELY(nInterruptNumber >= MAX_SIGNAL))
        return false;
    if (UNLIKELY(pHandler != 0 && m_pDbgHandler[nInterruptNumber] != 0))
        return false;
    if (UNLIKELY(pHandler == 0 && m_pDbgHandler[nInterruptNumber] == 0))
        return false;

    // Change the pHandler
    m_pDbgHandler[nInterruptNumber] = pHandler;

    return true;
}
size_t HostedInterruptManager::getBreakpointInterruptNumber()
{
    return SIGTRAP;
}
size_t HostedInterruptManager::getDebugInterruptNumber()
{
    return SIGTRAP;
}

#endif

void HostedInterruptManager::interrupt(InterruptState &interruptState)
{
    size_t nIntNumber = interruptState.getInterruptNumber();

    NOTICE("int: " << nIntNumber);

#if defined(DEBUGGER)
    {
        InterruptHandler *pHandler;

        // Get the debugger handler
        {
            LockGuard<Spinlock> lockGuard(m_Instance.m_Lock);
            pHandler = m_Instance.m_pDbgHandler[nIntNumber];
        }

        // Call the kernel debugger's handler, if any
        if (pHandler != 0)
        {
            pHandler->interrupt(nIntNumber, interruptState);
        }
    }
#endif

    InterruptHandler *pHandler;

    // Get the interrupt handler
    {
        LockGuard<Spinlock> lockGuard(m_Instance.m_Lock);
        pHandler = m_Instance.m_pHandler[nIntNumber];
    }

    // Call the normal interrupt handler, if any
    if (LIKELY(pHandler != 0))
    {
        pHandler->interrupt(nIntNumber, interruptState);
        return;
    }

    // unhandled interrupt, check for an exception
    if (LIKELY(nIntNumber != SIGTRAP && nIntNumber != SIGUSR1 && nIntNumber != SIGUSR2))
    {
        // TODO:: Check for debugger initialisation.
        // TODO: register dump, maybe a breakpoint so the deubbger can take over?
        // TODO: Rework this
        // for now just print out the exception name and number
        static LargeStaticString e;
        e.clear();
        e.append ("Signal #0x");
        e.append (nIntNumber, 16);
#if defined(DEBUGGER)
        Debugger::instance().start(interruptState, e);
#else
        panic(e);
#endif
    }
}

//
// Functions only usable in the kernel initialisation phase
//

static void handler(int which, siginfo_t *info, void *p)
{
    HostedInterruptManager *instance = reinterpret_cast<HostedInterruptManager *>(p);
    instance->signalShim(which, info);
}

void HostedInterruptManager::signalShim(int which, void *siginfo)
{
    siginfo_t *info = reinterpret_cast<siginfo_t *>(siginfo);

    InterruptState state;
    state.which = which;
    state.state = reinterpret_cast<uint64_t>(info->si_value.sival_ptr);
    interrupt(state);
}

void HostedInterruptManager::initialiseProcessor()
{
    // Set up our handler for every signal we want to trap.
    for (int i = 0; i < MAX_SIGNAL; ++i)
    {
        if(!(i == SIGUSR1 || i == SIGUSR2 || i == SIGTRAP))
            continue;
        struct sigaction act;
        memset(&act, 0, sizeof(act));
        act.sa_sigaction = handler;
        sigfillset(&act.sa_mask);
        act.sa_flags = SA_SIGINFO | SA_ONSTACK | SA_NODEFER;

        sigaction(i, &act, 0);
    }
}

HostedInterruptManager::HostedInterruptManager()
    : m_Lock()
{
    // Initialise the pointers to the pHandler
    for (size_t i = 0; i < 256; i++)
    {
        m_pHandler[i] = 0;
#ifdef DEBUGGER
        m_pDbgHandler[i] = 0;
#endif
    }
}

HostedInterruptManager::~HostedInterruptManager()
{
}
