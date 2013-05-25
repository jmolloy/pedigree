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

#include "select-syscalls.h"
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


static void selectEventHandler(uint8_t *pBuffer);

enum TimeoutType
{
    ReturnImmediately,
    SpecificTimeout,
    InfiniteTimeout
};

SelectEvent::SelectEvent() :
    Event(0, false), m_pSemaphore(0), m_pFdSet(0), m_FdIdx(0), m_pFile(0)
{
}

SelectEvent::SelectEvent(Semaphore *pSemaphore, fd_set *pFdSet, size_t fdIdx, File *pFile) :
    Event(reinterpret_cast<uintptr_t>(&selectEventHandler), false),
    m_pSemaphore(pSemaphore),
    m_pFdSet(pFdSet),
    m_FdIdx(fdIdx),
    m_pFile(pFile)
{
    assert(pSemaphore);
}

SelectEvent::~SelectEvent()
{
}

void SelectEvent::fire()
{
    FD_SET(m_FdIdx, m_pFdSet);

    m_pSemaphore->release();
}

size_t SelectEvent::serialize(uint8_t *pBuffer)
{
    size_t *pBuf = reinterpret_cast<size_t*>(pBuffer);
    pBuf[0] = EventNumbers::SelectEvent;
    pBuf[1] = reinterpret_cast<size_t>(m_pSemaphore);
    pBuf[2] = reinterpret_cast<size_t>(m_pFdSet);
    pBuf[3] = m_FdIdx;
    pBuf[4] = reinterpret_cast<size_t>(m_pFile);

    return 5 * sizeof(size_t);
}

bool SelectEvent::unserialize(uint8_t *pBuffer, SelectEvent &event)
{
    size_t *pBuf = reinterpret_cast<size_t*>(pBuffer);
    if (pBuf[0] != EventNumbers::SelectEvent)
        return false;

    event.m_pSemaphore = reinterpret_cast<Semaphore*>(pBuf[1]);
    event.m_pFdSet     = reinterpret_cast<fd_set*>(pBuf[2]);
    event.m_FdIdx      = pBuf[3];
    event.m_pFile      = reinterpret_cast<File*>(pBuf[4]);

    return true;
}

void selectEventHandler(uint8_t *pBuffer)
{
    SelectEvent e;
    if (!SelectEvent::unserialize(pBuffer, e))
    {
        FATAL("SelectEventHandler: unable to unserialize event!");
    }
    e.fire();
}

int posix_select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *errorfds, timeval *timeout)
{
    // Investigate the timeout parameter.
    TimeoutType timeoutType;
    size_t timeoutSecs = 0, timeoutUSecs = 0;
    if (timeout == 0)
        timeoutType = InfiniteTimeout;
    else if (timeout->tv_sec == 0 && timeout->tv_usec == 0)
        timeoutType = ReturnImmediately;
    else
    {
        timeoutType = SpecificTimeout;
        timeoutSecs = timeout->tv_sec;
        timeoutUSecs = timeout->tv_usec;
        /// \bug Unfortunately we don't sleep for under one second. So if we
        ///      end up with secs < 1, we set ourselves to ReturnImmediately.
        //if (timeoutSecs == 0)
        //    timeoutType = ReturnImmediately;
    }

    F_NOTICE("select(" << Dec << nfds << ", ?, ?, ?, {" << static_cast<uintptr_t>(timeoutType) << ", " << timeoutSecs << "})" << Hex);

    Process *pProcess = Processor::information().getCurrentThread()->getParent();
    PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
    if (!pSubsystem)
    {
        ERROR("No subsystem for this process!");
        return -1;
    }

    List<SelectEvent*> events;

    bool bError = false;
    bool bWillReturnImmediately = (timeoutType == ReturnImmediately);
    size_t nRet = 0;
    Semaphore sem(0);
    Spinlock reentrancyLock;

    // Walk the fd_sets.
    for (int i = 0; i < nfds; i++)
    {
        // valid fd?
        FileDescriptor *pFd = 0;
        if ((readfds && FD_ISSET(i, readfds)) ||
            (writefds && FD_ISSET(i, writefds)) ||
            (errorfds && FD_ISSET(i, errorfds)))
        {
            pFd = pSubsystem->getFileDescriptor(i);
            if (!pFd)
            {
                // Error - no such file descriptor.
                ERROR("select: no such file descriptor (" << Dec << i << ")");
                bError = true;
                break;
            }
        }

        if (readfds && FD_ISSET(i, readfds))
        {
            // Has the file already got data in it?
            /// \todo Specify read/write/error to select and monitor.
            if (pFd->file->select(false, 0))
            {
                bWillReturnImmediately = true;
                nRet ++;
            }
            else if (bWillReturnImmediately)
                FD_CLR(i, readfds);
            else
            {
                FD_CLR(i, readfds);
                // Need to set up a SelectEvent.
                SelectEvent *pEvent = new SelectEvent(&sem, readfds, i, pFd->file);
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
                    bWillReturnImmediately = true;
                    nRet ++;
                }

                reentrancyLock.release();
            }
        }
        if (writefds && FD_ISSET(i, writefds))
        {
            // Has the file already got data in it?
            /// \todo Specify read/write/error to select and monitor.
            if (pFd->file->select(true, 0))
            {
                bWillReturnImmediately = true;
                nRet ++;
            }
            else if (bWillReturnImmediately)
                FD_CLR(i, writefds);
            else
            {
                FD_CLR(i, writefds);
                // Need to set up a SelectEvent.
                SelectEvent *pEvent = new SelectEvent(&sem, writefds, i, pFd->file);
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
                    bWillReturnImmediately = true;
                    nRet ++;
                }

                reentrancyLock.release();
            }
        }
        if (errorfds && FD_ISSET(i, errorfds))
        {
            FD_CLR(i, errorfds);
        }
        /*
        if (errorfds && FD_ISSET(i, errorfds))
        {
            // Has the file already got data in it?
            /// \todo Specify read/write/error to select and monitor.
            if (pFd->file->select(false, 0))
            {
                bWillReturnImmediately = true;
                nRet ++;
            }
            else if (bWillReturnImmediately)
                FD_CLR(i, errorfds);
            else
            {
                FD_CLR(i, errorfds);
                // Need to set up a SelectEvent.
                SelectEvent *pEvent = new SelectEvent(&sem, errorfds, i, pFd->file);
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
                    bWillReturnImmediately = true;
                    nRet ++;
                }

                reentrancyLock.release();
            }
        }
        */
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
    Processor::information().getCurrentThread()->cullEvent(EventNumbers::SelectEvent);

    for (List<SelectEvent*>::Iterator it = events.begin();
         it != events.end();
         it++)
    {
        SelectEvent *pSE = *it;
        pSE->getFile()->cullMonitorTargets(Processor::information().getCurrentThread());
        delete pSE;
    }

    reentrancyLock.release();

    F_NOTICE("    -> " << Dec << ((bError) ? -1 : nRet) << Hex);

    return (bError) ? -1 : nRet;
}
