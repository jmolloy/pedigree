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

#include "sem-syscalls.h"
#include "pthread-syscalls.h"
#include <syscallError.h>
#include "errors.h"

#include <process/Semaphore.h>
#include <process/Mutex.h>

int posix_sem_close(sem_t *sem)
{
    // Named semaphores...
    SYSCALL_ERROR(Unimplemented);
    return -1;
}

int posix_sem_destroy(sem_t *sem)
{
    PT_NOTICE("sem_destroy");

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

    pSubsystem->removeSyncObject(*sem);

    return 0;
}

int posix_sem_getvalue(sem_t *sem, int *val)
{
    PT_NOTICE("sem_getvalue");

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

    PosixSubsystem::PosixSyncObject *p = pSubsystem->getSyncObject(*sem);
    if(p && p->pObject)
    {
        *val = reinterpret_cast<Semaphore*>(p->pObject)->getValue();
        return 0;
    }

    SYSCALL_ERROR(InvalidArgument);
    return -1;
}

int posix_sem_init(sem_t *sem, int pshared, unsigned value)
{
    PT_NOTICE("sem_init");

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
    PosixSubsystem::PosixSyncObject *s = new PosixSubsystem::PosixSyncObject();
    if(p)
    {
        if(s)
        {
            s->pObject = reinterpret_cast<void*>(p);
            s->isMutex = false;
        }
        else
        {
            SYSCALL_ERROR(OutOfMemory);
            return -1;
        }

        pSubsystem->insertSyncObject(fd, s);
        return 0;
    }

    if(s)
        delete s;

    SYSCALL_ERROR(NoSpaceLeftOnDevice);
    return -1;
}

int posix_sem_post(sem_t *sem)
{
    PT_NOTICE("sem_post");

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

    PosixSubsystem::PosixSyncObject *p = pSubsystem->getSyncObject(*sem);
    if(p && p->pObject)
    {
        if(p->isMutex)
        {
            SYSCALL_ERROR(InvalidArgument);
            return -1;
        }

        reinterpret_cast<Semaphore*>(p->pObject)->release();
        return 0;
    }

    SYSCALL_ERROR(InvalidArgument);
    return -1;
}

int posix_sem_timedwait(sem_t *sem, const struct timespec *tm)
{
    PT_NOTICE("sem_timedwait");

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

    PosixSubsystem::PosixSyncObject *p = pSubsystem->getSyncObject(*sem);
    if(p && p->pObject)
    {
        if(p->isMutex)
        {
            SYSCALL_ERROR(InvalidArgument);
            return -1;
        }

        reinterpret_cast<Semaphore*>(p->pObject)->acquire(1, tm->tv_sec + (tm->tv_nsec /  1000000000));
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
    PT_NOTICE("sem_trywait");

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

    PosixSubsystem::PosixSyncObject *p = pSubsystem->getSyncObject(*sem);
    if(p && p->pObject)
    {
        if(p->isMutex)
        {
            SYSCALL_ERROR(InvalidArgument);
            return -1;
        }

        if(reinterpret_cast<Semaphore*>(p->pObject)->tryAcquire())
            return 0;

        SYSCALL_ERROR(NoMoreProcesses);
        return -1;
    }

    SYSCALL_ERROR(InvalidArgument);
    return -1;
}

int posix_sem_wait(sem_t *sem)
{
    PT_NOTICE("sem_wait");

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

    PosixSubsystem::PosixSyncObject *p = pSubsystem->getSyncObject(*sem);
    if(p && p->pObject)
    {
        if(p->isMutex)
        {
            SYSCALL_ERROR(InvalidArgument);
            return -1;
        }

        reinterpret_cast<Semaphore*>(p->pObject)->acquire();
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

int posix_pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr)
{
    return 0;
#if 0
    if(!mutex)
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
    size_t fd = *mutex = pSubsystem->getFd();

    // Attempt to create it and then insert it
    Mutex *p = new Mutex(false);
    PosixSubsystem::PosixSyncObject *s = new PosixSubsystem::PosixSyncObject();
    if(p)
    {
        if(s)
        {
            s->pObject = reinterpret_cast<void*>(p);
            s->isMutex = true;
        }
        else
        {
            SYSCALL_ERROR(OutOfMemory);
            return -1;
        }

        pSubsystem->insertSyncObject(fd, s);
        return 0;
    }

    if(s)
        delete s;

    SYSCALL_ERROR(NoSpaceLeftOnDevice);
    return -1;
#endif
}

int posix_pthread_mutex_destroy(pthread_mutex_t *mutex)
{
    return 0;
#if 0
    if(!mutex || (!mutex == PTHREAD_MUTEX_INITIALIZER))
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

    pSubsystem->removeSyncObject(*mutex);

    return 0;
#endif
}

int posix_pthread_mutex_lock(pthread_mutex_t *mutex)
{
    return 0;
#if 0
    if(!mutex)
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

    PosixSubsystem::PosixSyncObject *p = pSubsystem->getSyncObject(*mutex);
    if(p && p->pObject)
    {
        if(!p->isMutex)
        {
            SYSCALL_ERROR(InvalidArgument);
            return -1;
        }

        reinterpret_cast<Mutex*>(p->pObject)->acquire();
        if(pThread->wasInterrupted())
        {
            SYSCALL_ERROR(TimedOut);
            return -1;
        }
        return 0;
    }

    SYSCALL_ERROR(InvalidArgument);
    return -1;
#endif
}

int posix_pthread_mutex_trylock(pthread_mutex_t *mutex)
{
    return 0;
#if 0
    if(!mutex)
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

    PosixSubsystem::PosixSyncObject *p = pSubsystem->getSyncObject(*mutex);
    if(p && p->pObject)
    {
        if(!p->isMutex)
        {
            SYSCALL_ERROR(InvalidArgument);
            return -1;
        }

        if(reinterpret_cast<Mutex*>(p->pObject)->tryAcquire())
            return 0;

        SYSCALL_ERROR(NoMoreProcesses);
        return -1;
    }

    SYSCALL_ERROR(InvalidArgument);
    return -1;
#endif
}

int posix_pthread_mutex_unlock(pthread_mutex_t *mutex)
{
    return 0;
#if 0
    if(!mutex)
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

    PosixSubsystem::PosixSyncObject *p = pSubsystem->getSyncObject(*mutex);
    if(p && p->pObject)
    {
        if(!p->isMutex)
        {
            SYSCALL_ERROR(InvalidArgument);
            return -1;
        }

        reinterpret_cast<Mutex*>(p->pObject)->release();
        return 0;
    }

    SYSCALL_ERROR(InvalidArgument);
    return -1;
#endif
}
