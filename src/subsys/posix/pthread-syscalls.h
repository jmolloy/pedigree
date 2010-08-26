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
#ifndef _PTHREAD_SYSCALLS_H
#define _PTHREAD_SYSCALLS_H

#include <processor/Processor.h>
#include <processor/VirtualAddressSpace.h>
#include <processor/state.h>

#include <process/Semaphore.h>
#include <process/Thread.h>
#include "PosixSubsystem.h"

#include "newlib.h"

#if 0
#define PT_NOTICE(x) NOTICE("[" << Dec << Processor::information().getCurrentThread()->getParent()->getId() << "." << Processor::information().getCurrentThread()->getId() << "]\t" << Hex << x)
#else
#define PT_NOTICE(x)
#endif

typedef void (*pthreadfn)(void*);
typedef void (*key_destructor)(void*);

int posix_pthread_create(pthread_t *thread, const pthread_attr_t *attr, pthreadfn start_addr, void *arg);
int posix_pthread_join(pthread_t thread, void **value_ptr);
int posix_pthread_detach(pthread_t thread);

pthread_t posix_pthread_self();

int posix_pthread_kill(pthread_t thread, int sig);
int posix_pthread_sigmask(int how, const uint32_t *set, uint32_t *oset);

int posix_pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr);
int posix_pthread_mutex_destroy(pthread_mutex_t *mutex);
int posix_pthread_mutex_lock(pthread_mutex_t *mutex);
int posix_pthread_mutex_trylock(pthread_mutex_t *mutex);
int posix_pthread_mutex_unlock(pthread_mutex_t *mutex);

int posix_pthread_enter(uintptr_t blk);
void posix_pthread_exit(void *ret);

void* posix_pthread_getspecific(pthread_key_t key);
int posix_pthread_setspecific(pthread_key_t key, const void *buff);
int posix_pthread_key_create(pthread_key_t *okey, key_destructor destructor);
int posix_pthread_key_delete(pthread_key_t key);
key_destructor posix_pthread_key_destructor(pthread_key_t key);

void pedigree_init_pthreads();

int posix_pedigree_thrwakeup(pthread_t thr);
int posix_pedigree_thrsleep(pthread_t thr);

#endif
