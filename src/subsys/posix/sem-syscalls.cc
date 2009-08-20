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

#include <sem-syscalls.h>
#include <syscallError.h>
#include "errors.h"

int posix_sem_close(sem_t *sem)
{
    // Named semaphores...
    SYSCALL_ERROR(Unimplemented);
    return -1;
}

int posix_sem_destroy(sem_t *sem)
{
    if(!sem)
    {
        SYSCALL_ERROR(InvalidArgument);
        return -1;
    }

    Process *pProcess = Processor::information().getCurrentThread()->getParent();
    PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
    if (!pSubsystem)
    {
        ERROR("No subsystem for this process!");
        return -1;
    }

    pSubsystem->removeSemaphore(*sem);

    return 0;
}

int posix_sem_getvalue(sem_t *sem, int *val)
{
    if(!sem)
    {
        SYSCALL_ERROR(InvalidArgument);
        return -1;
    }

    Process *pProcess = Processor::information().getCurrentThread()->getParent();
    PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
    if (!pSubsystem)
    {
        ERROR("No subsystem for this process!");
        return -1;
    }

    Semaphore *p = pSubsystem->getSemaphore(*sem);
    if(p)
    {
        *val = p->getValue();
        return 0;
    }

    SYSCALL_ERROR(InvalidArgument);
    return -1;
}

int posix_sem_init(sem_t *sem, int pshared, unsigned value)
{
    if(!sem)
    {
        SYSCALL_ERROR(InvalidArgument);
        return -1;
    }

    Process *pProcess = Processor::information().getCurrentThread()->getParent();
    PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
    if (!pSubsystem)
    {
        ERROR("No subsystem for this process!");
        return -1;
    }

    // Grab a descriptor for this semaphore
    size_t fd = *sem = pSubsystem->getFd();

    // Attempt to create it and then insert it
    Semaphore *p = new Semaphore(value);
    if(p)
    {
        pSubsystem->insertSemaphore(fd, p);
        return 0;
    }

    SYSCALL_ERROR(NoSpaceLeftOnDevice);
    return -1;
}

int posix_sem_post(sem_t *sem)
{
    if(!sem)
    {
        SYSCALL_ERROR(InvalidArgument);
        return -1;
    }

    Process *pProcess = Processor::information().getCurrentThread()->getParent();
    PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
    if (!pSubsystem)
    {
        ERROR("No subsystem for this process!");
        return -1;
    }

    Semaphore *p = pSubsystem->getSemaphore(*sem);
    if(p)
    {
        p->release();
        return 0;
    }

    SYSCALL_ERROR(InvalidArgument);
    return -1;
}

int posix_sem_timedwait(sem_t *sem, const struct timespec *tm)
{
    if(!sem)
    {
        SYSCALL_ERROR(InvalidArgument);
        return -1;
    }

    Thread *pThread = Processor::information().getCurrentThread();
    Process *pProcess = pThread->getParent();
    PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
    if (!pSubsystem)
    {
        ERROR("No subsystem for this process!");
        return -1;
    }

    Semaphore *p = pSubsystem->getSemaphore(*sem);
    if(p)
    {
        p->acquire(1, tm->tv_sec + (tm->tv_nsec /  1000000000));
        if(pThread->wasInterrupted())
        {
            SYSCALL_ERROR(TimedOut);
            return -1;
        }
        return 0;
    }

    SYSCALL_ERROR(InvalidArgument);
    return -1;
}

int posix_sem_trywait(sem_t *sem)
{
    if(!sem)
    {
        SYSCALL_ERROR(InvalidArgument);
        return -1;
    }

    Thread *pThread = Processor::information().getCurrentThread();
    Process *pProcess = pThread->getParent();
    PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
    if (!pSubsystem)
    {
        ERROR("No subsystem for this process!");
        return -1;
    }

    Semaphore *p = pSubsystem->getSemaphore(*sem);
    if(p)
    {
        if(p->tryAcquire())
            return 0;

        SYSCALL_ERROR(NoMoreProcesses);
        return -1;
    }

    SYSCALL_ERROR(InvalidArgument);
    return -1;
}

int posix_sem_wait(sem_t *sem)
{
    if(!sem)
    {
        SYSCALL_ERROR(InvalidArgument);
        return -1;
    }

    Thread *pThread = Processor::information().getCurrentThread();
    Process *pProcess = pThread->getParent();
    PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
    if (!pSubsystem)
    {
        ERROR("No subsystem for this process!");
        return -1;
    }

    Semaphore *p = pSubsystem->getSemaphore(*sem);
    if(p)
    {
        /// \todo Interruptible by a signal (EINTR)
        p->acquire();
        return 0;
    }

    SYSCALL_ERROR(InvalidArgument);
    return -1;
}
