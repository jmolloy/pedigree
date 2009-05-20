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

#include <PosixSubsystem.h>
#include <Log.h>

#include <process/Thread.h>
#include <processor/types.h>
#include <processor/Processor.h>
#include <process/SignalEvent.h>
#include <process/Scheduler.h>

#include <utilities/Tree.h>
#include <LockGuard.h>

typedef Tree<size_t, void*> sigHandlerTree;

PosixSubsystem::PosixSubsystem(PosixSubsystem &s) :
    Subsystem(s), m_SignalHandlers(), m_SignalHandlersLock(false), m_FdMap(), m_NextFd(0), m_FdLock()
{
    // Copy all signal handlers
    sigHandlerTree &parentHandlers = s.m_SignalHandlers;
    sigHandlerTree &myHandlers = m_SignalHandlers;
    for(sigHandlerTree::Iterator it = parentHandlers.begin(); it != parentHandlers.end(); it++)
    {
        size_t key = reinterpret_cast<size_t>(it.key());
        void *value = it.value();

        SignalHandler *newSig = new SignalHandler(*reinterpret_cast<SignalHandler *>(value));
        myHandlers.insert(key, reinterpret_cast<void *>(newSig));
    }
}

bool PosixSubsystem::kill(Thread *pThread)
{
    NOTICE("PosixSubsystem::kill");

    // Send SIGKILL
    SignalHandler *sig = reinterpret_cast<SignalHandler*>(m_SignalHandlers.lookup(9));

    if(sig && sig->pEvent)
    {
        pThread->sendEvent(sig->pEvent);
        Processor::setInterrupts(true);
    }

    // Hang the thread
    while(1)
        Scheduler::instance().yield();
    return true;
}

void PosixSubsystem::threadException(Thread *pThread, ExceptionType eType, InterruptState &state)
{
    NOTICE("PosixSubsystem::threadException");

    // What was the exception?
    SignalHandler *sig = 0;
    switch(eType)
    {
        case PageFault:
            NOTICE("    (Page fault)");
            // Send SIGSEGV
            sig = reinterpret_cast<SignalHandler*>(m_SignalHandlers.lookup(11));
            break;
        case InvalidOpcode:
            NOTICE("    (Invalid opcode)");
            // Send SIGILL
            sig = reinterpret_cast<SignalHandler*>(m_SignalHandlers.lookup(4));
            break;
        default:
            NOTICE("    (Unknown)");
            // Unknown exception
            ERROR("Unknown exception typein threadException - POSIX subsystem");
            break;
    }

    if(sig && sig->pEvent)
    {
        bool bWasInterrupts = Processor::getInterrupts();
        Processor::setInterrupts(false);

        pThread->sendEvent(sig->pEvent);
        Processor::information().getScheduler().checkEventState(state.getStackPointer());

        Processor::setInterrupts(bWasInterrupts);
    }

    // Hang the thread
    while(1)
        Scheduler::instance().yield();
}

void PosixSubsystem::setSignalHandler(size_t sig, SignalHandler* handler)
{
    LockGuard<Mutex> guard(m_SignalHandlersLock);

    sig %= 32;
    if(handler)
    {
        SignalHandler* tmp;
        tmp = reinterpret_cast<SignalHandler*>(m_SignalHandlers.lookup(sig));
        if(tmp)
        {
            // Remove from the list
            m_SignalHandlers.remove(sig);

            // Destroy the SignalHandler struct
            delete tmp;
        }

        // Insert into the signal handler table
        handler->sig = sig;

        m_SignalHandlers.insert(sig, handler);
    }
}

