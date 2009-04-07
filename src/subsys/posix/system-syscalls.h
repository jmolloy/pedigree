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

#ifndef SYSTEM_SYSCALLS_H
#define SYSTEM_SYSCALLS_H

#include <processor/Processor.h>
#include <processor/VirtualAddressSpace.h>
#include <processor/state.h>

#include "newlib.h"

#if 1
#define SC_NOTICE(x) NOTICE("[" << Dec << Processor::information().getCurrentThread()->getParent()->getId() << "]\t" << Hex << x)
#else
#define SC_NOTICE(x)
#endif

#define STACK_END    0x40000000
#define STACK_START  0x40020000
#define ARGV_ENV_LOC 0x40100000
#define ARGV_ENV_LEN 0x8000

int posix_sbrk(int delta);
int posix_fork(ProcessorState state);
int posix_execve(const char *name, const char **argv, const char **env, SyscallState &state);
int posix_waitpid(int pid, int *status, int options);
int posix_exit(int code);
int posix_getpid();

int posix_gettimeofday(timeval *tv, timezone *tz);

int posix_getpwent(passwd *pw, int n, char *str);
int posix_getpwnam(passwd *pw, const char *name, char *str);
int posix_getuid();
int posix_getgid();
int pedigree_login(int uid, const char *password);

int posix_sigaction(int sig, const struct sigaction *act, struct sigaction *oact);
uintptr_t posix_signal(int sig, void* func);
int posix_raise(int sig);
int posix_kill(int pid, int sig);
int posix_sigprocmask(int how, const uint32_t *set, uint32_t *oset);

int posix_alarm(uint32_t seconds);
int posix_sleep(uint32_t seconds);

int pedigree_sigret();

uintptr_t posix_dlopen(const char* file, int mode, void* p);
uintptr_t posix_dlsym(void* handle, const char* name);
int       posix_dlclose(void* handle);

#endif

