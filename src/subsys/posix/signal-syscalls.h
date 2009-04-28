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
#ifndef SIGNAL_SYSCALLS_H
#define SIGNAL_SYSCALLS_H

#include <processor/Processor.h>
#include <processor/VirtualAddressSpace.h>
#include <processor/state.h>

#include "newlib.h"

typedef void (*_sig_func_ptr)(int);

int posix_sigaction(int sig, const struct sigaction *act, struct sigaction *oact, int type);
uintptr_t posix_signal(int sig, void* func);
int posix_raise(int sig);
int posix_kill(int pid, int sig);
int posix_sigprocmask(int how, const uint32_t *set, uint32_t *oset);

#endif
