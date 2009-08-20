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

#if 1
#define PT_NOTICE(x) NOTICE("[" << Dec << Processor::information().getCurrentThread()->getParent()->getId() << "]\t" << Hex << x)
#else
#define PT_NOTICE(x)
#endif

typedef void (*pthreadfn)(void*);

int posix_pthread_create(pthread_t *thread, const pthread_attr_t *attr, pthreadfn start_addr, void *arg);
int posix_pthread_join(pthread_t thread, void **value_ptr);

void posix_pthread_enter();
void posix_pthread_return(uintptr_t ret);

void pedigree_init_pthreads();

#endif
