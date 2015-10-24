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

// Contains implementations of syscalls that use global info blocks.

#include "posixSyscallNumbers.h"

#include "errno.h"
#define errno (*__errno())
extern int *__errno (void);

#include "posix-syscall.h"

#include "newlib.h"

#include <string.h>

#include <compiler.h>
#include <process/InfoBlock.h>

static struct InfoBlock *infoBlock = 0;
static int hasInfoBlock = 0;

#define CHECK_INFO_BLOCK  if (UNLIKELY(!infoBlock)) getInfoBlock()

static void getInfoBlock()
{
    uintptr_t loc = syscall0(POSIX_PEDIGREE_GET_INFO_BLOCK);
    if (loc)
    {
        infoBlock = (struct InfoBlock *) loc;
        hasInfoBlock = 1;
    }
}

int gettimeofday(struct timeval *tv, void *tz)
{
    CHECK_INFO_BLOCK;
    if (!hasInfoBlock)
        return syscall2(POSIX_GETTIMEOFDAY, (long)tv, (long)tz);

    if (!tv)
    {
        errno = EINVAL;
        return -1;
    }

    // 'now' is in nanoseconds.
    /// \todo use tz
    uint64_t now = infoBlock->now;
    tv->tv_sec = now / 1000000000U;
    tv->tv_usec = now / 1000U;

    return 0;
}

int clock_gettime(clockid_t clock_id, struct timespec *tp)
{
    CHECK_INFO_BLOCK;
    if (!hasInfoBlock)
        return syscall2(POSIX_CLOCK_GETTIME, clock_id, (long) tp);

    if(!tp)
    {
        errno = EINVAL;
        return -1;
    }

    // 'now' is in nanoseconds.
    uint64_t now = infoBlock->now;
    tp->tv_sec = now / 1000000000U;
    tp->tv_nsec = now % 1000000000U;

    return 0;
}

int getpid()
{
    CHECK_INFO_BLOCK;
    if (!hasInfoBlock)
        return (long)syscall0(POSIX_GETPID);

    /// \todo this would make more sense on a process-specific infoblock, but
    ///       we don't have that yet.
    return infoBlock->pid;
}

int uname(struct utsname *n)
{
    CHECK_INFO_BLOCK;
    if (!hasInfoBlock)
        return -1;

    if (!n)
        return -1;

    strcpy(n->sysname, infoBlock->sysname);
    strcpy(n->release, infoBlock->release);
    strcpy(n->version, infoBlock->version);
    strcpy(n->machine, infoBlock->machine);
    strcpy(n->nodename, "pedigree.local");
    return 0;
}
