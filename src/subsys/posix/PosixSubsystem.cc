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

#include <utilities/assert.h>

#include "PosixProcess.h"

#include <linker/DynamicLinker.h>
#include <vfs/File.h>
#include <vfs/LockedFile.h>

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

PosixSubsystem::PosixSubsystem(PosixSubsystem &s) :
    Subsystem(s), m_SignalHandlers(), m_SignalHandlersLock(), m_FdMap(), m_NextFd(s.m_NextFd),
    m_FdLock(false), m_FdBitmap(), m_LastFd(0), m_FreeCount(s.m_FreeCount),
    m_MemoryMappedFiles(), m_AltSigStack(), m_SyncObjects(), m_Threads()
{
    m_SignalHandlersLock.acquire();
    s.m_SignalHandlersLock.enter();

    // Copy all signal handlers
    sigHandlerTree &parentHandlers = s.m_SignalHandlers;
    sigHandlerTree &myHandlers = m_SignalHandlers;
    for(sigHandlerTree::Iterator it = parentHandlers.begin(); it != parentHandlers.end(); it++)
    {
        size_t key = it.key();
        void *value = it.value();
        if(!value)
            continue;

        SignalHandler *newSig = new SignalHandler(*reinterpret_cast<SignalHandler *>(value));
        myHandlers.insert(key, newSig);
    }

    // Copy memory mapped files
    /// \todo Is this enough?
    for(Tree<void*, MemoryMappedFile*>::Iterator it = s.m_MemoryMappedFiles.begin(); it != s.m_MemoryMappedFiles.end(); it++) {
        if(it.value()->getFile()) {
            m_MemoryMappedFiles.insert(it.key(), it.value());
        }
    }
    
    s.m_SignalHandlersLock.leave();
    m_SignalHandlersLock.release();
}

PosixSubsystem::~PosixSubsystem()
{
    assert(--m_FreeCount == 0);

    // Ensure that no descriptor operations are taking place (and then, will take place)
    m_FdLock.acquire();

    // Modifying signal handlers, ensure that they are not in use
    m_SignalHandlersLock.acquire();

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

    m_SignalHandlersLock.release();

    // For sanity's sake, destroy any remaining descriptors
    m_FdLock.release();
    freeMultipleFds();

    // Remove any POSIX threads that might still be lying around
    for(Tree<size_t, PosixThread *>::Iterator it = m_Threads.begin(); it != m_Threads.end(); it++)
    {
        PosixThread *thread = reinterpret_cast<PosixThread *>(it.value());
        assert(thread); // There shouldn't have ever been a null PosixThread in there

        // If the thread is still running, it should be killed
        if(!thread->isRunning.tryAcquire())
        {
            WARNING("PosixSubsystem object freed when a thread is still running?");
            // Thread will just stay running, won't be deallocated or killed
        }

        // Clean up any thread-specific data
        for(Tree<size_t, PosixThreadKey *>::Iterator it2 = thread->m_ThreadData.begin(); it2 != thread->m_ThreadData.end(); it2++)
        {
            PosixThreadKey *p = reinterpret_cast<PosixThreadKey *>(it.value());
            assert(p);

            /// \todo Call the destructor (need a way to call into userspace and return back here)
            delete p;
        }

        thread->m_ThreadData.clear();
    }

    m_Threads.clear();

    // Clean up synchronisation objects
    for(Tree<size_t, PosixSyncObject *>::Iterator it = m_SyncObjects.begin(); it != m_SyncObjects.end(); it++)
    {
        PosixSyncObject *p = reinterpret_cast<PosixSyncObject *>(it.value());
        assert(p);

        if(p->pObject)
        {
            if(p->isMutex)
                delete reinterpret_cast<Mutex*>(p->pObject);
            else
                delete reinterpret_cast<Semaphore*>(p->pObject);
        }
    }

    m_SyncObjects.clear();

    // Switch to the address space of the process we're destroying.
    // We need to unmap memory maps, and we can't do that in our address space.
    VirtualAddressSpace &curr = Processor::information().getVirtualAddressSpace();
    VirtualAddressSpace *va = m_pProcess->getAddressSpace();

    if(va != &curr) {
        // Switch into the address space we want to unmap inside.
        Processor::switchAddressSpace(*va);
    }

    // Clean up memory mapped files (that haven't been unmapped...)
    for(Tree<void*, MemoryMappedFile*>::Iterator it = m_MemoryMappedFiles.begin(); it != m_MemoryMappedFiles.end(); it++)
    {
        //uintptr_t addr = reinterpret_cast<uintptr_t>(it.key());
        MemoryMappedFile *pFile = unmapFile(it.key());
        if(pFile->getFile())
        {
            // It better handle files that've already been unmapped...
            MemoryMappedFileManager::instance().unmap(pFile);
        }
        else
        {
            size_t extent = pFile->getExtent();

            // Anonymous mmap, manually free the space
            size_t pageSz = PhysicalMemoryManager::getPageSize();
            size_t numPages = (extent / pageSz) + (extent % pageSz ? 1 : 0);

            uintptr_t address = reinterpret_cast<uintptr_t>(it.key());

            // Unmap!
            for (size_t i = 0; i < numPages; i++)
            {
                void *unmapAddr = reinterpret_cast<void*>(address + (i * pageSz));
                if(va->isMapped(unmapAddr))
                {
                    // Unmap the virtual address
                    physical_uintptr_t phys = 0;
                    size_t flags = 0;
                    va->getMapping(unmapAddr, phys, flags);
                    va->unmap(reinterpret_cast<void*>(unmapAddr));

                    // Free the physical page
                    PhysicalMemoryManager::instance().freePage(phys);
                }
            }

            // Free from the space allocator as well
            m_pProcess->getSpaceAllocator().free(address, extent);
            delete pFile;
        }
    }
    m_MemoryMappedFiles.clear();

    if(va != &curr) {
        Processor::switchAddressSpace(curr);
    }
}

void PosixSubsystem::exit(int code)
{
    Thread *pThread = Processor::information().getCurrentThread();

    Process *pProcess = pThread->getParent();
    if (pProcess->getExitStatus() == 0)
        pProcess->setExitStatus( (code&0xFF) << 8 );
    if(code)
    {
        WARNING("Sending unexpected exit event (" << code << ") to thread");
        pThread->unexpectedExit();
    }

    // Exit called, but we could be at any nesting level in the event stack.
    // We have to propagate this exit() to all lower stack levels because they may have
    // semaphores and stuff open.

    // So, if we're not dealing with the lowest in the stack...
    /// \note If we're at state level one, we're potentially running as a thread that has
    ///       had an event sent to it from another process. If this is changed to > 0, it
    ///       is impossible to return to a shell when a segfault occurs in an app.
    if (pThread->getStateLevel() > 1)
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
    Processor::setInterrupts(false);

    // We're the lowest in the stack, so we can proceed with the exit function.

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
        else if(pGroup && (p->getGroupMembership() == PosixProcess::Leader))
        {
            // Pick a new process to be the leader, remove this one from the list
            PosixProcess *pNewLeader = 0;
            for(List<PosixProcess*>::Iterator it = pGroup->Members.begin(); it != pGroup->Members.end(); it++)
            {
                if((*it) == p)
                    it = pGroup->Members.erase(it);
                else if(!pNewLeader)
                    pNewLeader = *it;
            }

            // Set the new leader
            pNewLeader->setGroupMembership(PosixProcess::Leader);
            pGroup->Leader = pNewLeader;
        }
    }

    // Clean up the descriptor table
    freeMultipleFds();

    pProcess->kill();

    // Should NEVER get here.
    /// \note asm volatile
    for (;;)
#if defined(X86_COMMON)
        asm volatile("xor %eax, %eax");
#elif defined(ARM_COMMON)
        asm volatile("mov r0, #0");
#else
        ;
#endif
}

bool PosixSubsystem::kill(KillReason killReason, Thread *pThread)
{
    // Send SIGKILL. getSignalHandler handles all that locking shiz for us.
    SignalHandler *sig = getSignalHandler(killReason == Interrupted ? 2 : 9);
    if(!pThread)
        pThread = Processor::information().getCurrentThread();

    if(sig && sig->pEvent)
    {
        size_t pid = pThread->getParent()->getId();
        // Send the kill event
        pThread->sendEvent(sig->pEvent);

        // Allow the event to run
        Processor::setInterrupts(true);
        Scheduler::instance().yield();
    }

    return true;
}

void PosixSubsystem::threadException(Thread *pThread, ExceptionType eType, InterruptState &state)
{
    NOTICE_NOLOCK("PosixSubsystem::threadException");

    // What was the exception?
    SignalHandler *sig = 0;
    switch(eType)
    {
        case PageFault:
            NOTICE_NOLOCK("    (Page fault)");
            // Send SIGSEGV
            sig = getSignalHandler(11);
            break;
        case InvalidOpcode:
            NOTICE_NOLOCK("    (Invalid opcode)");
            // Send SIGILL
            sig = getSignalHandler(4);
            break;
        case GeneralProtectionFault:
            NOTICE_NOLOCK("    (General Fault)");
            // Send SIGSEGV
            sig = getSignalHandler(11);
            break;
        case DivideByZero:
            NOTICE_NOLOCK("    (Division by zero)");
            // Send SIGFPE
            sig = getSignalHandler(8);
            break;
            break;
        case FpuError:
            NOTICE_NOLOCK("    (FPU error)");
            // Send SIGFPE
            sig = getSignalHandler(8);
            break;
        case SpecialFpuError:
            NOTICE_NOLOCK("    (FPU error - special)");
            // Send SIGFPE
            sig = getSignalHandler(8);
            break;
        case TerminalInput:
            NOTICE_NOLOCK("    (Attempt to read from terminal by non-foreground process)");
            // Send SIGTTIN
            sig = getSignalHandler(21);
            break;
        case TerminalOutput:
            NOTICE_NOLOCK("    (Output to terminal by non-foreground process)");
            // Send SIGTTOU
            sig = getSignalHandler(22);
            break;
        case Continue:
            NOTICE_NOLOCK("    (Continuing a stopped process)");
            // Send SIGCONT
            sig = getSignalHandler(19);
            break;
        case Stop:
            NOTICE_NOLOCK("    (Stopping a process)");
            // Send SIGTSTP
            sig = getSignalHandler(18);
            break;
        default:
            NOTICE_NOLOCK("    (Unknown)");
            // Unknown exception
            ERROR_NOLOCK("Unknown exception type in threadException - POSIX subsystem");
            break;
    }

    // If we're good to go, send the signal.
    if(sig && sig->pEvent)
    {
        bool bWasInterrupts = Processor::getInterrupts();
        Processor::setInterrupts(false);

        pThread->sendEvent(sig->pEvent);

        Processor::setInterrupts(bWasInterrupts);
        Scheduler::instance().yield();
    }
}

void PosixSubsystem::setSignalHandler(size_t sig, SignalHandler* handler)
{
    m_SignalHandlersLock.acquire();

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

    m_SignalHandlersLock.release();
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
        FileDescriptor *pFd = it.value();
        if(!pFd)
            continue;
        size_t newFd = it.key();

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
        size_t Fd = it.key();
        FileDescriptor *pFd = it.value();
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
