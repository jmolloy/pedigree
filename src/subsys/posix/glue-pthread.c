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

#include "posixSyscallNumbers.h"

// Define errno before including syscall.h.
#include "errno.h"
#define errno (*__errno())
extern int *__errno (void);
int h_errno; // required by networking code

#include "posix-syscall.h"

#define _WANT_STRING_H
#include "newlib.h"

#define _PTHREAD_ATTR_MAGIC 0xdeadbeef

// Define to 1 to get verbose debugging (hinders performance) in some functions
#define PTHREAD_DEBUG       0

typedef void (*pthread_once_func_t)(void);
int onceFunctions[32] = {0};

static int pedigree_thrwakeup(pthread_t t)
{
    return syscall1(POSIX_PEDIGREE_THRWAKEUP, (long) t);
}

static int pedigree_thrsleep(pthread_t t)
{
    return syscall1(POSIX_PEDIGREE_THRSLEEP, (long) t);
}

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

int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine)(void*), void *arg)
{
    return syscall4(POSIX_PTHREAD_CREATE, (long) thread, (long) attr, (long) start_routine, (long) arg);
}

int pthread_join(pthread_t thread, void **value_ptr)
{
    return syscall2(POSIX_PTHREAD_JOIN, (long) thread, (long) value_ptr);
}

void pthread_exit(void *ret)
{
    syscall1(POSIX_PTHREAD_RETURN, (long) ret);
}

int pthread_detach(pthread_t thread)
{
    return syscall1(POSIX_PTHREAD_DETACH, thread);
}

pthread_t pthread_self()
{
    pthread_t ret = 0;
#ifdef X86_COMMON
    asm volatile("mov %%fs:0, %0" : "=r" (ret));
#endif

#ifdef ARMV7
    asm volatile("mrc p15,0,%0,c13,c0,3" : "=r" (ret));
#endif

    return ret;
}

int pthread_equal(pthread_t t1, pthread_t t2)
{
    return (t1 == t2) ? 1 : 0;
}

int pthread_kill(pthread_t thread, int sig)
{
    return syscall2(POSIX_PTHREAD_KILL, (long) thread, sig);
}

int pthread_sigmask(int how, const sigset_t *set, sigset_t *oset)
{
    return syscall3(POSIX_PTHREAD_SIGMASK, how, (long) set, (long) oset);
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

/// Uses a pthread_mutex_t but really works for both semaphores and mutexes
void __pedigree_init_lock(pthread_mutex_t *mutex, int startValue)
{
    mutex->value = startValue;
}

int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr)
{
#if PTHREAD_DEBUG
    syslog(LOG_NOTICE, "pthread_mutex_init(%x)", mutex);
#endif

    if(!mutex)
    {
        errno = EINVAL;
        return -1;
    }

    memset(mutex, 0, sizeof(pthread_mutex_t));

    if(pthread_spin_init(&mutex->lock, 0) < 0)
        return -1; // errno from the function

    __pedigree_init_lock(mutex, 1);
    mutex->q = mutex->back = mutex->front = 0;

    return 0;
}

int pthread_mutex_destroy(pthread_mutex_t *mutex)
{
#if PTHREAD_DEBUG
    syslog(LOG_NOTICE, "pthread_mutex_destroy(%x)", mutex);
#endif

    if(!mutex)
    {
        errno = EINVAL;
        return -1;
    }

    mutex->value = 0;
    mutex->q = mutex->back = mutex->front = 0;

    pthread_spin_destroy(&mutex->lock);

    memset(mutex, 0, sizeof(pthread_mutex_t));

    return 0;
}

int pthread_mutex_lock(pthread_mutex_t *mutex)
{
#if PTHREAD_DEBUG
    syslog(LOG_NOTICE, "pthread_mutex_lock(%x)", mutex);
#endif

    if(!mutex)
    {
        errno = EINVAL;
        return -1;
    }

    // Attempt a direct acquire
    int32_t val = mutex->value;
    if((val - 1) >= 0)
    {
        if(__sync_bool_compare_and_swap(&mutex->value, val, val - 1))
            return 0;
    }

    // Couldn't acquire, lock
    mutex_q_item *i = (mutex_q_item*) malloc(sizeof(mutex_q_item));
    i->thr = pthread_self();
    i->next = 0;

    mutex_q_item *old_front = mutex->front;
    mutex_q_item *old_back = mutex->back;
    mutex_q_item *old_q = mutex->q;

    pthread_spin_lock(&mutex->lock);
    if(!mutex->q)
    {
        mutex->back = mutex->front = i;
        mutex->q = mutex->front;
    }
    else
    {
        mutex->back->next = i;
        mutex->back = i;
    }

    pthread_spin_unlock(&mutex->lock);

    // Check for mutex unlock while we've been manipulating the queue!
    if(val == (mutex->value - 1))
    {
        pthread_spin_lock(&mutex->lock);
        mutex->back = old_back;
        mutex->front = old_front;
        mutex->q = old_q;
        free(i);
        pthread_spin_unlock(&mutex->lock);

        return 0;
    }
    pedigree_thrsleep(i->thr);

    free(i);

    // Locked
    return 0;
}

int pthread_mutex_trylock(pthread_mutex_t *mutex)
{
#if PTHREAD_DEBUG
    syslog(LOG_NOTICE, "pthread_mutex_trylock(%x)", mutex);
#endif

    if(!mutex)
    {
        errno = EINVAL;
        return -1;
    }

    int32_t val = mutex->value;
    if((val - 1) >= 0)
    {
        if(__sync_bool_compare_and_swap(&mutex->value, val, val - 1))
            return 0;
    }

    errno = EBUSY;
    return -1;
}

int pthread_mutex_unlock(pthread_mutex_t *mutex)
{
#if PTHREAD_DEBUG
    syslog(LOG_NOTICE, "pthread_mutex_unlock(%x)", mutex);
#endif

    if(!mutex)
    {
        errno = EINVAL;
        return -1;
    }

    // Mutexes are binary semaphores.
    __sync_val_compare_and_swap(&mutex->value, 0, 1);

    pthread_spin_lock(&mutex->lock);
    if((!mutex->q) || (!mutex->front))
    {
        pthread_spin_unlock(&mutex->lock);
        return 0;
    }

    // Wake up the next waiting thread.
    mutex_q_item *front = mutex->front;
    if(front)
    {
        mutex->front = front->next;
        mutex->q = mutex->front;

        pthread_t thr = front->thr;
        pedigree_thrwakeup(thr);
    }

    pthread_spin_unlock(&mutex->lock);

    return 0;
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

/// \todo Eventually implement condvars properly rather than as a thin wrapper
///       arround a pair of mutexes.

int pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr)
{
#if PTHREAD_DEBUG
    syslog(LOG_NOTICE, "pthread_cond_init(%x)", cond);
#endif

    if(!cond)
    {
        errno = EINVAL;
        return -1;
    }

    int ret = pthread_mutex_init(cond, 0);
#if PTHREAD_DEBUG
    syslog(LOG_NOTICE, "pthread_cond_init: returning %d from mutex init [%s]\n", ret, strerror(errno));
#endif

    return ret;
}

int pthread_cond_destroy(pthread_cond_t *cond)
{
    if(!cond)
    {
        errno = EINVAL;
        return -1;
    }

    return pthread_mutex_destroy(cond);
}

int pthread_cond_broadcast(pthread_cond_t *cond)
{
    if(!cond)
    {
        errno = EINVAL;
        return -1;
    }

    pthread_spin_lock(&cond->lock);
    while(cond->front)
    {
        __sync_fetch_and_sub(&cond->value, 1);

        pedigree_thrwakeup(cond->front->thr);
        cond->front = cond->front->next;
    }
    pthread_spin_unlock(&cond->lock);

    return 0;
}

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
    if((!cond) || (!mutex))
    {
        errno = EINVAL;
        return -1;
    }

    int e = 0;
    e = pthread_mutex_unlock(mutex);
    if(e)
        return e;
    e = pthread_mutex_lock(cond);
    pthread_mutex_lock(mutex);

    return e;
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
    return syscall2(POSIX_PTHREAD_SETSPECIFIC, key, (long) data);
}

int pthread_key_create(pthread_key_t *key, void (*destructor)(void *))
{
    return syscall2(POSIX_PTHREAD_KEY_CREATE, (long) key, (long) destructor);
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



int pthread_spin_init(pthread_spinlock_t *lock, int pshared)
{
    if(!lock)
    {
        errno = EINVAL;
        return -1;
    }

    lock->atom = 1;
    lock->owner = pthread_self();
    lock->locker = -1;
    return 0;
}

int pthread_spin_destroy(pthread_spinlock_t *lock)
{
    if(!lock)
    {
        errno = EINVAL;
        return -1;
    }

    if(lock->locker >= 0)
    {
        errno = EBUSY;
        return -1;
    }

    lock->owner = lock->locker = -1;
    return 0;
}

int pthread_spin_lock(pthread_spinlock_t *lock)
{
    if(!lock)
    {
        errno = EINVAL;
        return -1;
    }

    while(!__sync_bool_compare_and_swap(&lock->atom, 1, 0))
    {
        if(lock->locker == pthread_self())
        {
            // Attempt to lock the lock... but we've already acquired it!
            errno = EDEADLK;
            return -1;
        }

        /// \todo If there are no other threads running, this should throw EDEADLK
        sched_yield();
    }

    lock->locker = pthread_self();

    return 0;
}

int pthread_spin_trylock(pthread_spinlock_t *lock)
{
    if(!lock)
    {
        errno = EINVAL;
        return -1;
    }

    if(!__sync_bool_compare_and_swap(&lock->atom, 1, 0))
    {
        errno = EBUSY;
        return -1;
    }

    lock->locker = pthread_self();

    return 0;
}

int pthread_spin_unlock(pthread_spinlock_t *lock)
{
    if(!lock)
    {
        errno = EINVAL;
        return -1;
    }

    if(lock->locker < 0)
    {
        errno = EPERM;
        return -1;
    }

    lock->locker = -1;
    __sync_bool_compare_and_swap(&lock->atom, 0, 1);

    // Avoids a case where, in a loop constantly performing an acquire, no other
    // thread can access the spinlock.
    sched_yield();

    return 0;
}
