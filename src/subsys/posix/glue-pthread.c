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

#include "syscallNumbers.h"

#include "errno.h"
#define errno (*__errno())
extern int *__errno (void);
int h_errno; // required by networking code

// Define errno before including syscall.h.
#include "syscall.h"

#include "newlib.h"

#define _PTHREAD_ATTR_MAGIC 0xdeadbeef

typedef void (*pthread_once_func_t)(void);
int onceFunctions[32] = {0};

int pthread_once(pthread_once_t *once_control, pthread_once_func_t init_routine)
{
    if(!once_control || (*once_control > 32))
    {
        syslog(LOG_DEBUG, "[%d] pthread_once called with an invalid once_control (> 32)", getpid());
        return -1;
    }

    if(!onceFunctions[*once_control])
    {
        init_routine();
        onceFunctions[*once_control] = 1;
    }

    return 0;
}

int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void (*start_routine)(void*), void *arg)
{
    return syscall4(POSIX_PTHREAD_CREATE, (int) thread, (int) attr, (int) start_routine, (int) arg);
}

int pthread_join(pthread_t thread, void **value_ptr)
{
    return syscall2(POSIX_PTHREAD_JOIN, (int) thread, (int) value_ptr);
}

void pthread_exit(void *ret)
{
    syscall1(POSIX_PTHREAD_RETURN, (int) ret);
}

int pthread_detach(pthread_t thread)
{
    return syscall1(POSIX_PTHREAD_DETACH, thread);
}

pthread_t pthread_self()
{
    return syscall0(POSIX_PTHREAD_SELF);
}

int pthread_equal(pthread_t t1, pthread_t t2)
{
    return (t1 == t2) ? 1 : 0;
}

int pthread_kill(pthread_t thread, int sig)
{
    return syscall2(POSIX_PTHREAD_KILL, (int) thread, sig);
}

int pthread_sigmask(int how, const sigset_t *set, sigset_t *oset)
{
    return syscall3(POSIX_PTHREAD_SIGMASK, how, (int) set, (int) oset);
}

int pthread_attr_init(pthread_attr_t *attr)
{
    if(!attr)
    {
        errno = ENOMEM;
        return -1;
    }

    if(attr->magic == _PTHREAD_ATTR_MAGIC)
    {
        errno = EBUSY;
        return -1;
    }

    attr->stackSize = 0x100000;
    attr->detachState = 0;
    attr->magic = _PTHREAD_ATTR_MAGIC;
    return 0;
}

int pthread_attr_destroy(pthread_attr_t *attr)
{
    if(!attr)
    {
        errno = ENOMEM;
        return -1;
    }

    if(attr->magic != _PTHREAD_ATTR_MAGIC)
    {
        errno = EINVAL;
        return -1;
    }

    return 0;
}

int pthread_attr_getdetachstate(const pthread_attr_t *attr, int *ret)
{
    if(!attr || (attr->magic != _PTHREAD_ATTR_MAGIC) || !ret)
    {
        errno = EINVAL;
        return -1;
    }

    *ret = attr->detachState;
    return -1;
}

int pthread_attr_setdetachstate(pthread_attr_t *attr, int detachstate)
{
    if(
        (!attr || (attr->magic != _PTHREAD_ATTR_MAGIC))
        ||
        (detachstate != PTHREAD_CREATE_DETACHED && detachstate != PTHREAD_CREATE_JOINABLE)
        )
    {
        errno = EINVAL;
        return -1;
    }

    attr->detachState = detachstate;
    return -1;
}

int pthread_attr_getstacksize(const pthread_attr_t *attr, size_t *sz)
{
    if(!attr || (attr->magic != _PTHREAD_ATTR_MAGIC) || !sz)
    {
        errno = EINVAL;
        return -1;
    }

    *sz = attr->stackSize;

    return 0;
}

int pthread_attr_setstacksize(pthread_attr_t *attr, size_t stacksize)
{
    if(!attr || (attr->magic != _PTHREAD_ATTR_MAGIC) || (stacksize < PTHREAD_STACK_MIN) || (stacksize > (1 << 24)))
    {
        errno = EINVAL;
        return -1;
    }

    attr->stackSize = stacksize;

    return 0;
}

int pthread_attr_getschedparam(const pthread_attr_t *attr, struct sched_param *param)
{
    return 0;
}

int pthread_attr_setschedparam(pthread_attr_t *attr, const struct sched_param *param)
{
    return 0;
}

int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr)
{
    return syscall2(POSIX_PTHREAD_MUTEX_INIT, (int) mutex, (int) attr);
}

int pthread_mutex_destroy(pthread_mutex_t *mutex)
{
    return syscall1(POSIX_PTHREAD_MUTEX_DESTROY, (int) mutex);
}

int pthread_mutex_lock(pthread_mutex_t *mutex)
{
    if(!mutex)
    {
        errno = EINVAL;
        return -1;
    }

    if(*mutex == PTHREAD_MUTEX_INITIALIZER)
    {
        if(pthread_mutex_init(mutex, 0) == -1)
            return -1;
    }

    return syscall1(POSIX_PTHREAD_MUTEX_LOCK, (int) mutex);
}

int pthread_mutex_trylock(pthread_mutex_t *mutex)
{
    if(!mutex)
    {
        errno = EINVAL;
        return -1;
    }

    if(*mutex == PTHREAD_MUTEX_INITIALIZER)
    {
        if(pthread_mutex_init(mutex, 0) == -1)
            return -1;
    }

    return syscall1(POSIX_PTHREAD_MUTEX_TRYLOCK, (int) mutex);
}

int pthread_mutex_unlock(pthread_mutex_t *mutex)
{
    return syscall1(POSIX_PTHREAD_MUTEX_UNLOCK, (int) mutex);
}

int pthread_mutexattr_init(pthread_mutexattr_t *attr)
{
    return 0;
}

int pthread_mutexattr_destroy(pthread_mutexattr_t *attr)
{
    return 0;
}

/**
 * Some background on how I've decided to handle condvars...
 *
 * Basically, waiting on a resource to reach a certain value, while keeping a
 * lock on it, is the idea of condvars. So I've decided to implement condvars
 * as mutexes, which will (hopefully!) work just as well.
 */

int pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr)
{
    return pthread_mutex_init(cond, 0);
}

int pthread_cond_destroy(pthread_cond_t *cond)
{
    return pthread_mutex_destroy(cond);
}

int pthread_cond_broadcast(pthread_cond_t *cond)
{
    return pthread_mutex_unlock(cond);
}

/// \note pthread_cond_signal will unblock *at least one* thread... In this
///       implementation, it really is just like broadcast.
int pthread_cond_signal(pthread_cond_t *cond)
{
    return pthread_mutex_unlock(cond);
}

int pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex, const struct timespec *tm)
{
    errno = ENOSYS;
    return -1;
}

int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex)
{
    if(pthread_mutex_unlock(mutex) == -1)
        return -1;
    pthread_mutex_lock(cond);
    pthread_mutex_lock(mutex);
    return 0;
}

int pthread_condattr_destroy(pthread_condattr_t *attr)
{
    return 0;
}

int pthread_condattr_init(pthread_condattr_t *attr)
{
    return 0;
}

void* pthread_getspecific(pthread_key_t key)
{
    return (void*) syscall1(POSIX_PTHREAD_GETSPECIFIC, key);
}

int pthread_setspecific(pthread_key_t key, const void *data)
{
    return syscall2(POSIX_PTHREAD_SETSPECIFIC, key, (int) data);
}

int pthread_key_create(pthread_key_t *key, void (*destructor)(void *))
{
    return syscall2(POSIX_PTHREAD_KEY_CREATE, (int) key, (int) destructor);
}

typedef void (*key_destructor)(void*);

key_destructor pthread_key_destructor(pthread_key_t key)
{
    return (key_destructor) syscall1(POSIX_PTHREAD_KEY_DESTRUCTOR, key);
}

int pthread_key_delete(pthread_key_t key)
{
    /// \todo Need a way of calling userspace functions from the kernel, without
    ///       creating a new specialisation of Event and an assembly stub for it
    ///       to be able to return.
    void *buff = pthread_getspecific(key);
    pthread_setspecific(key, 0);
    key_destructor a = pthread_key_destructor(key);
    if(a)
        a(buff);
    return syscall1(POSIX_PTHREAD_KEY_DELETE, key);
}
