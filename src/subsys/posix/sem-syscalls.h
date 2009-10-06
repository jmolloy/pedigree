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
#ifndef _SEM_SYSCALLS_H
#define _SEM_SYSCALLS_H

#include <processor/Processor.h>
#include <processor/VirtualAddressSpace.h>
#include <processor/state.h>

#include <process/Semaphore.h>
#include "PosixSubsystem.h"

#include "newlib.h"

#if 1
#define SEM_NOTICE(x) NOTICE("[" << Dec << Processor::information().getCurrentThread()->getParent()->getId() << "]\t" << Hex << x)
#else
#define SEM_NOTICE(x)
#endif

int posix_sem_close(sem_t *);
int posix_sem_destroy(sem_t *);
int posix_sem_getvalue(sem_t *, int *);
int posix_sem_init(sem_t *, int, unsigned);
int posix_sem_post(sem_t *);
int posix_sem_timedwait(sem_t *, const struct timespec *);
int posix_sem_trywait(sem_t *);
int posix_sem_wait(sem_t *);

/// \todo Implement me :(
// sem_t *posix_sem_open(const char *, int, ...);
// int posix_sem_unlink(const char *);

#endif
