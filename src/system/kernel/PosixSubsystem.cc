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

#include "../modules/vfs/File.h"

#define O_RDONLY    0
#define O_WRONLY    1
#define O_RDWR      2

typedef Tree<size_t, PosixSubsystem::SignalHandler*> sigHandlerTree;

/// Default constructor
FileDescriptor::FileDescriptor() :
    file(0), offset(0), fd(0xFFFFFFFF), fdflags(0), flflags(0)
{
}

/// Parameterised constructor
FileDescriptor::FileDescriptor(File *newFile, uint64_t newOffset, size_t newFd, int fdFlags, int flFlags) :
    file(newFile), offset(newOffset), fd(newFd), fdflags(fdFlags), flflags(flFlags)
{
    if(file)
        file->increaseRefCount((flflags & O_RDWR) || (flflags & O_WRONLY));
}

/// Copy constructor
FileDescriptor::FileDescriptor(FileDescriptor &desc) :
    file(desc.file), offset(desc.offset), fd(desc.fd), fdflags(desc.fdflags), flflags(desc.flflags)
{
    if(file)
        file->increaseRefCount((flflags & O_RDWR) || (flflags & O_WRONLY));
}

/// Pointer copy constructor
FileDescriptor::FileDescriptor(FileDescriptor *desc) :
    file(0), offset(0), fd(0), fdflags(0), flflags(0)
{
    if(!desc)
        return;
    file = desc->file;
    offset = desc->offset;
    fd = desc->fd;
    fdflags = desc->fdflags;
    flflags = desc->flflags;
    if(file)
        file->increaseRefCount((flflags & O_RDWR) || (flflags & O_WRONLY));
}

/// Assignment operator implementation
FileDescriptor &FileDescriptor::operator = (FileDescriptor &desc)
{
    file = desc.file;
    offset = desc.offset;
    fd = desc.fd;
    fdflags = desc.fdflags;
    flflags = desc.flflags;
    if(file)
        file->increaseRefCount((flflags & O_RDWR) || (flflags & O_WRONLY));
    return *this;
}

/// Destructor - decreases file reference count
FileDescriptor::~FileDescriptor()
{
    if(file)
        file->decreaseRefCount((flflags & O_RDWR) || (flflags & O_WRONLY));
}

PosixSubsystem::PosixSubsystem(PosixSubsystem &s) :
    Subsystem(s), m_SignalHandlers(), m_SignalHandlersLock(false), m_FdMap(),
    m_NextFd(0), m_FdLock(), m_FdBitmap(), m_LastFd(0)
{
    LockGuard<Mutex> guard(m_SignalHandlersLock);
    LockGuard<Mutex> guardParent(s.m_SignalHandlersLock);

    // Copy all signal handlers
    sigHandlerTree &parentHandlers = s.m_SignalHandlers;
    sigHandlerTree &myHandlers = m_SignalHandlers;
    for(sigHandlerTree::Iterator it = parentHandlers.begin(); it != parentHandlers.end(); it++)
    {
        size_t key = reinterpret_cast<size_t>(it.key());
        void *value = it.value();

        SignalHandler *newSig = new SignalHandler(*reinterpret_cast<SignalHandler *>(value));
        myHandlers.insert(key, newSig);
    }
}

PosixSubsystem::~PosixSubsystem()
{
    // LockGuard<Mutex> guard(m_SignalHandlersLock);
    NOTICE("Destructor");

    // Destroy all signal handlers
    sigHandlerTree &myHandlers = m_SignalHandlers;
    for(sigHandlerTree::Iterator it = myHandlers.begin(); it != myHandlers.end(); it++)
    {
        size_t key = reinterpret_cast<size_t>(it.key());
        void *value = it.value();

        // Get the signal handler and remove it
        SignalHandler *sig = reinterpret_cast<SignalHandler *>(value);
        myHandlers.remove(key);

        // SignalHandler destructor will delete the Event
        delete sig;
    }
}

bool PosixSubsystem::kill(Thread *pThread)
{
    NOTICE("PosixSubsystem::kill");

    // Lock the signal handlers, so it's impossible for our signal handler to be pulled our
    // from beneath us by another CPU or something.
    LockGuard<Mutex> guard(m_SignalHandlersLock);

    // Send SIGKILL
    SignalHandler *sig = getSignalHandler(9); //reinterpret_cast<SignalHandler*>(m_SignalHandlers.lookup(9));

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

    // Lock the signal handlers, so it's impossible for our signal handler to be pulled our
    // from beneath us by another CPU or something.
    LockGuard<Mutex> guard(m_SignalHandlersLock);

    // What was the exception?
    SignalHandler *sig = 0;
    switch(eType)
    {
        case PageFault:
            NOTICE("    (Page fault)");
            // Send SIGSEGV
            sig = getSignalHandler(11);
            break;
        case InvalidOpcode:
            NOTICE("    (Invalid opcode)");
            // Send SIGILL
            sig = getSignalHandler(4);
            break;
        default:
            NOTICE("    (Unknown)");
            // Unknown exception
            ERROR("Unknown exception type in threadException - POSIX subsystem");
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


size_t PosixSubsystem::getFd()
{
    LockGuard<Spinlock> guard(m_FdLock);

    // Try to recycle if possible
    for(size_t i = m_LastFd; i < m_NextFd; i++)
    {
        if(!(m_FdBitmap.test(i)))
        {
            m_LastFd = i;
            m_FdBitmap.set(i);
            return i;
        }
    }

    // Otherwise, allocate
    // m_NextFd will always contain the highest allocated fd
    m_FdBitmap.set(m_NextFd);
    return m_NextFd++;
}

void PosixSubsystem::allocateFd(size_t fdNum)
{
    LockGuard<Spinlock> guard(m_FdLock); // Must be atomic.
    if(fdNum >= m_NextFd)
        m_NextFd = fdNum + 1;
    m_FdBitmap.set(fdNum);
}

void PosixSubsystem::freeFd(size_t fdNum)
{
    LockGuard<Spinlock> guard(m_FdLock); // Must be atomic.
    m_FdBitmap.clear(fdNum);

    FileDescriptor *pFd = m_FdMap.lookup(fdNum);
    //if(pFd)
    //    delete pFd;
    if(pFd && pFd->file)
        pFd->file->decreaseRefCount((pFd->flflags & O_RDWR) || (pFd->flflags & O_WRONLY));

    m_FdMap.remove(fdNum);

    if(fdNum < m_LastFd)
        m_LastFd = fdNum;
}
