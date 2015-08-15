/*
 * Copyright (c) 2008-2014, Pedigree Developers
 *
 * Please see the CONTRIB file in the root of the source tree for a full
 * list of contributors.
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

#define STUBBED(str) syscall1(POSIX_STUBBED, (long)(str)); \
    errno = ENOSYS;

typedef void (*pthread_once_func_t)(void);
static int onceFunctions[32] = {0};

static void *_pedigree_create_waiter()
{
    return (void *) syscall0(POSIX_PEDIGREE_CREATE_WAITER);
    return 0;
}

static void _pedigree_destroy_waiter(void *waiter)
{
    syscall1(POSIX_PEDIGREE_DESTROY_WAITER, (long) waiter);
}
static int _pedigree_thread_wait_for(void *waiter)
{
    if (!waiter)
    {
        errno = EINVAL;
        return -1;
    }
    return syscall1(POSIX_PEDIGREE_THREAD_WAIT_FOR, (long) waiter);
}

static int _pedigree_thread_trigger(void *waiter)
{
    return syscall1(POSIX_PEDIGREE_THREAD_TRIGGER, (long) waiter);
}

static int _pthread_is_valid(pthread_t p)
{
    return p && p->__internal.kthread >= 0;
}

static void _pthread_make_invalid(pthread_t p)
{
    p->__internal.kthread = -1;
}

int pthread_once(pthread_once_t *once_control, pthread_once_func_t init_routine)
{
    int control = once_control->__internal.control;
    if(!control || (control > 32))
    {
        syslog(LOG_DEBUG, "[%d] pthread_once called with an invalid once_control (> 32)", getpid());
        return -1;
    }

    if(!onceFunctions[control])
    {
        init_routine();
        onceFunctions[control] = 1;
        ++once_control->__internal.control;
    }

    return 0;
}

int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine)(void*), void *arg)
{
    *thread = (pthread_t) malloc(sizeof(**thread));
    return syscall4(POSIX_PTHREAD_CREATE, (long) thread, (long) attr, (long) start_routine, (long) arg);
}

int pthread_join(pthread_t thread, void **value_ptr)
{
    int result = syscall2(POSIX_PTHREAD_JOIN, (long) thread, (long) value_ptr);
    free(thread);
    return result;
}

void pthread_exit(void *ret)
{
    syscall1(POSIX_PTHREAD_RETURN, (long) ret);
}

int pthread_detach(pthread_t thread)
{
    return syscall1(POSIX_PTHREAD_DETACH, (long) thread);
}

pthread_t pthread_self()
{
    /// \todo this is not brilliant.
    static struct _pthread_t result;
#ifdef X86_COMMON
    asm volatile("mov %%fs:0, %0" : "=r" (result.__internal.kthread));
#endif

#ifdef ARMV7
    asm volatile("mrc p15,0,%0,c13,c0,3" : "=r" (result.__internal.kthread));
#endif

    return &result;
}

int pthread_equal(pthread_t t1, pthread_t t2)
{
    if (!(t1 && t2))
        return 0;
    return t1->__internal.kthread == t2->__internal.kthread;
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

    if(attr->__internal.magic == _PTHREAD_ATTR_MAGIC)
    {
        errno = EBUSY;
        return -1;
    }

    attr->__internal.stackSize = 0x100000;
    attr->__internal.detachState = 0;
    attr->__internal.magic = _PTHREAD_ATTR_MAGIC;
    return 0;
}

int pthread_attr_destroy(pthread_attr_t *attr)
{
    if(!attr)
    {
        errno = ENOMEM;
        return -1;
    }

    if(attr->__internal.magic != _PTHREAD_ATTR_MAGIC)
    {
        errno = EINVAL;
        return -1;
    }

    return 0;
}

int pthread_attr_getdetachstate(const pthread_attr_t *attr, int *ret)
{
    if(!attr || (attr->__internal.magic != _PTHREAD_ATTR_MAGIC) || !ret)
    {
        errno = EINVAL;
        return -1;
    }

    *ret = attr->__internal.detachState;
    return -1;
}

int pthread_attr_setdetachstate(pthread_attr_t *attr, int detachstate)
{
    if((!attr || (attr->__internal.magic != _PTHREAD_ATTR_MAGIC)) ||
        (detachstate != PTHREAD_CREATE_DETACHED && detachstate != PTHREAD_CREATE_JOINABLE))
    {
        errno = EINVAL;
        return -1;
    }

    attr->__internal.detachState = detachstate;
    return -1;
}

int pthread_attr_getstacksize(const pthread_attr_t *attr, size_t *sz)
{
    if(!attr || (attr->__internal.magic != _PTHREAD_ATTR_MAGIC) || !sz)
    {
        errno = EINVAL;
        return -1;
    }

    *sz = attr->__internal.stackSize;

    return 0;
}

int pthread_attr_setstacksize(pthread_attr_t *attr, size_t stacksize)
{
    if(!attr || (attr->__internal.magic != _PTHREAD_ATTR_MAGIC) ||
        (stacksize < PTHREAD_STACK_MIN) || (stacksize > (1 << 24)))
    {
        errno = EINVAL;
        return -1;
    }

    attr->__internal.stackSize = stacksize;

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
#if PTHREAD_DEBUG
    syslog(LOG_NOTICE, "pthread_mutex_init(%x)", mutex);
#endif

    if(!mutex)
    {
        errno = EINVAL;
        return -1;
    }

    memset(mutex, 0, sizeof(pthread_mutex_t));

    mutex->__internal.value = 1;
    _pthread_make_invalid(mutex->__internal.owner);
    mutex->__internal.waiter = _pedigree_create_waiter();

    if (attr)
    {
        mutex->__internal.attr = *attr;
    }

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

    mutex->__internal.value = 0;
    _pedigree_destroy_waiter(mutex->__internal.waiter);

    memset(mutex, 0, sizeof(pthread_mutex_t));

    return 0;
}

int pthread_mutex_lock(pthread_mutex_t *mutex)
{
#if PTHREAD_DEBUG
    syslog(LOG_NOTICE, "pthread_mutex_lock(%p) [return: %p]", mutex, __builtin_return_address(0));
#endif

    /**
     * A mutex in this case is just a binary Semaphore.
     * The initial value therefore is '1' (ie, unlocked).
     * When locking, this is reduced to zero, and then lower if we recurse.
     * The lock can only be taken if the counter is non-zero.
     */

    if(!mutex)
    {
        errno = EINVAL;
        return -1;
    }

    while(1)
    {
        int r = pthread_mutex_trylock(mutex);
        if ((r < 0) && (errno != EBUSY))
        {
            // Error!
            return r;
        }
        else if (r == 0)
        {
            // Acquired!
            return 0;
        }
        else
        {
            // Busy.
            if (_pedigree_thread_wait_for(mutex->__internal.waiter) < 0)
            {
                // Error comes from the syscall.
                return -1;
            }
        }
    }
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

    int32_t val = mutex->__internal.value;
    if((val - 1) >= 0)
    {
        if(__sync_bool_compare_and_swap(&mutex->__internal.value, val, val - 1))
            goto locked;
    }

    if (mutex->__internal.attr.__internal.type == PTHREAD_MUTEX_RECURSIVE)
    {
        if (pthread_equal(pthread_self(), mutex->__internal.owner))
        {
            // Recurse.
            if(__sync_bool_compare_and_swap(&mutex->__internal.value, val, val - 1))
                goto locked;
        }
    }

    goto err;

locked:
    mutex->__internal.owner = pthread_self();
    return 0;
err:
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

    // Is the mutex OK?
    if (!_pthread_is_valid(mutex->__internal.owner))
    {
        errno = EPERM;
        return -1;
    }

    // Are we allowed to unlock this mutex?
    if (!pthread_equal(mutex->__internal.owner, pthread_self()))
    {
        errno = EPERM;
        return -1;
    }

    // Perform the actual unlock.
    int32_t val = mutex->__internal.value;
    if (!__sync_bool_compare_and_swap(&mutex->__internal.value, val, val + 1))
    {
        // Someone may have reached there first. But how? Weird.
        syslog(LOG_ALERT, "CaS failed in pthread_mutex_unlock!");
    }

    // If the result ended up not actually unlocking the lock (eg, recursion),
    // don't wake up any threads just yet.
    if ((val + 1) <= 0)
    {
        return 0;
    }

    // Otherwise we're good to wake stuff up.
    _pthread_make_invalid(mutex->__internal.owner);
    _pedigree_thread_trigger(mutex->__internal.waiter);

    return 0;
}

int pthread_mutexattr_init(pthread_mutexattr_t *attr)
{
    attr->__internal.type = PTHREAD_MUTEX_DEFAULT;
    return 0;
}

int pthread_mutexattr_destroy(pthread_mutexattr_t *attr)
{
    return 0;
}

int pthread_mutexattr_gettype(const pthread_mutexattr_t *attr, int *type)
{
    *type = attr->__internal.type;
    return 0;
}

int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type)
{
    /// \todo erorr checking...
    attr->__internal.type = type;
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
#if PTHREAD_DEBUG
    syslog(LOG_NOTICE, "pthread_cond_destroy(%x)", cond);
#endif

    if(!cond)
    {
        errno = EINVAL;
        return -1;
    }

    return pthread_mutex_destroy(cond);
}

int pthread_cond_broadcast(pthread_cond_t *cond)
{
#if PTHREAD_DEBUG
    syslog(LOG_NOTICE, "pthread_cond_broadcast(%x)", cond);
#endif

    if(!cond)
    {
        errno = EINVAL;
        return -1;
    }

    do
    {
        __sync_fetch_and_sub(&cond->__internal.value, 1);
    }
    while(_pedigree_thread_trigger(cond->__internal.waiter) > 0);

    return 0;
}

int pthread_cond_signal(pthread_cond_t *cond)
{
#if PTHREAD_DEBUG
    syslog(LOG_NOTICE, "pthread_cond_signal(%x)", cond);
#endif

    return pthread_mutex_unlock(cond);
}

int pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex, const struct timespec *tm)
{
#if PTHREAD_DEBUG
    syslog(LOG_NOTICE, "pthread_cond_timedwait(%x)", cond);
#endif

    errno = ENOSYS;
    return -1;
}

int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex)
{
#if PTHREAD_DEBUG
    syslog(LOG_NOTICE, "pthread_cond_wait(%x, %x)", cond, mutex);
#endif

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
    attr->__internal.clock_id = CLOCK_MONOTONIC;
    return 0;
}

int pthread_condattr_getclock(const pthread_condattr_t *restrict attr, clockid_t *restrict clock_id)
{
    *clock_id = attr->__internal.clock_id;
    return 0;
}

int pthread_condattr_setclock(pthread_condattr_t *attr, clockid_t clock_id)
{
    attr->__internal.clock_id = clock_id;
    return 0;
}

void* pthread_getspecific(pthread_key_t key)
{
    return (void*) syscall1(POSIX_PTHREAD_GETSPECIFIC, (long) &key);
}

int pthread_setspecific(pthread_key_t key, const void *data)
{
    return syscall2(POSIX_PTHREAD_SETSPECIFIC, (long) &key, (long) data);
}

int pthread_key_create(pthread_key_t *key, void (*destructor)(void *))
{
    return syscall2(POSIX_PTHREAD_KEY_CREATE, (long) key, (long) destructor);
}

typedef void (*key_destructor)(void*);

key_destructor pthread_key_destructor(pthread_key_t key)
{
    return (key_destructor) syscall1(POSIX_PTHREAD_KEY_DESTRUCTOR, (long) &key);
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
    return syscall1(POSIX_PTHREAD_KEY_DELETE, (long) &key);
}

int pthread_rwlockattr_init(pthread_rwlockattr_t *attr)
{
    return 0;
}

int pthread_rwlock_destroy(pthread_rwlock_t *lock)
{
#if PTHREAD_DEBUG
    syslog(LOG_NOTICE, "pthread_rwlock_destroy(%x)", lock);
#endif

    return pthread_mutex_destroy(&lock->mutex);
}

int pthread_rwlock_init(pthread_rwlock_t *lock, const pthread_rwlockattr_t *attr)
{
#if PTHREAD_DEBUG
    syslog(LOG_NOTICE, "pthread_rwlock_init(%x)", lock);
#endif

    return pthread_mutex_init(&lock->mutex, 0);
}

int pthread_rwlock_rdlock(pthread_rwlock_t *lock)
{
#if PTHREAD_DEBUG
    syslog(LOG_NOTICE, "pthread_rwlock_rdlock(%x)", lock);
#endif

    return pthread_mutex_lock(&lock->mutex);
}

int pthread_rwlock_tryrdlock(pthread_rwlock_t *lock)
{
#if PTHREAD_DEBUG
    syslog(LOG_NOTICE, "pthread_rwlock_tryrdlock(%x)", lock);
#endif

    return pthread_mutex_trylock(&lock->mutex);
}

int pthread_rwlock_trywrlock(pthread_rwlock_t *lock)
{
#if PTHREAD_DEBUG
    syslog(LOG_NOTICE, "pthread_rwlock_trywrlock(%x)", lock);
#endif

    return pthread_mutex_trylock(&lock->mutex);
}

int pthread_rwlock_unlock(pthread_rwlock_t *lock)
{
#if PTHREAD_DEBUG
    syslog(LOG_NOTICE, "pthread_rwlock_unlock(%x)", lock);
#endif

    return pthread_mutex_unlock(&lock->mutex);
}

int pthread_rwlock_wrlock(pthread_rwlock_t *lock)
{
#if PTHREAD_DEBUG
    syslog(LOG_NOTICE, "pthread_rwlock_wrlock(%x)", lock);
#endif

    return pthread_mutex_lock(&lock->mutex);
}

int pthread_rwlockattr_destroy(pthread_rwlockattr_t *attr)
{
    return 0;
}

int pthread_spin_init(pthread_spinlock_t *lock, int pshared)
{
    if(!lock)
    {
        errno = EINVAL;
        return -1;
    }

    lock->__internal.atom = 1;
    lock->__internal.owner = pthread_self();
    _pthread_make_invalid(lock->__internal.locker);
    return 0;
}

int pthread_spin_destroy(pthread_spinlock_t *lock)
{
    if(!lock)
    {
        errno = EINVAL;
        return -1;
    }

    if(_pthread_is_valid(lock->__internal.locker))
    {
        errno = EBUSY;
        return -1;
    }

    _pthread_make_invalid(lock->__internal.locker);
    _pthread_make_invalid(lock->__internal.owner);
    return 0;
}

int pthread_spin_lock(pthread_spinlock_t *lock)
{
    if(!lock)
    {
        errno = EINVAL;
        return -1;
    }

    int r = 0;
    while(!(r = pthread_spin_trylock(lock)))
    {
        if(pthread_equal(lock->__internal.locker, pthread_self()))
        {
            // Attempt to lock the lock... but we've already acquired it!
            errno = EDEADLK;
            return -1;
        }

        /// \todo EDEADLK from other sources?
        sched_yield();
    }

    lock->__internal.locker = pthread_self();

    return 0;
}

int pthread_spin_trylock(pthread_spinlock_t *lock)
{
    if(!lock)
    {
        errno = EINVAL;
        return -1;
    }

    if(!__sync_bool_compare_and_swap(&lock->__internal.atom, 1, 0))
    {
        errno = EBUSY;
        return -1;
    }

    lock->__internal.locker = pthread_self();

    return 0;
}

int pthread_spin_unlock(pthread_spinlock_t *lock)
{
    if(!lock)
    {
        errno = EINVAL;
        return -1;
    }

    // No locker.
    if(!_pthread_is_valid(lock->__internal.locker))
    {
        errno = EPERM;
        return -1;
    }

    _pthread_make_invalid(lock->__internal.locker);
    __sync_bool_compare_and_swap(&lock->__internal.atom, 0, 1);

    // Avoids a case where, in a loop constantly performing an acquire, no other
    // thread can access the spinlock.
    sched_yield();

    return 0;
}
