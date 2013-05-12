/*
 * Copyright (c) 2013 Matthew Iselin
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

#include "poll-syscalls.h"
#include <utilities/assert.h>

#include <syscallError.h>
#include <processor/types.h>
#include <processor/Processor.h>
#include <processor/MemoryRegion.h>
#include <processor/PhysicalMemoryManager.h>
#include <processor/VirtualAddressSpace.h>
#include <process/Process.h>
#include <utilities/Tree.h>
#include <vfs/File.h>
#include <vfs/LockedFile.h>
#include <vfs/MemoryMappedFile.h>
#include <vfs/Symlink.h>
#include <vfs/Directory.h>
#include <vfs/VFS.h>
#include <console/Console.h>
#include <network-stack/NetManager.h>
#include <network-stack/Tcp.h>
#include <utilities/utility.h>
#include <utilities/TimedTask.h>

#include <Subsystem.h>
#include <PosixSubsystem.h>


static void pollEventHandler(uint8_t *pBuffer);

enum TimeoutType
{
    ReturnImmediately,
    SpecificTimeout,
    InfiniteTimeout
};

PollEvent::PollEvent() :
    Event(0, false), m_pSemaphore(0), m_pFd(0), m_nREvent(0), m_pFile(0)
{
}

PollEvent::PollEvent(Semaphore *pSemaphore, struct pollfd *fd, int revent, File *pFile) :
    Event(reinterpret_cast<uintptr_t>(&pollEventHandler), false),
    m_pSemaphore(pSemaphore),
    m_pFd(fd),
    m_nREvent(revent),
    m_pFile(pFile)
{
    assert(pSemaphore);
}

PollEvent::~PollEvent()
{
}

void PollEvent::fire()
{
    m_pFd->revents |= m_nREvent;

    m_pSemaphore->release();
}

size_t PollEvent::serialize(uint8_t *pBuffer)
{
    size_t *pBuf = reinterpret_cast<size_t*>(pBuffer);
    pBuf[0] = EventNumbers::PollEvent;
    pBuf[1] = reinterpret_cast<size_t>(m_pSemaphore);
    pBuf[2] = reinterpret_cast<size_t>(m_pFd);
    pBuf[3] = static_cast<size_t>(m_nREvent);
    pBuf[4] = reinterpret_cast<size_t>(m_pFile);

    return 5 * sizeof(size_t);
}

bool PollEvent::unserialize(uint8_t *pBuffer, PollEvent &event)
{
    size_t *pBuf = reinterpret_cast<size_t*>(pBuffer);
    if (pBuf[0] != EventNumbers::PollEvent)
        return false;

    event.m_pSemaphore = reinterpret_cast<Semaphore*>(pBuf[1]);
    event.m_pFd        = reinterpret_cast<struct pollfd*>(pBuf[2]);
    event.m_nREvent    = static_cast<int>(pBuf[3]);
    event.m_pFile      = reinterpret_cast<File*>(pBuf[4]);

    return true;
}

void pollEventHandler(uint8_t *pBuffer)
{
    PollEvent e;
    if (!PollEvent::unserialize(pBuffer, e))
    {
        FATAL("PollEventHandler: unable to unserialize event!");
    }
    e.fire();
}

/** poll: determine if a set of file descriptors are writable/readable.
 *
 *  Permits any number of descriptors, unlike select().
 *  \todo Timeout
 */
int posix_poll(struct pollfd* fds, unsigned int nfds, int timeout)
{
    F_NOTICE("poll(" << Dec << nfds << ", " << timeout << Hex << ")");

    // Investigate the timeout parameter.
    TimeoutType timeoutType;
    size_t timeoutSecs = timeout / 1000;
    size_t timeoutUSecs = (timeout % 1000) * 1000;
    if (timeout < 0)
        timeoutType = InfiniteTimeout;
    else if(timeout == 0)
        timeoutType = ReturnImmediately;
    else
        timeoutType = SpecificTimeout;

    // Grab the subsystem for this process
    Process *pProcess = Processor::information().getCurrentThread()->getParent();
    PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
    if (!pSubsystem)
    {
        ERROR("No subsystem for this process!");
        return -1;
    }

    List<PollEvent*> events;

    bool bError = false;
    bool bWillReturnImmediately = (timeoutType == ReturnImmediately);
    size_t nRet = 0;
    Semaphore sem(0);
    Spinlock reentrancyLock;

    for (unsigned int i = 0; i < nfds; i++)
    {
        // Grab the pollfd structure.
        struct pollfd *me = &fds[i];
        me->revents = 0;
        if(me->fd < 0)
        {
            continue;
        }

        // valid fd?
        FileDescriptor *pFd = pSubsystem->getFileDescriptor(me->fd);
        if (!pFd)
        {
            // Error - no such file descriptor.
            ERROR("select: no such file descriptor (" << Dec << i << ")");
            me->revents |= POLLNVAL;
            bError = true;
            ++nRet;
            continue;
        }

        if (me->events & POLLIN)
        {
            // Has the file already got data in it?
            /// \todo Specify read/write/error to select and monitor.
            if (pFd->file->select(false, 0))
            {
                me->revents |= POLLIN;
                bWillReturnImmediately = true;
                nRet ++;
            }
            else if (!bWillReturnImmediately)
            {
                // Need to set up a PollEvent.
                PollEvent *pEvent = new PollEvent(&sem, me, POLLIN, pFd->file);
                reentrancyLock.acquire();
                pFd->file->monitor(Processor::information().getCurrentThread(), pEvent);
                events.pushBack(pEvent);

                // Quickly check again now we've added the monitoring event,
                // to avoid a race condition where we could miss the event.
                //
                /// \note This is safe because the event above can only be
                ///       dispatched to this thread, and while we hold the
                ///       reentrancy spinlock that cannot happen!
                if (pFd->file->select(false, 0))
                {
                    me->revents |= POLLIN;
                    bWillReturnImmediately = true;
                    nRet ++;
                }

                reentrancyLock.release();
            }
        }
        if (me->events & POLLOUT)
        {
            // Has the file already got data in it?
            /// \todo Specify read/write/error to select and monitor.
            if (pFd->file->select(true, 0))
            {
                me->revents |= POLLOUT;
                bWillReturnImmediately = true;
                nRet ++;
            }
            else if (!bWillReturnImmediately)
            {
                // Need to set up a PollEvent.
                PollEvent *pEvent = new PollEvent(&sem, me, POLLOUT, pFd->file);
                reentrancyLock.acquire();
                pFd->file->monitor(Processor::information().getCurrentThread(), pEvent);
                events.pushBack(pEvent);

                // Quickly check again now we've added the monitoring event,
                // to avoid a race condition where we could miss the event.
                //
                /// \note This is safe because the event above can only be
                ///       dispatched to this thread, and while we hold the
                ///       reentrancy spinlock that cannot happen!
                if (pFd->file->select(true, 0))
                {
                    me->revents |= POLLOUT;
                    bWillReturnImmediately = true;
                    nRet ++;
                }

                reentrancyLock.release();
            }
        }
        if (me->events & POLLERR)
        {
            F_NOTICE("    -> POLLERR not yet supported");
        }
    }

    // Grunt work is done, now time to cleanup.
    if (!bWillReturnImmediately && !bError)
    {
        // We got here because there is a specific or infinite timeout and
        // no FD was ready immediately.
        //
        // We wait on the semaphore 'sem': Its address has been given to all
        // the events and will be raised whenever an FD has action.
        assert(nRet == 0);
        sem.acquire(1, timeoutSecs, timeoutUSecs);

        // Did we actually get the semaphore or did we timeout?
        if (!Processor::information().getCurrentThread()->wasInterrupted())
        {
            // We were signalled, so one more FD ready.
            nRet ++;
            // While the semaphore is nonzero, more FDs are ready.
            while (sem.tryAcquire())
                nRet++;
        }
    }

    // Cleanup.
    reentrancyLock.acquire();

    // Ensure there are no events still pending for this thread.
    Processor::information().getCurrentThread()->cullEvent(EventNumbers::PollEvent);

    for (List<PollEvent*>::Iterator it = events.begin();
         it != events.end();
         it++)
    {
        PollEvent *pSE = *it;
        pSE->getFile()->cullMonitorTargets(Processor::information().getCurrentThread());
        delete pSE;
    }

    reentrancyLock.release();

    F_NOTICE("    -> " << Dec << ((bError) ? -1 : nRet) << Hex);

    return (bError) ? -1 : nRet;
}
