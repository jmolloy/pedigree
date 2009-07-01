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

#include <utilities/RadixTree.h>
#include <utilities/Tree.h>
#include <LockGuard.h>

#include <assert.h>

#include "PosixProcess.h"

#include "../modules/linker/DynamicLinker.h"
#include "../modules/vfs/File.h"
#include "../modules/vfs/LockedFile.h"

#define O_RDONLY    0
#define O_WRONLY    1
#define O_RDWR      2

#define	FD_CLOEXEC	1

typedef Tree<size_t, PosixSubsystem::SignalHandler*> sigHandlerTree;
typedef Tree<size_t, FileDescriptor*> FdMap;

RadixTree<LockedFile*> g_PosixGlobalLockedFiles;

/// Default constructor
FileDescriptor::FileDescriptor() :
    file(0), offset(0), fd(0xFFFFFFFF), fdflags(0), flflags(0), lockedFile(0)
{
}

/// Parameterised constructor
FileDescriptor::FileDescriptor(File *newFile, uint64_t newOffset, size_t newFd, int fdFlags, int flFlags, LockedFile *lf) :
    file(newFile), offset(newOffset), fd(newFd), fdflags(fdFlags), flflags(flFlags), lockedFile(lf)
{
    if(file)
    {
        lockedFile = g_PosixGlobalLockedFiles.lookup(file->getFullPath());
        file->increaseRefCount((flflags & O_RDWR) || (flflags & O_WRONLY));
    }
}

/// Copy constructor
FileDescriptor::FileDescriptor(FileDescriptor &desc) :
    file(desc.file), offset(desc.offset), fd(desc.fd), fdflags(desc.fdflags), flflags(desc.flflags), lockedFile(0)
{
    if(file)
    {
        lockedFile = g_PosixGlobalLockedFiles.lookup(file->getFullPath());
        file->increaseRefCount((flflags & O_RDWR) || (flflags & O_WRONLY));
    }
}

/// Pointer copy constructor
FileDescriptor::FileDescriptor(FileDescriptor *desc) :
    file(0), offset(0), fd(0), fdflags(0), flflags(0), lockedFile(0)
{
    if(!desc)
        return;

    file = desc->file;
    offset = desc->offset;
    fd = desc->fd;
    fdflags = desc->fdflags;
    flflags = desc->flflags;
    if(file)
    {
        lockedFile = g_PosixGlobalLockedFiles.lookup(file->getFullPath());
        file->increaseRefCount((flflags & O_RDWR) || (flflags & O_WRONLY));
    }
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
    {
        lockedFile = g_PosixGlobalLockedFiles.lookup(file->getFullPath());
        file->increaseRefCount((flflags & O_RDWR) || (flflags & O_WRONLY));
    }
    return *this;
}

/// Destructor - decreases file reference count
FileDescriptor::~FileDescriptor()
{
    if(file)
    {
        // Unlock the file we have a lock on, release from the global lock table
        if(lockedFile)
        {
            g_PosixGlobalLockedFiles.remove(file->getFullPath());
            lockedFile->unlock();
            delete lockedFile;
        }
        file->decreaseRefCount((flflags & O_RDWR) || (flflags & O_WRONLY));
    }
}

/// \todo Copy the MemoryMappedFiles tree
PosixSubsystem::PosixSubsystem(PosixSubsystem &s) :
    Subsystem(s), m_SignalHandlers(), m_SignalHandlersLock(false), m_FdMap(), m_NextFd(s.m_NextFd),
    m_FdLock(false), m_FdBitmap(), m_LastFd(0), m_FreeCount(s.m_FreeCount),
    m_MemoryMappedFiles()
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
        if(!value)
            continue;

        SignalHandler *newSig = new SignalHandler(*reinterpret_cast<SignalHandler *>(value));
        myHandlers.insert(key, newSig);
    }
}

PosixSubsystem::~PosixSubsystem()
{
    assert(--m_FreeCount == 0);

    // Ensure that no descriptor operations are taking place (and then, will take place)
    m_FdLock.acquire();

    // Modifying signal handlers, ensure that they are not in use
    LockGuard<Mutex> guard(m_SignalHandlersLock);

    // Destroy all signal handlers
    sigHandlerTree &myHandlers = m_SignalHandlers;
    List<void*> signalsToRemove;
    for(sigHandlerTree::Iterator it = myHandlers.begin(); it != myHandlers.end(); it++)
    {
        // Get the signal handler and remove it. Note that there shouldn't be null
        // SignalHandlers, at all.
        SignalHandler *sig = reinterpret_cast<SignalHandler *>(it.value());
        assert(sig);

        // SignalHandler's destructor will delete the Event itself
        delete sig;
    }

    // And now that the signals are destroyed, remove them from the Tree
    myHandlers.clear();

    // For sanity's sake, destroy any remaining descriptors
    m_FdLock.release();
    freeMultipleFds();
}

void PosixSubsystem::exit(int code)
{

    Thread *pThread = Processor::information().getCurrentThread();

    Process *pProcess = pThread->getParent();
    if (pProcess->getExitStatus() == 0)
        pProcess->setExitStatus( (code&0xFF) << 8 );
    
    // Exit called, but we could be at any nesting level in the event stack.
    // We have to propagate this exit() to all lower stack levels because they may have
    // semaphores and stuff open.

    // So, if we're not dealing with the lowest in the stack...
    if (pThread->getStateLevel() > 0)
    {
        // OK, we have other events running. They'll have to die first before we can do anything.
        pThread->setUnwindState(Thread::Exit);

        Thread *pBlockingThread = pThread->getBlockingThread(pThread->getStateLevel()-1);
        while (pBlockingThread)
        {
            pBlockingThread->setUnwindState(Thread::ReleaseBlockingThread);
            pBlockingThread = pBlockingThread->getBlockingThread();
        }

        Processor::information().getScheduler().eventHandlerReturned();
    }

    // We're the lowest in the stack, so we can proceed with the exit function.

    PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
    if (!pSubsystem)
    {
        ERROR("No subsystem for one or both of the processes!");
        return;
    }

    delete pProcess->getLinker();

    MemoryMappedFileManager::instance().unmapAll();

    // If it's a POSIX process, remove group membership
    if(pProcess->getType() == Process::Posix)
    {
        PosixProcess *p = static_cast<PosixProcess*>(pProcess);
        ProcessGroup *pGroup = p->getProcessGroup();
        if(pGroup && (p->getGroupMembership() == PosixProcess::Member))
        {
            for(List<PosixProcess*>::Iterator it = pGroup->Members.begin(); it != pGroup->Members.end(); it++)
            {
                if((*it) == p)
                {
                    it = pGroup->Members.erase(it);
                    break;
                }
            }
        }
        else if(pGroup && (p->getGroupMembership() == PosixProcess::Member))
        {
            // Group leader not handled yet!
            /// \todo The group ID must not be allowed to be allocated again until the last group member exits.
            ERROR("Group leader is exiting, not handled yet!");
        }
    }

    // Clean up the descriptor table
    pSubsystem->freeMultipleFds();

    pProcess->kill();

    // Should NEVER get here.
    /// \note asm volatile
    for (;;) asm volatile("xor %eax, %eax");
}

bool PosixSubsystem::kill(Thread *pThread)
{
    NOTICE("PosixSubsystem::kill");

    // Lock the signal handlers, so it's impossible for our signal handler to be pulled our
    // from beneath us by another CPU or something.
    LockGuard<Mutex> guard(m_SignalHandlersLock);

    // Send SIGKILL
    SignalHandler *sig = getSignalHandler(9);

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

    /// \todo WTF?
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
            NOTICE("Removing an old signal (was " << reinterpret_cast<uintptr_t>(tmp) << ").");

            // Remove from the list
            m_SignalHandlers.remove(sig);

            // Destroy the SignalHandler struct
            delete tmp;
        }

        // Insert into the signal handler table
        handler->sig = sig;

        NOTICE("Adding new signal for " << sig << ": " << reinterpret_cast<uintptr_t>(handler) << ".");
        m_SignalHandlers.insert(sig, handler);
    }
}


size_t PosixSubsystem::getFd()
{
    LockGuard<Mutex> guard(m_FdLock);

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
    LockGuard<Mutex> guard(m_FdLock); // Don't allow any access to the FD data
    if(fdNum >= m_NextFd)
        m_NextFd = fdNum + 1;
    m_FdBitmap.set(fdNum);
}

void PosixSubsystem::freeFd(size_t fdNum)
{
    LockGuard<Mutex> guard(m_FdLock); // Don't allow any access to the FD data
    m_FdBitmap.clear(fdNum);

    FileDescriptor *pFd = m_FdMap.lookup(fdNum);
    if(pFd)
    {
        m_FdMap.remove(fdNum);
        delete pFd;
    }

    if(fdNum < m_LastFd)
        m_LastFd = fdNum;
}

bool PosixSubsystem::copyDescriptors(PosixSubsystem *pSubsystem)
{
    assert(pSubsystem);

    // We're totally resetting our local state, ensure there's no files hanging around.
    freeMultipleFds();

    // Totally changing everything... Don't allow other functions to meddle.
    LockGuard<Mutex> guard(m_FdLock);
    LockGuard<Mutex> guardCopyHost(pSubsystem->m_FdLock);

    // Copy each descriptor across from the original subsystem
    FdMap &map = pSubsystem->m_FdMap;
    for(FdMap::Iterator it = map.begin(); it != map.end(); it++)
    {
        FileDescriptor *pFd = reinterpret_cast<FileDescriptor*>(it.value());
        if(!pFd)
            continue;
        size_t newFd = reinterpret_cast<size_t>(it.key());

        FileDescriptor *pNewFd = new FileDescriptor(*pFd);

        // Perform the same action as addFileDescriptor. We need to duplicate here because
        // we currently hold the FD lock, which will deadlock if we call any function which
        // attempts to acquire it.
        if(newFd >= m_NextFd)
            m_NextFd = newFd + 1;
        m_FdBitmap.set(newFd);
        m_FdMap.insert(newFd, pNewFd);
    }

    return true;
}

void PosixSubsystem::freeMultipleFds(bool bOnlyCloExec, size_t iFirst, size_t iLast)
{
    assert(iFirst < iLast);

    LockGuard<Mutex> guard(m_FdLock); // Don't allow any access to the FD data

    // Because removing FDs as we go from the Tree can actually leave the Tree
    // iterators in a dud state, we'll add all the FDs to remove to this list.
    List<void*> fdsToRemove;

    // Are all FDs to be freed? Or only a selection?
    bool bAllToBeFreed = ((iFirst == 0 && iLast == ~0UL) && !bOnlyCloExec);
    if(bAllToBeFreed)
        m_LastFd = 0;

    FdMap &map = m_FdMap;
    for(FdMap::Iterator it = map.begin(); it != map.end(); it++)
    {
        size_t Fd = reinterpret_cast<size_t>(it.key());
        FileDescriptor *pFd = reinterpret_cast<FileDescriptor*>(it.value());
        if(!pFd)
            continue;

        if(!(Fd >= iFirst && Fd <= iLast))
            continue;

        if(bOnlyCloExec)
        {
            if(!(pFd->fdflags & FD_CLOEXEC))
                continue;
        }

        // Perform the same action as freeFd. We need to duplicate code here because we currently
        // hold the FD lock, which will deadlock if we call any function which attempts to
        // acquire it.

        // No longer usable
        m_FdBitmap.clear(Fd);

        // Add to the list of FDs to remove, iff we won't be cleaning up the entire set
        if(!bAllToBeFreed)
            fdsToRemove.pushBack(reinterpret_cast<void*>(Fd));

        // Delete the descriptor itself
        delete pFd;

        // And reset the "last freed" tracking variable, if this is lower than it already.
        if(Fd < m_LastFd)
            m_LastFd = Fd;
    }

    // Clearing all AND not caring about CLOEXEC FDs? If so, clear the map. Otherwise, only
    // clear the FDs that are supposed to be cleared.
    if(bAllToBeFreed)
        m_FdMap.clear();
    else
    {
        for(List<void*>::Iterator it = fdsToRemove.begin(); it != fdsToRemove.end(); it++)
            m_FdMap.remove(reinterpret_cast<size_t>(*it));
    }
}
