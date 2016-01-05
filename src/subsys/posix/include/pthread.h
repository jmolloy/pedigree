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

#ifndef __PTHREAD_H
#define __PTHREAD_H

#ifndef _POSIX_THREADS
#define _POSIX_THREADS
#endif

#include <sys/types.h>
#include <stdint.h>
#include <unistd.h>
#include <signal.h>
#include <sched.h>

#ifndef PTHREAD_STACK_MIN
#define PTHREAD_STACK_MIN 0x1000
#endif

// Marks code as not implemented for those reading header files
#define NOTIMPL

// #define PTHREAD_CANCEL_ASYNCHRONOUS
#define PTHREAD_CANCEL_ENABLE           1
// #define PTHREAD_CANCEL_DEFERRED
#define PTHREAD_CANCEL_DISABLE          0
#define PTHREAD_CANCELED
#define PTHREAD_COND_INITIALIZER        PTHREAD_MUTEX_INITIALIZER

#define PTHREAD_CREATE_DETACHED         1
#define PTHREAD_CREATE_JOINABLE         0

#define PTHREAD_EXPLICIT_SCHED
#define PTHREAD_INHERIT_SCHED

#define PTHREAD_PROCESS_SHARED          1
#define PTHREAD_PROCESS_PRIVATE         2

#define PTHREAD_MUTEX_NORMAL            0
#define PTHREAD_MUTEX_ERRORCHECK        1
#define PTHREAD_MUTEX_RECURSIVE         2
#define PTHREAD_MUTEX_DEFAULT           3

#define PTHREAD_MUTEXATTR_INITIALIZER   {{{PTHREAD_MUTEX_NORMAL}}}
#define PTHREAD_MUTEX_INITIALIZER       {{{1, 0, PTHREAD_MUTEXATTR_INITIALIZER, 0}}}

#define PTHREAD_ONCE_INIT               {{0}}

#ifdef __cplusplus
extern "C" {
#endif

// Base functionality
int         _EXFUN(pthread_create, (pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine)(void*), void *arg));
int         _EXFUN(pthread_join, (pthread_t thread, void **value_ptr));
void        _EXFUN(pthread_exit, (void *value_ptr));
int         _EXFUN(pthread_detach, (pthread_t thread));
pthread_t   _EXFUN(pthread_self, (void));
int         _EXFUN(pthread_equal, (pthread_t t1, pthread_t t2));
int         _EXFUN(pthread_atfork, (void (*prepare)(void), void (*parent)(void), void (*child)(void)));
int         _EXFUN(pthread_kill, (pthread_t thread, int sig));
int         _EXFUN(pthread_sigmask, (int how, const sigset_t *set, sigset_t *oset));

// Extended functionality
int         _EXFUN(pthread_once, (pthread_once_t *, void (*)(void)));

// Thread cancellation
int NOTIMPL _EXFUN(pthread_cancel, (pthread_t));
/*
void        _EXFUN(pthread_cleanup_push, (void (*)(void *), void *));
void        _EXFUN(pthread_cleanup_pop, (int));
int         _EXFUN(pthread_setcancelstate, (int, int *));
int         _EXFUN(pthread_setcanceltype, (int, int *));
void        _EXFUN(pthread_testcancel, (void));
*/

// Thread Attributes
int         _EXFUN(pthread_attr_init, (pthread_attr_t *));
int         _EXFUN(pthread_attr_destroy, (pthread_attr_t *));
int         _EXFUN(pthread_attr_getdetachstate, (const pthread_attr_t *, int *));
int         _EXFUN(pthread_attr_setdetachstate, (pthread_attr_t *attr, int detachstate));
int         _EXFUN(pthread_attr_getstacksize, (const pthread_attr_t *, size_t *));
int         _EXFUN(pthread_attr_setstacksize, (pthread_attr_t *attr, size_t stacksize));
int NOTIMPL _EXFUN(pthread_attr_getschedparam, (const pthread_attr_t *attr, struct sched_param *param));
int NOTIMPL _EXFUN(pthread_attr_setschedparam, (pthread_attr_t *attr, const struct sched_param *param));

// Keys
void*       _EXFUN(pthread_getspecific, (pthread_key_t));
int         _EXFUN(pthread_setspecific, (pthread_key_t, const void *));
int         _EXFUN(pthread_key_create, (pthread_key_t *, void (*)(void *)));
int         _EXFUN(pthread_key_delete, (pthread_key_t));

// Mutexes
int         _EXFUN(pthread_mutex_init, (pthread_mutex_t *, const pthread_mutexattr_t *));
int         _EXFUN(pthread_mutex_destroy, (pthread_mutex_t *));
int         _EXFUN(pthread_mutex_lock, (pthread_mutex_t *));
int         _EXFUN(pthread_mutex_trylock, (pthread_mutex_t *));
int         _EXFUN(pthread_mutex_unlock, (pthread_mutex_t *));
int         _EXFUN(pthread_mutexattr_init, (pthread_mutexattr_t *));
int         _EXFUN(pthread_mutexattr_destroy, (pthread_mutexattr_t *));
int         _EXFUN(pthread_mutexattr_gettype, (const pthread_mutexattr_t *, int *));
int         _EXFUN(pthread_mutexattr_settype, (pthread_mutexattr_t *, int));

// Condition Variables
int         _EXFUN(pthread_cond_init, (pthread_cond_t *, const pthread_condattr_t *));
int         _EXFUN(pthread_cond_destroy, (pthread_cond_t *));
int         _EXFUN(pthread_cond_broadcast, (pthread_cond_t *));
int         _EXFUN(pthread_cond_signal, (pthread_cond_t *));
int NOTIMPL _EXFUN(pthread_cond_timedwait, (pthread_cond_t *, pthread_mutex_t *, const struct timespec *));
int         _EXFUN(pthread_cond_wait, (pthread_cond_t *, pthread_mutex_t *));
int         _EXFUN(pthread_condattr_destroy, (pthread_condattr_t *));
int         _EXFUN(pthread_condattr_init, (pthread_condattr_t *));
int         _EXFUN(pthread_condattr_getclock, (const pthread_condattr_t *, clockid_t *));
int         _EXFUN(pthread_condattr_setclock, (pthread_condattr_t *, clockid_t));


// RW Locks
int         _EXFUN(pthread_rwlockattr_init, (pthread_rwlockattr_t *));
int         _EXFUN(pthread_rwlock_destroy, (pthread_rwlock_t *));
int         _EXFUN(pthread_rwlock_init, (pthread_rwlock_t *, const pthread_rwlockattr_t *));
int         _EXFUN(pthread_rwlock_rdlock, (pthread_rwlock_t *));
int         _EXFUN(pthread_rwlock_tryrdlock, (pthread_rwlock_t *));
int         _EXFUN(pthread_rwlock_trywrlock, (pthread_rwlock_t *));
int         _EXFUN(pthread_rwlock_unlock, (pthread_rwlock_t *));
int         _EXFUN(pthread_rwlock_wrlock, (pthread_rwlock_t *));
int         _EXFUN(pthread_rwlockattr_destroy, (pthread_rwlockattr_t *));

int         _EXFUN(pthread_spin_destroy, (pthread_spinlock_t*));
int         _EXFUN(pthread_spin_init, (pthread_spinlock_t*, int));
int         _EXFUN(pthread_spin_lock, (pthread_spinlock_t*));
int         _EXFUN(pthread_spin_trylock, (pthread_spinlock_t*));
int         _EXFUN(pthread_spin_unlock, (pthread_spinlock_t*));

#ifdef __cplusplus
}
#endif

#endif
