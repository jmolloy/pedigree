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

#ifndef _PTHREAD_SYSCALLS_H
#define _PTHREAD_SYSCALLS_H

#include <processor/Processor.h>
#include <processor/VirtualAddressSpace.h>
#include <processor/state.h>

#include <process/Semaphore.h>
#include <process/Thread.h>
#include "PosixSubsystem.h"

#include "newlib.h"

#if 1
#define PT_NOTICE(x) NOTICE("[" << Dec << Processor::information().getCurrentThread()->getParent()->getId() << "." << Processor::information().getCurrentThread()->getId() << "]\t" << Hex << x)
#else
#define PT_NOTICE(x)
#endif

typedef void (*pthreadfn)(void*);
typedef void (*key_destructor)(void*);

int posix_pthread_create(pthread_t *thread, const pthread_attr_t *attr, pthreadfn start_addr, void *arg);
int posix_pthread_join(pthread_t *thread, void **value_ptr);
int posix_pthread_detach(pthread_t *thread);

int posix_pthread_kill(pthread_t *thread, int sig);
int posix_pthread_sigmask(int how, const uint32_t *set, uint32_t *oset);

int posix_pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr);
int posix_pthread_mutex_destroy(pthread_mutex_t *mutex);
int posix_pthread_mutex_lock(pthread_mutex_t *mutex);
int posix_pthread_mutex_trylock(pthread_mutex_t *mutex);
int posix_pthread_mutex_unlock(pthread_mutex_t *mutex);

int posix_pthread_enter(uintptr_t blk);
void posix_pthread_exit(void *ret);

void* posix_pthread_getspecific(pthread_key_t *key);
int posix_pthread_setspecific(pthread_key_t *key, const void *buff);
int posix_pthread_key_create(pthread_key_t *okey, key_destructor destructor);
int posix_pthread_key_delete(pthread_key_t *key);
key_destructor posix_pthread_key_destructor(pthread_key_t *key);

void pedigree_init_pthreads();
void pedigree_copy_posix_thread(Thread *, PosixSubsystem *, Thread *, PosixSubsystem *);

/// Creates a new wait object that threads can use to synchronise.
void *posix_pedigree_create_waiter();
int posix_pedigree_thread_wait_for(void *waiter);
int posix_pedigree_thread_trigger(void *waiter);
void posix_pedigree_destroy_waiter(void *waiter);

#endif
